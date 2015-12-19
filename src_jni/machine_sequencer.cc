/*
 * VuKNOB
 * Copyright (C) 2014 by Anton Persson
 *
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
 * Copyright (C) 2011 by Anton Persson
 *
 * About SATAN:
 *   Originally developed as a small subproject in
 *   a course about applied signal processing.
 * Original Developers:
 *   Anton Persson
 *   Johan Thim
 *
 * http://www.733kru.org/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "machine_sequencer.hh"
#include "dynlib/dynlib.h"

#include <stddef.h>

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/*************************************
 *
 * Exception classes
 *
 *************************************/

MachineSequencer::NoSuchController::NoSuchController(const std::string &name) :
	jException(name + " : no such controller", jException::sanity_error) {}

/*************************************
 *
 * Class MachineSequencer::NoteEntry
 *
 *************************************/

MachineSequencer::NoteEntry::NoteEntry(const NoteEntry *original) : prev(NULL), next(NULL) {
	this->set_to(original);
}

MachineSequencer::NoteEntry::NoteEntry() : prev(NULL), next(NULL) {
	channel = program = note = 0;
	velocity = 0x7f;
	on_at = length = -1;
}

MachineSequencer::NoteEntry::~NoteEntry() {
}

void MachineSequencer::NoteEntry::set_to(const NoteEntry *original) {
	this->channel = (original->channel & 0x0f);
	this->program = (original->program & 0x7f);
	this->velocity = (original->velocity & 0x7f);
	this->note = (original->note & 0x7f);
	this->on_at = original->on_at;
	this->length = original->length;
}

/*************************************
 *
 * Class MachineSequencer::Loop
 *
 *************************************/

MachineSequencer::Loop *MachineSequencer::LoopCreator::create(const KXMLDoc &loop_xml) {
	return new Loop(loop_xml);
}

MachineSequencer::Loop *MachineSequencer::LoopCreator::create() {
	return new Loop();
}

MachineSequencer::Loop::Loop(const KXMLDoc &loop_xml) :
	note(NULL), next_note_to_play(NULL) {

	for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
		active_note[x] = NULL;
	}

	int c, c_max = 0;

	try {
		c_max = loop_xml["note"].get_count();
	} catch(...) {
		// ignore
	}

	NoteEntry temp_note_NULL;
	for(c = 0; c < c_max; c++) {
		KXMLDoc note_xml = loop_xml["note"][c];
		NoteEntry temp_note;

		temp_note.set_to(&temp_note_NULL);

		int value, line, tick, length;
		KXML_GET_NUMBER(note_xml,"channel",value, 0); temp_note.channel = value;
		KXML_GET_NUMBER(note_xml,"program", value, 0); temp_note.program = value;
		KXML_GET_NUMBER(note_xml,"velocity", value, 127); temp_note.velocity = value;
		KXML_GET_NUMBER(note_xml,"note", value, 0); temp_note.note = value;
		KXML_GET_NUMBER(note_xml,"on", line, -1);
		KXML_GET_NUMBER(note_xml,"on_tick", tick, 0);
		KXML_GET_NUMBER(note_xml,"length", length, -1);
		if(line != -1) { // old style, project_interface_level < 6
			temp_note.on_at = PAD_TIME(line, tick);
			temp_note.length = PAD_TIME(length, 0);
		} else {
			temp_note.on_at = tick;
			temp_note.length = length;
		}

		if(temp_note.length >= 0) {
			internal_insert_note(new NoteEntry(&temp_note));
		}
	}
}

MachineSequencer::Loop::Loop() :
	note(NULL), next_note_to_play(NULL) {

	for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
		active_note[x] = NULL;
	}
}

MachineSequencer::Loop::~Loop() {
	// XXX no proper cleanup done yet, we never delete a loop...
}

void MachineSequencer::Loop::get_loop_xml(std::ostringstream &stream, int id) {
	stream << "<loop id=\"" << id << "\" >\n";

	NoteEntry *crnt = note;

	while(crnt != NULL) {
		stream << "    <note "
		       << " channel=\"" << (int)(crnt->channel) << "\""
		       << " program=\"" << (int)(crnt->program) << "\""
		       << " velocity=\"" << (int)(crnt->velocity) << "\""
		       << " note=\"" << (int)(crnt->note) << "\""
		       << " on_tick=\"" << crnt->on_at << "\""
		       << " length=\"" << crnt->length << "\""
		       << " />\n"
			;

		crnt = crnt->next;
	}

	stream << "</loop>\n";
}

void MachineSequencer::Loop::start_to_play() {
	loop_position = 0;
	next_note_to_play = note;
}

bool MachineSequencer::Loop::activate_note(NoteEntry *note) {
	for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
		if(active_note[x] == NULL) {
			active_note[x] = note;
			return true;
		}
	}
	return false;
}

void MachineSequencer::Loop::deactivate_note(NoteEntry *note) {
	for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
		if(active_note[x] == note) {
			active_note[x] = NULL;
			return;
		}
	}
}

void MachineSequencer::Loop::process_note_on(bool mute, MidiEventBuilder *_meb) {
	while(next_note_to_play != NULL && (next_note_to_play->on_at == loop_position)) {
		NoteEntry *n = next_note_to_play;

		if(is_playing && (!mute) && activate_note(n)) {
			_meb->queue_note_on(n->note, n->velocity, n->channel);
			n->ticks2off = n->length + 1;
		}
		// then move on to the next in queue
		next_note_to_play = n->next;
	}
}

void MachineSequencer::Loop::process_note_off(MidiEventBuilder *_meb) {
	for(int k = 0; k < MAX_ACTIVE_NOTES; k++) {
		if(active_note[k] != NULL) {
			NoteEntry *n = active_note[k];
			if(n->ticks2off > 0) {
				n->ticks2off--;
			} else {
				deactivate_note(n);
				_meb->queue_note_off(n->note, 0x80, n->channel);
			}
		}
	}
}

void MachineSequencer::Loop::process(bool mute, MidiEventBuilder *_meb) {
	process_note_on(mute, _meb);
	process_note_off(_meb);

	loop_position++;
}

const MachineSequencer::NoteEntry *MachineSequencer::Loop::notes_get() const {
	return note;
}

MachineSequencer::NoteEntry *MachineSequencer::Loop::internal_delete_note(const NoteEntry *net) {
	NoteEntry *crnt = note;
	while(crnt != NULL) {
		if(crnt == net) {
			if(crnt->prev != NULL) {
				crnt->prev->next = crnt->next;
			}
			if(crnt->next != NULL) {
				crnt->next->prev = crnt->prev;
			}
			if(crnt == note)
				note = crnt->next;

			return crnt;
		}
		crnt = crnt->next;
	}
	return NULL;
}

void MachineSequencer::Loop::note_delete(const NoteEntry *net) {
	typedef struct {
		MachineSequencer::Loop *thiz;
		const MachineSequencer::NoteEntry *n;
		MachineSequencer::NoteEntry *deletable;
	} Param;
	Param param = {
		.thiz = this,
		.n = net,
		.deletable = NULL
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->deletable = p->thiz->internal_delete_note(p->n);
		},
		&param, true);
	if(param.deletable) delete param.deletable;
}

void MachineSequencer::Loop::internal_update_note(
	const NoteEntry *original, const NoteEntry *new_entry) {

	NoteEntry *crnt = note;
	while(crnt != NULL) {
		if(crnt == original) {
			crnt->set_to(new_entry);
			return;
		}
		crnt = crnt->next;
	}
}

void MachineSequencer::Loop::note_update(const MachineSequencer::NoteEntry *original,
					 const MachineSequencer::NoteEntry *new_entry) {
	typedef struct {
		MachineSequencer::Loop *thiz;
		const MachineSequencer::NoteEntry *org;
		const MachineSequencer::NoteEntry *trg;
	} Param;
	Param param = {
		.thiz = this,
		.org = original,
		.trg = new_entry
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->thiz->internal_update_note(p->org, p->trg);
		},
		&param, true);
}

const MachineSequencer::NoteEntry *MachineSequencer::Loop::internal_insert_note(
	MachineSequencer::NoteEntry *new_one) {

	NoteEntry *crnt = note;
	while(crnt != NULL) {
		SATAN_DEBUG("internal_insert_note() - crnt: %p\n", crnt);
		SATAN_DEBUG("internal_insert_note() - new_one: %p\n", new_one);
		if(crnt->on_at > new_one->on_at) {
			new_one->prev = crnt->prev;
			new_one->next = crnt;

			if(crnt->prev) {
				crnt->prev->next = new_one;
			} else { // must be the first one, then
				note = new_one;
			}
			crnt->prev = new_one;

			return new_one;
		}

		if(crnt->next == NULL) {
			// end of the line...
			crnt->next = new_one;
			new_one->prev = crnt;
			new_one->next = NULL;
			return new_one;
		}

		crnt = crnt->next;
	}

	// reached only if no notes entries exist
	note = new_one;
	note->next = NULL;
	note->prev = NULL;

	return new_one;
}

const MachineSequencer::NoteEntry *MachineSequencer::Loop::note_insert(const MachineSequencer::NoteEntry *__new_entry) {
	typedef struct {
		MachineSequencer::Loop *thiz;
		MachineSequencer::NoteEntry *new_entry;
		const MachineSequencer::NoteEntry *retval;
	} Param;
	Param param = {
		.thiz = this,
		.new_entry = new NoteEntry(__new_entry),
		.retval = NULL
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->retval = p->thiz->internal_insert_note(p->new_entry);
		},
		&param, true);

	return param.retval;
}

MachineSequencer::NoteEntry *MachineSequencer::Loop::internal_clear() {
	NoteEntry *notes_to_delete = note;
	note = NULL;
	return notes_to_delete;
}

void MachineSequencer::Loop::clear_loop() {
	typedef struct {
		MachineSequencer::Loop *thiz;
		NoteEntry *notes_to_delete;
	} Param;
	Param param = {
		.thiz = this,
		.notes_to_delete = NULL
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->notes_to_delete = p->thiz->internal_clear();
		},
		&param, true);

	while(param.notes_to_delete) {
		NoteEntry *delete_me = param.notes_to_delete;
		param.notes_to_delete = param.notes_to_delete->next;
		delete delete_me;
	}
}

MachineSequencer::NoteEntry *MachineSequencer::Loop::internal_insert_notes(MachineSequencer::NoteEntry *new_notes) {
	NoteEntry *notes_to_delete = internal_clear();

	NoteEntry *tmp = new_notes;
	while(tmp != NULL) {
		NoteEntry *crnt = tmp;
		tmp = tmp->next;
		(void) internal_insert_note(crnt);
	}
	return notes_to_delete;
}

void MachineSequencer::Loop::copy_loop(const MachineSequencer::Loop *src) {
	typedef struct {
		MachineSequencer::Loop *thiz;
		MachineSequencer::NoteEntry *new_notes;
		NoteEntry *notes_to_delete;
	} Param;
	Param param = {
		.thiz = this,
		.new_notes = NULL,
		.notes_to_delete = NULL
	};

	const NoteEntry *original = src->notes_get();
	NoteEntry *new_ones = NULL, *clone = NULL;
	while(original) {
		if(clone == NULL) {
			new_ones = clone = new NoteEntry(original);
			clone->prev = clone->next = NULL;
		} else {
			clone->next = new NoteEntry(original);
			clone->next->prev = clone;
			clone = clone->next;
			clone->next = NULL;
		}
		original = original->next;
	}
	param.new_notes = new_ones;

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->notes_to_delete = p->thiz->internal_insert_notes(p->new_notes);
		},
		&param, true);

	while(param.notes_to_delete) {
		NoteEntry *delete_me = param.notes_to_delete;
		param.notes_to_delete = param.notes_to_delete->next;
		delete delete_me;
	}
}

/*************************************
 *
 * Class MachineSequencer - Public Inheritance stuff
 *
 *************************************/

std::vector<std::string> MachineSequencer::internal_get_controller_groups() {
	std::vector<std::string> retval;
	retval.clear();
	return retval;
}

std::vector<std::string> MachineSequencer::internal_get_controller_names() {
	std::vector<std::string> retval;
	retval.clear();
	return retval;
}

std::vector<std::string> MachineSequencer::internal_get_controller_names(const std::string &group_name) {
	std::vector<std::string> retval;
	retval.clear();
	return retval;
}

Machine::Controller *MachineSequencer::internal_get_controller(const std::string &name) {
	throw jException("No such controller.", jException::sanity_error);
}

std::string MachineSequencer::internal_get_hint() {
	return "sequencer";
}

/*************************************
 *
 * Class MachineSequencer - Protected Inheritance stuff
 *
 *************************************/

bool MachineSequencer::detach_and_destroy() {
	detach_all_inputs();
	detach_all_outputs();

	return true;
}

std::string MachineSequencer::get_class_name() {
	return "MachineSequencer";
}

std::string MachineSequencer::get_descriptive_xml() {
	std::ostringstream stream;

	if(sibling_name == "") throw jException("MachineSequencer created without sibling, failure.\n",
						     jException::sanity_error);

	stream << "<siblingmachine name=\"" << sibling_name << "\" />\n";

	get_loops_xml(stream);
	get_loop_sequence_xml(stream);
	pad.get_pad_xml(stream);

	// write controller envelopes to the xml stream
	std::map<std::string, ControllerEnvelope *>::iterator k;
	for(k  = controller_envelope.begin();
	    k != controller_envelope.end();
	    k++) {
		(*k).second->write_to_xml((*k).first, stream);
	}
	return stream.str();
}

/*************************************
 *
 * Class MachineSequencer - Private object functions
 *
 *************************************/

int MachineSequencer::internal_get_loop_id_at(int position) {
	if(!(position >= 0 && position < loop_sequence_length)) {
		return NOTE_NOT_SET;
	}

	return loop_sequence[position];
}

void MachineSequencer::internal_get_loop_ids_at(int *position_vector, int length) {
	int k;
	for(k = 0; k < length; k++) {
		position_vector[k] = internal_get_loop_id_at(position_vector[k]);
	}
}

void MachineSequencer::internal_set_loop_id_at(int position, int loop_id) {
	if(position < 0) throw ParameterOutOfSpec();

	if(!(position >= 0 && position < loop_sequence_length)) {
		try {
			set_sequence_length(position + 16);
		} catch(...) {
			throw ParameterOutOfSpec();
		}
		refresh_length();
	}
	if((loop_id != NOTE_NOT_SET) &&
	   (loop_id < 0 || loop_id >= loop_store_size || loop_store[loop_id] == NULL)
		) {
		throw ParameterOutOfSpec();
	}

	loop_sequence[position] = loop_id;
	inform_registered_callbacks_async();
}

int MachineSequencer::internal_add_new_loop() {
	// don't allow more than 99 loops... (current physical limit of pncsequencer.cc GUI design..)
	if(last_free_loop_store_position == 100) throw NoFreeLoopsAvailable();

	Loop *new_loop = LoopCreator::create();

	int k;
	for(k = 0; k < loop_store_size; k++) {
		if(loop_store[k] == NULL) {
			loop_store[k] = new_loop;
			last_free_loop_store_position = k + 1;
			SATAN_DEBUG("New loop added - nr loops: %d\n", last_free_loop_store_position);
			return k;
		}
	}
	try {
		double_loop_store();
	} catch(...) {
		delete new_loop;
		throw;
	}

	loop_store[k] = new_loop;
	last_free_loop_store_position = k + 1;
	SATAN_DEBUG("New loop added (store doubled) - nr loops: %d\n", last_free_loop_store_position);
	return k;
}

void MachineSequencer::prep_empty_loop_sequence() {
	loop_sequence_length = sequence_length;

	loop_sequence = (int *)malloc(sizeof(int) * loop_sequence_length);
	if(loop_sequence == NULL) {
		free(loop_store);
		throw jException("Failed to allocate memory for loop sequence.",
				 jException::syscall_error);
	}

	SATAN_DEBUG(" allocated seq at [%p], length: %d\n",
		    loop_sequence, loop_sequence_length);

	// init sequence to NOTE_NOT_SET
	int k;
	for(k = 0; k < loop_sequence_length; k++) {
		loop_sequence[k] = NOTE_NOT_SET;
	}
}

void MachineSequencer::get_loops_xml(std::ostringstream &stream) {
	int k;

	for(k = 0; k < loop_store_size; k++) {
		if(loop_store[k] != NULL) {
			loop_store[k]->get_loop_xml(stream, k);
		}
	}
}

void MachineSequencer::get_loop_sequence_xml(std::ostringstream &stream) {
	int k;

	stream << "<sequence>\n";
	for(k = 0; k < loop_sequence_length; k++) {
		int loopid = internal_get_loop_id_at(k);

		if(loopid != NOTE_NOT_SET)
			stream << "<entry pos=\"" << k << "\" id=\"" << loopid << "\" />\n";
	}
	stream << "</sequence>\n";
}

void MachineSequencer::get_midi_controllers(const std::string &name, int &coarse, int &fine) {
	Machine::Controller *c = NULL;
	coarse = -1;
	fine = -1;

	try {

		Machine *target = Machine::internal_get_by_name(sibling_name);
		c = target->internal_get_controller(name);

		if(c) {
			// we do not need the return value here, we can assume that
			// if the controller has a midi equivalent the coarse and fine integers will
			// be set accordingly. If the controller does NOT have a midi equivalent, the values
			// will both remain -1...
			(void) c->has_midi_controller(coarse, fine);

			delete c; // remember to delete c when done, otherwise we get a mem leak.. (we do not need it since we will use MIDI to control it)
		}

	} catch(...) {
		// before rethrowing, delete c if needed
		if(c) delete c;
		throw;
	}
}

std::set<std::string> MachineSequencer::internal_available_midi_controllers() {
	std::set<std::string> retval;

	Machine *target = Machine::internal_get_by_name(sibling_name);

	for(auto k : target->internal_get_controller_names()) {
		Machine::Controller *c = target->internal_get_controller(k);
		int coarse = -1, fine = -1;

		if(c->has_midi_controller(coarse, fine)) {
			retval.insert(k);
		}

		delete c; // remember to delete c when done, otherwise we get a mem leak..
	}

	return retval;
}

void MachineSequencer::create_controller_envelopes() {
	std::set<std::string>::iterator k;
	std::set<std::string> ctrs = internal_available_midi_controllers();
	for(k = ctrs.begin(); k != ctrs.end(); k++) {

		// only create ControllerEnvelope objects if non existing.
		std::map<std::string, ControllerEnvelope *>::iterator l;
		if((l = controller_envelope.find((*k))) == controller_envelope.end()) {

			ControllerEnvelope *nc = new ControllerEnvelope();
			controller_envelope[(*k)] = nc;
			get_midi_controllers((*k), nc->controller_coarse, nc->controller_fine);

		}
	}
}

void MachineSequencer::process_controller_envelopes(int tick, MidiEventBuilder *_meb) {

	std::map<std::string, ControllerEnvelope *>::iterator k;
	for(k  = controller_envelope.begin();
	    k != controller_envelope.end();
	    k++) {
		(*k).second->process_envelope(tick, _meb);
	}
}

void MachineSequencer::fill_buffers() {
	// get output signal buffer and clear it.
	Signal *out_sig = output[MACHINE_SEQUENCER_MIDI_OUTPUT_NAME];

	int output_limit = out_sig->get_samples();
	void **output_buffer = (void **)out_sig->get_buffer();

	memset(output_buffer,
	       0,
	       sizeof(void *) * output_limit
		);

	// synch to click track
	int sequence_position = get_next_sequence_position();
	int current_tick = get_next_tick();
	int samples_per_tick = get_samples_per_tick(_MIDI);
	bool do_loop = get_loop_state();
	int loop_start = get_loop_start();
	int loop_stop = get_loop_length() + loop_start;
	int samples_per_tick_shuffle = get_samples_per_tick_shuffle(_MIDI);
	int skip_length = get_next_tick_at(_MIDI);

	_meb.use_buffer(output_buffer, output_limit);

	bool no_sound = is_playing ? mute : true;

	while(_meb.skip(skip_length)) {

		pad.process(no_sound, PAD_TIME(sequence_position, current_tick), &_meb);

		if(current_tick == 0) {
			int loop_id = internal_get_loop_id_at(sequence_position);

			if(loop_id != NOTE_NOT_SET) {
				current_loop = loop_store[loop_id];
				current_loop->start_to_play();
			}
		}

		if(current_loop) {
			current_loop->process(no_sound, &_meb);
		}

		process_controller_envelopes(PAD_TIME(sequence_position, current_tick), &_meb);

		current_tick = (current_tick + 1) % MACHINE_TICKS_PER_LINE;

		if(current_tick == 0) {
			sequence_position++;

			if(do_loop && sequence_position >= loop_stop) {
				sequence_position = loop_start;
			}
		}

		if(sequence_position % 2 == 0) {
			skip_length = samples_per_tick - samples_per_tick_shuffle;
		} else {
			skip_length = samples_per_tick + samples_per_tick_shuffle;
		}

	}

	_meb.finish_current_buffer();
}

void MachineSequencer::reset() {
	current_loop = NULL;
	pad.reset();
}

/*************************************
 *
 * Class MachineSequencer - public object interface
 *
 *************************************/

int MachineSequencer::get_loop_id_at(int position) {
	typedef struct {
		MachineSequencer *thiz;
		int pos;
		int retval;
	} Param;
	Param param = {
		.thiz = this,
		.pos = position
	};

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->retval = p->thiz->internal_get_loop_id_at(p->pos);
		},
		&param, true);

	return param.retval;
}

void MachineSequencer::get_loop_ids_at(int *position_vector, int length) {
	typedef struct {
		MachineSequencer *thiz;
		int *pos;
		int len;
	} Param;
	Param param = {
		.thiz = this,
		.pos = position_vector,
		.len = length
	};

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->thiz->internal_get_loop_ids_at(p->pos, p->len);
		},
		&param, true);
}

void MachineSequencer::set_loop_id_at(int position, int loop_id) {
	typedef struct {
		MachineSequencer *thiz;
		int pos, l_id;
		bool parameter_out_of_spec;
	} Param;
	Param param = {
		.thiz = this,
		.pos = position,
		.l_id = loop_id,
		.parameter_out_of_spec = false
	};

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;

			try {
				p->thiz->internal_set_loop_id_at(p->pos, p->l_id);
			} catch(ParameterOutOfSpec poos) {
				p->parameter_out_of_spec = true;
			}
		},
		&param, true);
	if(param.parameter_out_of_spec)
		throw ParameterOutOfSpec();
}

MachineSequencer::Loop *MachineSequencer::get_loop(int loop_id) {
	typedef struct {
		MachineSequencer *thiz;
		int lid;
		MachineSequencer::Loop *retval;
	} Param;
	Param param = {
		.thiz = this,
		.lid = loop_id
	};

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;

			if(!(p->lid >= 0 && p->lid < p->thiz->loop_store_size)) {
				p->retval = NULL;
			} else
				p->retval = p->thiz->loop_store[p->lid];
		},
		&param, true);

	if(param.retval == NULL) {
		// do not return NULL pointers, if user tries to get an unallocated loop
		throw NoSuchLoop();
	}

	return param.retval;
}

int MachineSequencer::get_nr_of_loops() {
	return last_free_loop_store_position;
}

int MachineSequencer::add_new_loop() {
	typedef struct {
		MachineSequencer *thiz;
		int retval;
	} Param;
	Param param = {
		.thiz = this,
		.retval = -1
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			try {
				p->retval = p->thiz->internal_add_new_loop();
			} catch(NoFreeLoopsAvailable nfla) {
				p->retval = -1;
			}
		},
		&param, true);

	if(param.retval == -1)
		throw NoFreeLoopsAvailable();

	return param.retval;
}

void MachineSequencer::delete_loop(int id) {
	typedef struct {
		MachineSequencer *thiz;
		int _id;
	} Param;
	Param param = {
		.thiz = this,
		._id = id
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;

			int k;
			for(k = 0; k < p->thiz->loop_sequence_length; k++) {
				if(p->thiz->loop_sequence[k] == p->_id)
					p->thiz->loop_sequence[k] = NOTE_NOT_SET;

				if(p->thiz->loop_sequence[k] > p->_id)
					p->thiz->loop_sequence[k] = p->thiz->loop_sequence[k] - 1;
			}

			for(k = p->_id; k < p->thiz->loop_store_size - 1; k++) {
				p->thiz->loop_store[k] = p->thiz->loop_store[k + 1];
			}
			p->thiz->last_free_loop_store_position--;

		},
		&param, true);
}

bool MachineSequencer::is_mute() {
	return mute;
}

void MachineSequencer::set_mute(bool m) {
	mute = m;
}

std::set<std::string> MachineSequencer::available_midi_controllers() {
	typedef struct {
		MachineSequencer *thiz;
		std::set<std::string> retval;
	} Param;
	Param param = {
		.thiz = this
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->retval = p->thiz->internal_available_midi_controllers();
		},
		&param, true);

	return param.retval;
}

void MachineSequencer::assign_pad_to_midi_controller(Pad::PadConfiguration::PadAxis p_axis,
						     const std::string &name) {
	Machine::machine_operation_enqueue(
		[p_axis, this, name] (void *) {
			int pad_controller_coarse = -1;
			int pad_controller_fine = -1;
			try {
				get_midi_controllers(name, pad_controller_coarse, pad_controller_fine);
			} catch(...) {
				/* ignore fault here */
			}
			SATAN_DEBUG("assign_pad_to_midi_controller() : c(%d), f(%d)\n",
				    pad_controller_coarse,
				    pad_controller_fine);
			pad.config.set_coarse_controller(
				p_axis == Pad::PadConfiguration::pad_y_axis ? 0 : 1, pad_controller_coarse);
			pad.config.set_fine_controller(
				p_axis == Pad::PadConfiguration::pad_y_axis ? 0 : 1, pad_controller_fine);
		},
		NULL, true);
}

Pad* MachineSequencer::get_pad() {
	return &pad;
}

static std::vector<std::string> arpeggio_pattern_id_list;
static bool arpeggio_pattern_id_list_ready = false;
static void build_arpeggio_pattern_id_list() {
	if(arpeggio_pattern_id_list_ready) return;
	arpeggio_pattern_id_list_ready = true;

	for(int k = 0; k < MAX_BUILTIN_ARP_PATTERNS; k++) {
		std::stringstream bi_name;
		bi_name << "built-in #" << k;
		arpeggio_pattern_id_list.push_back(bi_name.str());
	}
}

void MachineSequencer::set_pad_arpeggio_pattern(const std::string identity) {
	build_arpeggio_pattern_id_list();

	int arp_pattern_index = -1;
	for(unsigned int k = 0; k < arpeggio_pattern_id_list.size(); k++) {
		if(arpeggio_pattern_id_list[k] == identity) {
			arp_pattern_index = k;
		}
	}

	machine_operation_enqueue(
		[this, arp_pattern_index] (void *d) {
			if(arp_pattern_index == -1) {
				pad.config.set_arpeggio_direction(Pad::PadConfiguration::arp_off);
			}

			pad.config.set_arpeggio_pattern(arp_pattern_index);
		},
		NULL, true);
}

void MachineSequencer::set_pad_arpeggio_direction(Pad::PadConfiguration::ArpeggioDirection arp_direction) {
	machine_operation_enqueue(
		[this, arp_direction] (void *) {
			SATAN_DEBUG("Arpeggio direction selected (%p): %d\n", &(pad), arp_direction);
			pad.config.set_arpeggio_direction(arp_direction);
		},
		NULL, true);
}

void MachineSequencer::set_pad_chord_mode(Pad::PadConfiguration::ChordMode pconf) {
	machine_operation_enqueue(
		[this, pconf] (void *) {
			SATAN_DEBUG("Chord selected (%p): %d\n", &(pad), pconf);
			pad.config.set_chord_mode(pconf);
		},
		NULL, true);
}

void MachineSequencer::export_pad_to_loop(int loop_id) {
	if(loop_id == LOOP_NOT_SET)
		loop_id = add_new_loop();

	SATAN_DEBUG("Will export to loop number %d\n", loop_id);
	Loop *loop = get_loop(loop_id);

	loop->clear_loop();

	typedef struct {
		MachineSequencer *thiz;
		Loop *l;
	} Param;
	Param param = {
		.thiz = this,
		.l = loop
	};

	machine_operation_enqueue(
		[] (void *d) {

			int start_tick = 0;
			int stop_tick = -1;

			if(Machine::get_loop_state()) {
				start_tick = Machine::get_loop_start() * MACHINE_TICKS_PER_LINE;
				stop_tick = (Machine::get_loop_start() + Machine::get_loop_length()) * MACHINE_TICKS_PER_LINE;
			}

			Param *p = (Param *)d;

			PadMidiExportBuilder<NoteEntry, Loop> pmxb(
				p->l, start_tick,
				[](NoteEntry* nptr, Loop* lp) {
					lp->internal_insert_note(nptr);
				}
				);

			try {
				p->thiz->pad.export_to_loop(start_tick, stop_tick, pmxb);
			} catch(...) {
				SATAN_ERROR("MachineSequencer::export_to_loop() - Exception caught.\n");
				throw;
			}
		},
		&param, true);
}

std::vector<std::string> MachineSequencer::get_pad_arpeggio_patterns() {
	build_arpeggio_pattern_id_list();
	return arpeggio_pattern_id_list;
}

Machine *MachineSequencer::get_machine() {
	typedef struct {
		MachineSequencer *thiz;
		Machine *m;
		std::function<Machine *(const std::string &name)> gbn;
	} Param;
	Param param = {
		.thiz = this
	};
	param.gbn = Machine::internal_get_by_name;

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			p->m = p->gbn(p->thiz->sibling_name);
		},
		&param, true);

	return param.m;
}

const ControllerEnvelope *MachineSequencer::get_controller_envelope(const std::string &name) const {
	typedef struct {
		const MachineSequencer *thiz;
		const std::string &n;
		const ControllerEnvelope *retval;
	} Param;
	Param param = {
		.thiz = this,
		.n = name,
		.retval = NULL
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			if(p->thiz->controller_envelope.find(p->n) != p->thiz->controller_envelope.end()) {
				p->retval = (*(p->thiz->controller_envelope.find(p->n))).second;
			}
		},
		&param, true);

	if(param.retval == NULL)
		throw NoSuchController(name);

	return param.retval;
}

void MachineSequencer::update_controller_envelope(
	const ControllerEnvelope *original, const ControllerEnvelope *new_entry) {

	typedef struct {
		MachineSequencer *thiz;
		const ControllerEnvelope *orig;
		const ControllerEnvelope *newe;
	} Param;
	Param param = {
		.thiz = this,
		.orig = original,
		.newe = new_entry
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;

			std::map<std::string, ControllerEnvelope *>::iterator k;
			for(k  = p->thiz->controller_envelope.begin();
			    k != p->thiz->controller_envelope.end();
			    k++) {
				if((*k).second == p->orig) {
					(*k).second->set_to(p->newe);
					return;
				}
			}
		},
		&param, true);
}

void MachineSequencer::enqueue_midi_data(size_t len, const char* data) {
	char* data_copy = (char*)malloc(len);
	if(!data_copy) return;
	memcpy(data_copy, data, len);

	Machine::machine_operation_enqueue(
		[this, len, data_copy] (void *) {

			_meb.queue_midi_data(len, data_copy);
			free(data_copy);

		}, NULL, false);
}

/*************************************
 *
 * Class MachineSequencer - NON static stuff
 *
 *************************************/

MachineSequencer::~MachineSequencer() {
	{ // unmap machine2sequencer
		std::map<Machine *, MachineSequencer *>::iterator k;

		for(k = machine2sequencer.begin();
		    k != machine2sequencer.end();
		    k++) {
			if((*k).second == this) {
				machine2sequencer.erase(k);
				break;
			}
		}
	}

	{ // erase all ControllerEnvelope objects
		std::map<std::string, ControllerEnvelope *>::iterator k;
		for(k  = controller_envelope.begin();
		    k != controller_envelope.end();
		    k++) {
			delete ((*k).second);
		}
	}

	inform_registered_callbacks_async();
}

MachineSequencer::MachineSequencer(int project_interface_level, const KXMLDoc &machine_xml, const std::string &name) :
	//
	//  project_interface_level  < 5 == Pad uses old absolute coordinates
	//                          => 5 == Pad uses new relative coordinates
	//
	Machine(name, true, 0.0f, 0.0f),
	current_loop(NULL),
	mute(false),
	sibling_name(""),
	loop_store(NULL),
	loop_store_size(0),
	last_free_loop_store_position(0x0000),
	loop_sequence(NULL),
	loop_sequence_length(0)
{
	// "c" for "counter", used to count through xml nodes..
	int c, c_max = 0;

	output_descriptor[MACHINE_SEQUENCER_MIDI_OUTPUT_NAME] =
		Signal::Description(_MIDI, 1, false);
	setup_machine();

	pad.load_pad_from_xml(project_interface_level, machine_xml);

	sibling_name = machine_xml["siblingmachine"].get_attr("name");

	/**********************
	 *
	 *  Read in loops
	 *
	 */

	// get number of loop entries in XML
	try {
		c_max = machine_xml["loop"].get_count();
	} catch(...) {
		// ignore
	}

	// get maximum loop id, so we can set up loop store correctly
	int max_loop_id = -1;
	for(c = 0; c < c_max; c++) {
		KXMLDoc loop_xml = machine_xml["loop"][c];
		int loop_id;
		loop_id = -1;
		KXML_GET_NUMBER(loop_xml,"id",loop_id,-1);
		if(loop_id > max_loop_id) max_loop_id = loop_id;
	}

	// allocate loop store and clear it
	loop_store_size = MACHINE_SEQUENCER_INITIAL_SEQUENCE_LENGTH > max_loop_id ?
		MACHINE_SEQUENCER_INITIAL_SEQUENCE_LENGTH : max_loop_id;
	loop_store = (Loop **)malloc(sizeof(Loop *) * loop_store_size);
	if(loop_store == NULL)
		throw jException("Failed to allocate memory for initial loop store.",
				 jException::syscall_error);
	memset(loop_store, 0, sizeof(Loop *) * loop_store_size);

	// then we read the actual loops here
	for(c = 0; c < c_max; c++) {
		KXMLDoc loop_xml = machine_xml["loop"][c];

		int loop_id;
		loop_id = -1;
		KXML_GET_NUMBER(loop_xml,"id",loop_id,-1);

		if(loop_id != -1) {
			Loop *new_loop = LoopCreator::create(loop_xml);
			loop_store[loop_id] = new_loop;
			if(loop_id > last_free_loop_store_position)
				last_free_loop_store_position = loop_id;
		}
	}
	last_free_loop_store_position++;

	/*****************
	 *
	 * OK - loops are done, read the sequence
	 *
	 */
	KXMLDoc sequence_xml = machine_xml["sequence"];
	// get number of sequence entries in XML
	try {
		c_max = sequence_xml["entry"].get_count();
	} catch(...) {
		c_max = 0; // no "entry" nodes available
	}

	SATAN_DEBUG_("---------------------------------------------\n");
	SATAN_DEBUG("  READING SEQ FOR [%s]\n", name.c_str());

	prep_empty_loop_sequence();

	for(c = 0; c < c_max; c++) {
		KXMLDoc s_entry_xml = sequence_xml["entry"][c];

		int pos, id;
		KXML_GET_NUMBER(s_entry_xml,"pos",pos,-1);
		KXML_GET_NUMBER(s_entry_xml,"id",id,-1);

		if(pos != -1 && id != -1)
			internal_set_loop_id_at(pos, id);

		SATAN_DEBUG("    SEQ[%d] = %d\n", pos, id);
	}

	SATAN_DEBUG("  SEQ WAS FOR [%s]\n", name.c_str());
	SATAN_DEBUG_("---------------------------------------------\n");

	/*********************
	 *
	 * Time to read the controller envelopes
	 *
	 */
	// get number of envelope entries in XML
	try {
		c_max = machine_xml["envelope"].get_count();
	} catch(...) {
		c_max = 0; // no "entry" nodes available
	}

	for(c = 0; c < c_max; c++) {
		KXMLDoc e_entry_xml = machine_xml["envelope"][c];

		{
			ControllerEnvelope *nc = new ControllerEnvelope();
			try {
				std::string env_id = nc->read_from_xml(e_entry_xml);

				{ // if there is already a controller envelope object, delete it and remove the entry
					std::map<std::string, ControllerEnvelope *>::iterator k;
					k = controller_envelope.find(env_id);
					if(k != controller_envelope.end() &&
					   (*k).second != NULL) {
						delete (*k).second;
						controller_envelope.erase(k);
					}
				}
				// insert new entry
				controller_envelope[env_id] = nc;
			} catch(...) {
				delete nc;
				throw;
			}
		}
	}
}

MachineSequencer::MachineSequencer(const std::string &name_root) :
	Machine(name_root + "___MACHINE_SEQUENCER", true, 0.0f, 0.0f),
	current_loop(NULL),
	mute(false),
	sibling_name(name_root),
	loop_store(NULL),
	loop_store_size(0),
	last_free_loop_store_position(0x0000),
	loop_sequence(NULL),
	loop_sequence_length(0)
{
	// create the controller envelopes
	create_controller_envelopes();

	output_descriptor[MACHINE_SEQUENCER_MIDI_OUTPUT_NAME] =
		Signal::Description(_MIDI, 1, false);
	setup_machine();

	loop_store_size = MACHINE_SEQUENCER_INITIAL_SEQUENCE_LENGTH;
	loop_store = (Loop **)malloc(sizeof(Loop *) * loop_store_size);
	if(loop_store == NULL)
		throw jException("Failed to allocate memory for initial loop store.",
				 jException::syscall_error);
	memset(loop_store, 0, sizeof(Loop *) * loop_store_size);

	prep_empty_loop_sequence();

	// Create a loop to start
	internal_add_new_loop();
}

void MachineSequencer::double_loop_store() {
	int new_size = loop_store_size * 2;
	Loop **new_store = (Loop **)malloc(sizeof(Loop *) * new_size);

	if(new_store == NULL)
		throw jException("Failed to allocate memory for doubled loop store.",
				 jException::syscall_error);

	memset(new_store, 0, sizeof(Loop *) * new_size);

	int k;
	for(k = 0; k < loop_store_size; k++)
		new_store[k] = loop_store[k];

	free(loop_store);
	loop_store = new_store;
	loop_store_size = new_size;
}

void MachineSequencer::refresh_length() {
	if(loop_sequence_length >= sequence_length) {
		return; // no need to change
	}

	int new_size = sequence_length;

	int *new_seq = (int *)malloc(sizeof(int) * new_size);

	int
		k,
		mid_term = new_size < loop_sequence_length ? new_size : loop_sequence_length;
	for(k = 0;
	    k < mid_term;
	    k++) {
		new_seq[k] = loop_sequence[k];
	}
	for(k = mid_term;
	    k < new_size;
	    k++) {
		new_seq[k] = NOTE_NOT_SET;
	}

	if(loop_sequence != NULL) free(loop_sequence);

	loop_sequence = new_seq;
	loop_sequence_length = new_size;
}

void MachineSequencer::write_MIDI_info_track(MidiExport *target) {
	// create the midi chunk
	MidiExport::Chunk *midi_chunk = target->append_new_chunk("MTrk");

	uint32_t msPerQuarterNote;

	msPerQuarterNote = 60000000 / Machine::get_bpm();

	// push "Copyright notice" meta event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x02);
	midi_chunk->push_string("Copyrighted Material");

	// push "Cue point" meta event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x07);
	midi_chunk->push_string("Created by SATAN for Android");

	// push "Set Tempo" meta event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x51);
	midi_chunk->push_u32b_word_var_len(3);
	midi_chunk->push_u8b_word((msPerQuarterNote & 0xff0000) >> 16);
	midi_chunk->push_u8b_word((msPerQuarterNote & 0x00ff00) >>  8);
	midi_chunk->push_u8b_word((msPerQuarterNote & 0x0000ff) >>  0);

	// push "Time Signature" meta event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x58);
	midi_chunk->push_u32b_word_var_len(4);
	midi_chunk->push_u8b_word(0x02);
	midi_chunk->push_u8b_word(0x04);
	midi_chunk->push_u8b_word(0x24);
	midi_chunk->push_u8b_word(0x08);

	// push "end of track" event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x2f);
	midi_chunk->push_u32b_word_var_len(0);
}

// returns the nr of loops exported (= midi tracks)
int MachineSequencer::export_loops2midi_file(MidiExport *target) {
	return 0;
}

// will export entire sequence as one track
bool MachineSequencer::export_sequence2midi_file(MidiExport *target) {
	bool was_dropped = true; // assume most sequences will be dropped from the final midi file

	// create the midi chunk
	MidiExport::Chunk *midi_chunk = target->append_new_chunk("MTrk");

	// push "Track Name" meta event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x03);
	midi_chunk->push_string(sibling_name);

	/*************
	 *
	 *  Generate note related midi data
	 *
	 *************/
	Loop *l = NULL; // loop currently being exported

	int s_p = 0, l_s = 0, l_p = 0; // sequence position, loop sequence position, current loop position
	int last_p = 0;
	// keep track of the off position which is last
	int highest_off_p = -1;

	const NoteEntry *ndata = NULL; // note data

	std::map<int, NoteEntry *> notes_to_off; // currently active notes that we should stop (integer value is at which value of s_p when we should stop it)

	for(s_p = 0; s_p < loop_sequence_length || s_p < highest_off_p; s_p++, l_p++) {
		if(s_p < loop_sequence_length && loop_sequence[s_p] != NOTE_NOT_SET) {
			l_p = 0;
			l_s = s_p;
			l = loop_store[loop_sequence[s_p]];
			ndata = l->notes_get();
		}

		if(ndata != NULL) {
			while(ndata != NULL && ndata->on_at <= PAD_TIME(l_p, 0)) {
				NoteEntry *new_one = new NoteEntry(ndata);
				int off_position = l_s + PAD_TIME_LINE(ndata->on_at) + ndata->length + 1;

				SATAN_DEBUG("MEXPORT: on: %d, len: %d\n",
					    ndata->on_at, ndata->length);

				if(off_position > highest_off_p)
					highest_off_p = off_position;

				/***
				 * first we handle the book keeping
				 ***/
				std::map<int, NoteEntry *>::iterator k;

				k = notes_to_off.find(off_position);

				// if others exist on this off position, chain them to the new one first
				if(k != notes_to_off.end()) {
					new_one->next = (*k).second;
				}

				notes_to_off[off_position] = new_one;

				ndata = ndata->next;

				/***
				 * then we generate the MIDI export data, a note on message
				 ***/
				was_dropped = false; // indicate we wrote useful data to this track.
				uint32_t offset = (s_p - last_p) * MACHINE_TICKS_PER_LINE;
				last_p = s_p;
				midi_chunk->push_u32b_word_var_len(offset);
				midi_chunk->push_u8b_word(0x90 | (0x0f & new_one->channel));
				midi_chunk->push_u8b_word(new_one->note);
				midi_chunk->push_u8b_word(new_one->velocity);

			}
		}

		std::map<int, NoteEntry *>::iterator current_to_off = notes_to_off.find(s_p);
		if(current_to_off != notes_to_off.end()) {
			NoteEntry *tip = (*current_to_off).second;
			notes_to_off.erase(current_to_off);

			while(tip != NULL) {
				uint32_t offset = (s_p - last_p) * MACHINE_TICKS_PER_LINE;
				last_p = s_p;
				midi_chunk->push_u32b_word_var_len(offset);
				midi_chunk->push_u8b_word(0x80 | (0x0f & tip->channel));
				midi_chunk->push_u8b_word(tip->note);
				midi_chunk->push_u8b_word(tip->velocity);

				NoteEntry *deletable_off = tip;
				tip = tip->next; delete deletable_off;
			}
		}
	}

	// push "end of track" event
	midi_chunk->push_u32b_word_var_len(0);
	midi_chunk->push_u8b_word(0xff);
	midi_chunk->push_u8b_word(0x2f);
	midi_chunk->push_u32b_word_var_len(0);

	if(was_dropped) midi_chunk->drop();

	return !was_dropped;
}

/*************************************
 *
 * Class MachineSequencer - STATIC stuff
 *
 *************************************/

std::vector<MachineSequencer *> MachineSequencer::machine_from_xml_cache;
std::map<Machine *, MachineSequencer *> MachineSequencer::machine2sequencer;
int MachineSequencer::sequence_length = MACHINE_SEQUENCER_INITIAL_SEQUENCE_LENGTH;
std::vector<MachineSequencerSetChangeCallbackF_t> MachineSequencer::set_change_callback;

void MachineSequencer::inform_registered_callbacks_async() {
	if(Machine::internal_get_load_state()) return; // if in loading state, don't bother with this.

	std::vector<MachineSequencerSetChangeCallbackF_t>::iterator k;
	for(k  = set_change_callback.begin();
	    k != set_change_callback.end();
	    k++) {
		auto f = (*k);
		run_async_function(
			[f]() {
				f(NULL);
			}
			);
	}
}

void MachineSequencer::set_sequence_length(int newv) {
	sequence_length = newv;

	{
		std::map<Machine *, MachineSequencer *>::iterator k;

		for(k =  machine2sequencer.begin();
		    k !=  machine2sequencer.end();
		    k++) {
			(*k).second->refresh_length();
		}
	}
}

/*************************************
 *
 * Class MachineSequencer - PUBLIC STATIC stuff
 *
 *************************************/

void MachineSequencer::presetup_from_xml(int project_interface_level, const KXMLDoc &machine_xml) {
	typedef struct {
		const KXMLDoc &mxml;
		int pil;
	} Param;
	Param param = {
		.mxml = machine_xml,
		.pil = project_interface_level
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			std::string iname = p->mxml.get_attr("name"); // instance name
			MachineSequencer *result = new MachineSequencer(p->pil, p->mxml, iname);
			machine_from_xml_cache.push_back(result);
		},
		&param, true);
}

void MachineSequencer::finalize_xml_initialization() {
	typedef struct {
		std::function<Machine *(const std::string &name)> internal_get_by_name;
	} Param;
	Param param;
	param.internal_get_by_name = Machine::internal_get_by_name;

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			std::vector<MachineSequencer *>::iterator k;

			for(k  = machine_from_xml_cache.begin();
			    k != machine_from_xml_cache.end();
			    k++) {
				MachineSequencer *mseq = (*k);
				if(mseq) {
					Machine *sibling = NULL;

					sibling = p->internal_get_by_name(mseq->sibling_name);

					if(sibling == NULL)
						throw jException("Project file contains a MachineSequencer, but no matching sibling ]" +
								 mseq->sibling_name +
								 "[", jException::sanity_error);

					mseq->tightly_connect(sibling);
					machine2sequencer[sibling] = mseq;

					// create the controller envelopes
					mseq->create_controller_envelopes();
				}
			}

			// we're done now - clear it out
			machine_from_xml_cache.clear();

			// this will make sure all sequence lengths are synchronized
			set_sequence_length(sequence_length);

			inform_registered_callbacks_async();
		},
		&param, true);

}

void MachineSequencer::create_sequencer_for_machine(Machine *sibling) {
	{
		bool midi_input_found = false;

		try {
			(void) sibling->get_input_index(MACHINE_SEQUENCER_MIDI_INPUT_NAME);
			midi_input_found = true;
		} catch(...) {
			// ignore
		}

		if(!midi_input_found) return; // no midi input to attach MachineSequencer too
	}

	{
		Machine::machine_operation_enqueue(
			[] (void *d) {
				Machine *sib = (Machine *)d;
				if(machine2sequencer.find(sib) !=
				   machine2sequencer.end()) {
					SATAN_ERROR("Error. Machine already has a sequencer.\n");
					throw jException("Trying to create MachineSequencer for a machine that already has one!",
							 jException::sanity_error);
				}
			},
			sibling, true);
	}
	MachineSequencer *mseq = NULL;
	try {
		mseq = new MachineSequencer(sibling->get_name());
	} catch(const jException &e) {
		SATAN_ERROR("(A) MachineSequencer::create_sequencer_for_machine() - Caught a jException: %s\n", e.message.c_str());
		throw;
	}

	sibling->attach_input(mseq,
			      MACHINE_SEQUENCER_MIDI_OUTPUT_NAME,
			      MACHINE_SEQUENCER_MIDI_INPUT_NAME);

	{
		typedef struct {
			MachineSequencer *ms;
			Machine *sib;
		} Param;
		Param param = {
			.ms = mseq,
			.sib = sibling
		};
		Machine::machine_operation_enqueue(
			[] (void *d) {
				Param *p = (Param *)d;
				/* Make sure these machines are tightly connected
				 * so that they can not exist without the other
				 */
				p->ms->tightly_connect(p->sib);
				machine2sequencer[p->sib] = p->ms;
				inform_registered_callbacks_async();
			},
			&param, true);
	}
}

MachineSequencer *MachineSequencer::get_sequencer_for_machine(Machine *m) {
	typedef struct {
		Machine *machine;
		MachineSequencer *retval;
	} Param;
	Param param = {
		.machine = m,
		.retval = NULL
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;

			std::map<Machine *, MachineSequencer *>::iterator k;

			for(k =  machine2sequencer.begin();
			    k !=  machine2sequencer.end();
			    k++) {
				if((*k).first == p->machine) {
					p->retval = (*k).second;
				}
			}
		},
		&param, true);

	return param.retval;
}

std::map<std::string, MachineSequencer *, ltstr> MachineSequencer::get_sequencers() {
	std::map<Machine *, MachineSequencer *> m2s;

	Machine::machine_operation_enqueue(
		[] (void *d) {
			std::map<Machine *, MachineSequencer *> *_m2s =
				(std::map<Machine *, MachineSequencer *> *)d;
			*_m2s = machine2sequencer;
		},
		&m2s, true);

	std::map<std::string, MachineSequencer *, ltstr> retval;

	// rest of processing done outside the lock
	std::map<Machine *, MachineSequencer *>::iterator k;
	for(k =  m2s.begin();
	    k !=  m2s.end();
	    k++) {
		retval[(*k).first->get_name()] = (*k).second;
	}

	return retval;
}

void MachineSequencer::register_change_callback(void (*callback_f_p)(void *)) {
	typedef struct {
		void (*cfp)(void *);
	} Param;
	Param param = {
		.cfp = callback_f_p
	};
	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			set_change_callback.push_back(p->cfp);
		},
		&param, true);
}

void MachineSequencer::export2MIDI(bool _just_loops, const std::string &_output_path) {
	typedef struct {
		bool jloops;
		const std::string &opth;
	} Param;
	Param param = {
		.jloops = _just_loops,
		.opth = _output_path
	};

	Machine::machine_operation_enqueue(
		[] (void *d) {
			Param *p = (Param *)d;
			bool just_loops = p->jloops;
			const std::string &output_path = p->opth;

			MidiExport *target = new MidiExport(output_path);

			try {
				// first we create the header-chunk, since it must be first in the file
				// please note that the contents of the header chunk is created last
				// since it depends on how many tracks we will export which we do not know yet
				MidiExport::Chunk *header = target->append_new_chunk("MThd");
				uint16_t nr_of_tracks = 1;

				// write information container track
				write_MIDI_info_track(target);

				// step through all sequences / loops
				std::map<Machine *, MachineSequencer *>::iterator k;
				for(k  = machine2sequencer.begin();
				    k != machine2sequencer.end();
				    k++) {
					if(just_loops)
						nr_of_tracks +=
							(*k).second->export_loops2midi_file(target);
					else {
						if((*k).second->export_sequence2midi_file(target))
							nr_of_tracks++;
					}
				}

				// create the content of the header chunk
				header->push_u16b_word(1);
				header->push_u16b_word(nr_of_tracks);
				header->push_u16b_word(MACHINE_TICKS_PER_LINE * get_lpb());

				// OK, finalize the file (dump it to disk...)
				target->finalize_file();

				// then we delete the construct.
				delete target;
			} catch(...) {
				delete target;

				throw;
			}
		},
		&param, true);
}
