/*
 * vu|KNOB
 * Copyright (C) 2015 by Anton Persson
 *
 * http://www.vuknob.com/
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

#include "sequence.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#ifdef __RI_SERVER_SIDE
#include "machine.hh"
#endif

#ifdef __RI__SERVER_SIDE
#include "server.hh"
#endif

#define SEQUENCE_MIDI_INPUT_NAME "midi"
#define SEQUENCE_MIDI_OUTPUT_NAME "midi_OUTPUT"

SERVER_CODE(
	Sequence::Sequence(int32_t new_obj_id, const Factory *factory)
	: BaseMachine(new_obj_id, factory)
	ON_SERVER(, Machine("Sequencer", false, 0.0f, 0.0f))
	{
		ON_SERVER(
			Machine::machine_operation_enqueue(
				[this] {
					output_descriptor[SEQUENCE_MIDI_OUTPUT_NAME] =
						Signal::Description(_MIDI, 1, false);
					setup_machine();
				}
				);
			);
		register_handlers();
		SATAN_DEBUG("Sequence() created server side.\n");

	}

	void Sequence::handle_req_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		uint32_t pattern_id = std::stoul(msg.get_value("pattern_id"));
		int start_at = std::stoi(msg.get_value("start_at"));
		int loop_length = std::stoi(msg.get_value("loop_length"));
		int stop_at = std::stoi(msg.get_value("stop_at"));
		bool operation_successfull = false;
		auto length = (stop_at - start_at);

		if(start_at >= stop_at)
			return; // incorrect values
		if(loop_length > length)
			loop_length = length;

		Machine::machine_operation_enqueue(
			[this, &operation_successfull,
			 pattern_id,
			 start_at, loop_length, stop_at] {
				operation_successfull = process_add_pattern_instance(
					pattern_id, start_at, loop_length, stop_at);
			});

		if(operation_successfull) {
			// tell all clients to add it
			send_message(
				cmd_add_pattern_instance,
				[pattern_id, start_at,
				 loop_length, stop_at](std::shared_ptr<Message> &msg_to_send) {
					msg_to_send->set_value("pattern_id", std::to_string(pattern_id));
					msg_to_send->set_value("start_at", std::to_string(start_at));
					msg_to_send->set_value("loop_length", std::to_string(loop_length));
					msg_to_send->set_value("stop_at", std::to_string(stop_at));
				}
				);
		}
	}

	void Sequence::handle_req_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		PatternInstance pattern_instance = {
			.pattern_id = std::stoul(msg.get_value("pattern_id")),
			.start_at = std::stoi(msg.get_value("start_at")),
			.loop_length = std::stoi(msg.get_value("loop_length")),
			.stop_at = std::stoi(msg.get_value("stop_at")),
			.next = NULL,
		};

		Machine::machine_operation_enqueue(
			[this, &pattern_instance]() {
				process_del_pattern_instance(pattern_instance);
			});
		// tell all clients to delete it
		send_message(
			cmd_del_pattern_instance,
			[pattern_instance](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value(
					"pattern_id",
					std::to_string(pattern_instance.pattern_id));
				msg_to_send->set_value(
					"start_at",
					std::to_string(pattern_instance.start_at));
				msg_to_send->set_value(
					"loop_length",
					std::to_string(pattern_instance.loop_length));
				msg_to_send->set_value(
					"stop_at",
					std::to_string(pattern_instance.stop_at));
			}
			);
	}

	void Sequence::handle_req_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		uint32_t pattern_id = std::stoul(msg.get_value("pattern_id"));
		int channel = std::stoi(msg.get_value("channel"));
		int program = std::stoi(msg.get_value("program"));
		int velocity = std::stoi(msg.get_value("velocity"));
		int note = std::stoi(msg.get_value("note"));
		int on_at = std::stoi(msg.get_value("on_at"));
		int length = std::stoi(msg.get_value("length"));

		Machine::machine_operation_enqueue(
			[this, pattern_id, channel, program,  velocity,
			 note, on_at, length] {
				process_add_note(
					pattern_id,
					channel, program, velocity,
					note, on_at, length);
			});

		send_message(
			cmd_add_note,
			[pattern_id,
			 channel, program,
			 velocity, note,
			 on_at, length](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value(
					"pattern_id",
					std::to_string(pattern_id));
				msg_to_send->set_value(
					"channel",
					std::to_string(channel));
				msg_to_send->set_value(
					"program",
					std::to_string(program));
				msg_to_send->set_value(
					"velocity",
					std::to_string(velocity));
				msg_to_send->set_value(
					"note",
					std::to_string(note));
				msg_to_send->set_value(
					"on_at",
					std::to_string(on_at));
				msg_to_send->set_value(
					"length",
					std::to_string(length));
			}
			);
	}

	void Sequence::handle_req_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		Note note =
		{
			.channel = std::stoi(msg.get_value("channel")),
			.program = std::stoi(msg.get_value("program")),
			.velocity = std::stoi(msg.get_value("velocity")),
			.note = std::stoi(msg.get_value("note")),
			.on_at = std::stoi(msg.get_value("on_at")),
			.length = std::stoi(msg.get_value("length"))
		};
		uint32_t pattern_id = std::stoul(msg.get_value("pattern_id"));

		Machine::machine_operation_enqueue(
			[this, pattern_id, note] {
				process_delete_note(pattern_id, note);
			});

		// tell all clients to delete it
		send_message(
			cmd_del_note,
			[pattern_id, note](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value(
					"pattern_id",
					std::to_string(pattern_id));
				msg_to_send->set_value(
					"channel",
					std::to_string(note.channel));
				msg_to_send->set_value(
					"program",
					std::to_string(note.program));
				msg_to_send->set_value(
					"velocity",
					std::to_string(note.velocity));
				msg_to_send->set_value(
					"note",
					std::to_string(note.note));
				msg_to_send->set_value(
					"on_at",
					std::to_string(note.on_at));
				msg_to_send->set_value(
					"length",
					std::to_string(note.length));
			}
			);

	}

	void Sequence::handle_req_pad_set_octave(RemoteInterface::Context *context,
						 RemoteInterface::MessageHandler *src,
						 const RemoteInterface::Message& msg) {
		int octave = std::stoi(msg.get_value("octave"));
		Machine::machine_operation_enqueue(
			[this, octave]() {
				pad.config.set_octave(octave);
			}
			);
	}

	void Sequence::handle_req_pad_set_scale(RemoteInterface::Context *context,
						RemoteInterface::MessageHandler *src,
						const RemoteInterface::Message& msg) {
		int scale = std::stoi(msg.get_value("scale"));
		Machine::machine_operation_enqueue(
			[this, scale]() {
				pad.config.set_scale(scale);
			}
			);
	}

	void Sequence::handle_req_pad_record(RemoteInterface::Context *context,
					     RemoteInterface::MessageHandler *src,
					     const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_req_pad_quantize(RemoteInterface::Context *context,
					       RemoteInterface::MessageHandler *src,
					       const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_req_pad_assign_midi_controller(RemoteInterface::Context *context,
							     RemoteInterface::MessageHandler *src,
							     const RemoteInterface::Message& msg) {
		auto p_axis = (PadAxis_t)std::stoi(msg.get_value("axis"));
		auto name = msg.get_value("controller");

		int pad_controller_coarse = -1;
		int pad_controller_fine = -1;
		get_midi_controllers(name, pad_controller_coarse, pad_controller_fine);
		SATAN_DEBUG("Sequence::handle_req_pad_assign_midi_controller() : c(%d), f(%d)\n",
			    pad_controller_coarse,
			    pad_controller_fine);
		Machine::machine_operation_enqueue(
			[this, p_axis, pad_controller_coarse, pad_controller_fine]() {
				pad.config.set_coarse_controller(
					p_axis == pad_y_axis ? 0 : 1, pad_controller_coarse);
				pad.config.set_fine_controller(
					p_axis == pad_y_axis ? 0 : 1, pad_controller_fine);
			}
			);
	}

	void Sequence::handle_req_pad_set_chord_mode(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		ChordMode_t chord_mode = (ChordMode_t)std::stoi(msg.get_value("chordmode"));
		Pad::PadConfiguration::ChordMode cm = Pad::PadConfiguration::chord_off;
		switch(chord_mode) {
		case chord_off:
			cm = Pad::PadConfiguration::chord_off;
			break;
		case chord_triad:
			cm = Pad::PadConfiguration::chord_triad;
			break;
		case chord_quad:
			cm = Pad::PadConfiguration::chord_quad;
			break;
		}
		Machine::machine_operation_enqueue(
			[this, cm]() {
				pad.config.set_chord_mode(cm);
			}
			);
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

	void Sequence::handle_req_pad_set_arpeggio_pattern(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		std::string identity = msg.get_value("arppattern");

		build_arpeggio_pattern_id_list();

		int arp_pattern_index = -1;
		for(unsigned int k = 0; k < arpeggio_pattern_id_list.size(); k++) {
			if(arpeggio_pattern_id_list[k] == identity) {
				arp_pattern_index = k;
			}
		}

		Machine::machine_operation_enqueue(
			[this, arp_pattern_index]() {
				if(arp_pattern_index == -1) {
					pad.config.set_arpeggio_direction(Pad::PadConfiguration::arp_off);
				}

				pad.config.set_arpeggio_pattern(arp_pattern_index);
			}
			);
	}

	void Sequence::handle_req_pad_set_arpeggio_direction(RemoteInterface::Context *context,
							     RemoteInterface::MessageHandler *src,
							     const RemoteInterface::Message& msg) {
		ArpeggioDirection_t i_arp_direction = (ArpeggioDirection_t)std::stoi(msg.get_value("direction"));
		Pad::PadConfiguration::ArpeggioDirection adir =
			Pad::PadConfiguration::arp_off;
		switch(i_arp_direction) {
		case arp_off:
			adir = Pad::PadConfiguration::arp_off;
			break;
		case arp_forward:
			adir = Pad::PadConfiguration::arp_forward;
			break;
		case arp_reverse:
			adir = Pad::PadConfiguration::arp_reverse;
			break;
		case arp_pingpong:
			adir = Pad::PadConfiguration::arp_pingpong;
			break;
		}
		Machine::machine_operation_enqueue(
			[this, adir]() {
				SATAN_DEBUG("Arpeggio direction selected (%p): %d\n", &(pad), adir);
				pad.config.set_arpeggio_direction(adir);
			});
	}

	void Sequence::handle_req_pad_clear(RemoteInterface::Context *context,
					    RemoteInterface::MessageHandler *src,
					    const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_req_pad_enqueue_event(RemoteInterface::Context *context,
						    RemoteInterface::MessageHandler *src,
						    const RemoteInterface::Message& msg) {
		PadEvent_t event_type = (PadEvent_t)(std::stol(msg.get_value("evt")));
		int finger = std::stol(msg.get_value("fgr"));
		int xp = std::stol(msg.get_value("xp"));
		int yp = std::stol(msg.get_value("yp"));
		int zp = std::stol(msg.get_value("zp"));

		Pad::PadEvent::PadEvent_t pevt = Pad::PadEvent::ms_pad_no_event;
		switch(event_type) {
		case ms_pad_press:
			pevt = Pad::PadEvent::ms_pad_press;
			break;
		case ms_pad_slide:
			pevt = Pad::PadEvent::ms_pad_slide;
			break;
		case ms_pad_release:
			pevt = Pad::PadEvent::ms_pad_release;
			break;
		case ms_pad_no_event:
			pevt = Pad::PadEvent::ms_pad_no_event;
			break;
		}
		pad.enqueue_event(finger, pevt, xp, yp, zp);
	}

	void Sequence::handle_req_enqueue_midi_data(RemoteInterface::Context *context,
						    RemoteInterface::MessageHandler *src,
						    const RemoteInterface::Message& msg) {
		size_t len = 0;
		char *data = NULL;
		decode_byte_array(msg.get_value("data"), len, &data);
		if(data == NULL) {
			SATAN_ERROR("Could not decode enqueued midi data.");
			return;
		}
		Machine::machine_operation_enqueue(
			[this, data, len] {
				_meb.queue_midi_data(len, data);
				free(data);
			});
	}

	std::map<std::shared_ptr<MachineAPI>, std::shared_ptr<Sequence> > Sequence::machine2sequence;

	void Sequence::create_sequence_for_machine(std::shared_ptr<MachineAPI> sibling) {
		for(auto socket_name : sibling->get_socket_names(BaseMachine::InputSocket))
			if(MACHINE_SEQUENCER_MIDI_INPUT_NAME == socket_name) {
				Machine::machine_operation_enqueue(
					[sibling]() {
						if(machine2sequence.find(sibling) !=
						   machine2sequence.end()) {
							SATAN_ERROR("Error. Machine already has a sequencer.\n");
							throw jException("Trying to create MachineSequencer for a machine that already has one!",
									 jException::sanity_error);
						}
					}
					);

				Server::create_object<Sequence>(
					[sibling](std::shared_ptr<Sequence> seq) {
						seq->init_from_machine_ptr(seq, seq);
						SATAN_DEBUG("Sequence::create_sequence_for_machine() attach input for sibling. A\n");
						sibling->attach_machine_input(
							seq,
							MACHINE_SEQUENCER_MIDI_OUTPUT_NAME,
							MACHINE_SEQUENCER_MIDI_INPUT_NAME);
						SATAN_DEBUG("Sequence::create_sequence_for_machine() attach input for sibling. B\n");
						seq->sequence_name = sibling->get_name();

						SATAN_DEBUG("    ***   \n");
						SATAN_DEBUG("    ***   Scanning controllers for midi...\n");
						SATAN_DEBUG("    ***   \n");

						for(auto knob : sibling->get_all_knobs()) {
							SATAN_DEBUG(" ---- knob name: %s\n", knob->get_name().c_str());
							int coarse_controller, fine_controller;
							if(knob->has_midi_controller(coarse_controller, fine_controller)) {
								seq->knob2midi[knob->get_name()] =
									std::pair<int, int>(coarse_controller, fine_controller);
								SATAN_DEBUG("  !! Created knob2midi[%s] for %p\n", knob->get_name().c_str(), knob.get());
							} else {
								SATAN_DEBUG("  !! --- has no midi.\n");
							}
						}

						SATAN_DEBUG("Sequence::create_sequence_for_machine() -- sequence_name = %s\n",
							    seq->sequence_name.c_str());

						seq->connect_tightly(sibling);
						machine2sequence[sibling] = seq;
					}
					);
			}
	}

	void Sequence::serialize(std::shared_ptr<Message> &target) {
		BaseMachine::serialize(target);
		Serialize::ItemSerializer iser;
		serderize_sequence(iser);
		target->set_value("sequence_data", iser.result());
	}

	bool Sequence::detach_and_destroy() {
		detach_all_inputs();
		detach_all_outputs();

		request_delete_me();

		return true;
	}

	std::string Sequence::get_class_name() {
		return "Sequence";
	}

	std::string Sequence::get_descriptive_xml() {
		return "";
	}

	ON_SERVER(
		uint32_t Sequence::get_pattern_starting_at(int sequence_position) {
			uint32_t id_at_requested_position = IDAllocator::NO_ID_AVAILABLE;
			instance_list.for_each(
				[this, sequence_position, &id_at_requested_position](PatternInstance *pin) {
					if(pin->start_at == sequence_position)
						id_at_requested_position = pin->pattern_id;
				}
				);
			return id_at_requested_position;
		}

		void Sequence::start_to_play_pattern(Pattern *pattern_to_play) {
			current_pattern = pattern_to_play;
			current_pattern->current_playing_position = 0;
			current_pattern->next_note_to_play = current_pattern->note_list.head;
		}

		bool Sequence::activate_note(Pattern *p, Note *note) {
			for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
				if(p->active_note[x] == NULL) {
					p->active_note[x] = note;
					return true;
				}
			}
			return false;
		}

		void Sequence::deactivate_note(Pattern *p, Note *note) {
			for(int x = 0; x < MAX_ACTIVE_NOTES; x++) {
				if(p->active_note[x] == note) {
					p->active_note[x] = NULL;
					return;
				}
			}
		}

		void Sequence::process_note_on(Pattern *p, bool mute, MidiEventBuilder *_meb) {
			auto playing = get_is_playing();
			while(p->next_note_to_play != NULL &&
			      (p->next_note_to_play->on_at == p->current_playing_position)) {
				Note *n = p->next_note_to_play;

				if(playing && (!mute) && activate_note(p, n)) {
					_meb->queue_note_on(n->note, n->velocity, n->channel);
					n->ticks2off = n->length + 1;
				}
				// then move on to the next in queue
				p->next_note_to_play = n->next;
			}
		}

		void Sequence::process_note_off(Pattern *p, MidiEventBuilder *_meb) {
			for(int k = 0; k < MAX_ACTIVE_NOTES; k++) {
				if(p->active_note[k] != NULL) {
					Note *n = p->active_note[k];
					if(n->ticks2off > 0) {
						n->ticks2off--;
					} else {
						deactivate_note(p, n);
						_meb->queue_note_off(n->note, 0x80, n->channel);
					}
				}
			}
		}

		void Sequence::process_current_pattern(bool mute, MidiEventBuilder *_meb)  {
			if(current_pattern) {
				process_note_on(current_pattern, mute, _meb);
				process_note_off(current_pattern, _meb);
				current_pattern->current_playing_position++;
			}
		}

		void Sequence::fill_buffers() {
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

			bool no_sound = get_is_playing() ? mute : true;

			while(_meb.skip(skip_length)) {

				pad.process(no_sound, PAD_TIME(sequence_position, current_tick), &_meb);

				if(current_tick == 0) {
					auto pattern_id = get_pattern_starting_at(sequence_position);

					if(pattern_id != IDAllocator::NO_ID_AVAILABLE) {
						start_to_play_pattern(patterns[pattern_id]);
					}
				}

				process_current_pattern(no_sound, &_meb);

//				process_controller_envelopes(PAD_TIME(sequence_position, current_tick), &_meb);

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
		);

	ON_CLIENT(
		void Sequence::fill_buffers() {}
		);

	void Sequence::reset() {
		ON_SERVER(
			pad.reset();
			);
	}

	std::vector<std::string> Sequence::internal_get_controller_groups() {
		std::vector<std::string> empty;
		return empty;
	}

	std::vector<std::string> Sequence::internal_get_controller_names() {
		std::vector<std::string> empty;
		return empty;
	}

	std::vector<std::string> Sequence::internal_get_controller_names(
		const std::string &group_name) {
		std::vector<std::string> empty;
		return empty;
	}

	Machine::Controller* Sequence::internal_get_controller(const std::string &name) {
		throw jException("No such controller.", jException::sanity_error);
	}

	std::string Sequence::internal_get_hint() {
		return "sequencer";
	}
);

CLIENT_CODE(

	Sequence::Sequence(const Factory *factory, const RemoteInterface::Message &serialized)
	: BaseMachine(factory, serialized)
	ON_SERVER(, Machine("Sequencer", false, 0.0f, 0.0f))
	{
		ON_SERVER(
			Machine::machine_operation_enqueue(
				[this] {
					output_descriptor[SEQUENCE_MIDI_OUTPUT_NAME] =
						Signal::Description(_MIDI, 1, false);
					setup_machine();
				}
				);
			);
		register_handlers();

		Serialize::ItemDeserializer serder(serialized.get_value("sequence_data"));
		serderize_sequence(serder);

		SATAN_DEBUG("Sequence() created client side.\n");
	}

	void Sequence::handle_cmd_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));
		auto start_at = std::stoi(msg.get_value("start_at"));
		auto loop_length = std::stoi(msg.get_value("loop_length"));
		auto stop_at = std::stoi(msg.get_value("stop_at"));

		if(process_add_pattern_instance(pattern_id, start_at, loop_length, stop_at)) {
			PatternInstance pi;
			pi.pattern_id = pattern_id;
			pi.start_at = start_at;
			pi.loop_length = loop_length;
			pi.stop_at = stop_at;
			for(auto sl : sequence_listeners)
				sl->instance_added(pi);
		}
	}

	void Sequence::handle_cmd_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		PatternInstance to_del = {
			.pattern_id = std::stoul(msg.get_value("pattern_id")),
			.start_at = std::stoi(msg.get_value("start_at")),
			.loop_length = std::stoi(msg.get_value("loop_length")),
			.stop_at = std::stoi(msg.get_value("stop_at")),
			.next = NULL,
		};

		process_del_pattern_instance(to_del);

		for(auto sl : sequence_listeners)
			sl->instance_deleted(to_del);
	}

	void Sequence::handle_cmd_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));
		auto channel = std::stoi(msg.get_value("channel"));
		auto program = std::stoi(msg.get_value("program"));
		auto velocity = std::stoi(msg.get_value("velocity"));
		auto note = std::stoi(msg.get_value("note"));
		auto on_at = std::stoi(msg.get_value("on_at"));
		auto length = std::stoi(msg.get_value("length"));

		process_add_note(
			pattern_id,
			channel, program, velocity,
			note, on_at, length
			);

		Note added_note;
		added_note.channel = channel;
		added_note.program = program;
		added_note.velocity = velocity;
		added_note.note = note;
		added_note.on_at = on_at;
		added_note.length = length;

		for(auto pl : patterns[pattern_id]->pattern_listeners)
			pl->note_added(pattern_id, added_note);
	}

	void Sequence::handle_cmd_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		Note note_to_delete =
		{
			.channel = std::stoi(msg.get_value("channel")),
			.program = std::stoi(msg.get_value("program")),
			.velocity = std::stoi(msg.get_value("velocity")),
			.note = std::stoi(msg.get_value("note")),
			.on_at = std::stoi(msg.get_value("on_at")),
			.length = std::stoi(msg.get_value("length"))
		};
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));

		if(patterns.find(pattern_id) == patterns.end())
			return;

		process_delete_note(
			pattern_id,
			note_to_delete);

		for(auto pl : patterns[pattern_id]->pattern_listeners)
			pl->note_deleted(pattern_id, note_to_delete);
	}

	void Sequence::insert_pattern_in_sequence(uint32_t pattern_id,
						  int start_at,
						  int loop_length,
						  int stop_at)  {
		send_message_to_server(
			req_add_pattern_instance,
			[pattern_id, start_at, loop_length, stop_at]
			(std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("pattern_id", std::to_string(pattern_id));
				msg2send->set_value("start_at", std::to_string(start_at));
				msg2send->set_value("loop_length", std::to_string(loop_length));
				msg2send->set_value("stop_at", std::to_string(stop_at));
			}
		);
	}

	void Sequence::get_sequence(std::list<PatternInstance> &storage)  {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);

		storage.clear();

		instance_list.for_each(
			[&storage](PatternInstance *pin) {
				storage.push_back(*pin);
			}
			);
	}

	void Sequence::delete_pattern_from_sequence(const PatternInstance& pattern_instance)  {
		auto pattern_id = pattern_instance.pattern_id;
		auto start_at = pattern_instance.start_at;
		auto loop_length = pattern_instance.loop_length;
		auto stop_at = pattern_instance.stop_at;
		send_message_to_server(
			req_del_pattern_instance,
			[pattern_id, start_at, loop_length, stop_at]
			(std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("pattern_id", std::to_string(pattern_id));
				msg2send->set_value("start_at", std::to_string(start_at));
				msg2send->set_value("loop_length", std::to_string(loop_length));
				msg2send->set_value("stop_at", std::to_string(stop_at));
			}
		);
	}


	void Sequence::add_note(uint32_t pattern_id,
				int channel, int program, int velocity,
				int note, int on_at, int length)  {
		send_message_to_server(
			req_add_note,
			[pattern_id, channel, program, velocity,
			 note, on_at, length]
			(std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("pattern_id", std::to_string(pattern_id));
				msg2send->set_value("channel", std::to_string(channel));
				msg2send->set_value("program", std::to_string(program));
				msg2send->set_value("velocity", std::to_string(velocity));
				msg2send->set_value("note", std::to_string(note));
				msg2send->set_value("on_at", std::to_string(on_at));
				msg2send->set_value("length", std::to_string(length));
			}
		);
	}

	void Sequence::get_notes(uint32_t pattern_id, std::list<Note> &storage)  {
		storage.clear();

		auto ptrn_i = patterns.find(pattern_id);
		if(ptrn_i == patterns.end())
			return;
		auto ptrn = (*ptrn_i).second;

		ptrn->note_list.for_each(
			[&storage](Note* nin) {
				storage.push_back(*nin);
			}
			);
	}

	void Sequence::delete_note(uint32_t pattern_id, const Note& note_obj)  {
		auto channel = note_obj.channel;
		auto program = note_obj.program;
		auto velocity = note_obj.velocity;
		auto note = note_obj.note;
		auto on_at = note_obj.on_at;
		auto length = note_obj.length;

		send_message_to_server(
			req_del_note,
			[pattern_id, channel, program, velocity,
			 note, on_at, length]
			(std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("pattern_id", std::to_string(pattern_id));
				msg2send->set_value("channel", std::to_string(channel));
				msg2send->set_value("program", std::to_string(program));
				msg2send->set_value("velocity", std::to_string(velocity));
				msg2send->set_value("note", std::to_string(note));
				msg2send->set_value("on_at", std::to_string(on_at));
				msg2send->set_value("length", std::to_string(length));
			}
		);
	}

	std::set<std::string> Sequence::available_midi_controllers() {
		std::set<std::string> result;

		for(auto k2m : knob2midi) {
			result.insert(k2m.first);
		}

		return result;
	}

	void Sequence::pad_export_to_loop(int loop_id) {}

	void Sequence::pad_set_octave(int octave) {
		send_message_to_server(
			req_pad_set_octave,
			[octave](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("octave", std::to_string(octave));
			}
			);
	}

	void Sequence::pad_set_scale(int scale_index) {
		send_message_to_server(
			req_pad_set_scale,
			[scale_index](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("scale", std::to_string(scale_index));
			}
			);
	}

	void Sequence::pad_set_record(bool record) {}

	void Sequence::pad_set_quantize(bool do_quantize) {}

	void Sequence::pad_assign_midi_controller(PadAxis_t axis, const std::string &controller) {
		send_message_to_server(
			req_pad_assign_midi_controller,
			[axis, controller](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("axis", std::to_string(axis));
				msg2send->set_value("controller", controller);
				SATAN_DEBUG("Sequence::pad_assign_midi_controller() - controller: %s\n", controller.c_str());
			}
			);
	}

	void Sequence::pad_set_chord_mode(ChordMode_t chord_mode) {
		send_message_to_server(
			req_pad_set_chord_mode,
			[chord_mode](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("chordmode", std::to_string((int)chord_mode));
			}
			);
	}

	void Sequence::pad_set_arpeggio_pattern(const std::string &arp_pattern) {
		send_message_to_server(
			req_pad_set_arpeggio_pattern,
			[arp_pattern](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("arppattern", arp_pattern);
			}
			);
	}

	void Sequence::pad_set_arpeggio_direction(ArpeggioDirection_t arp_direction) {
		send_message_to_server(
			req_pad_set_arpeggio_direction,
			[arp_direction](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("direction", std::to_string((int)arp_direction));
			}
			);
	}

	void Sequence::pad_clear() {}

	void Sequence::pad_enqueue_event(int finger, PadEvent_t event_type, float ev_x, float ev_y, float ev_z) {
		ev_x *= 16383.0f;
		ev_y = (1.0 - ev_y) * 16383.0f;
		ev_z = ev_z * 16383.0f;
		int xp = ev_x;
		int yp = ev_y;
		int zp = ev_z;

		auto thiz = std::dynamic_pointer_cast<RIMachine>(shared_from_this());
		send_message_to_server(
			req_pad_enqueue_event,
			[xp, yp, zp, finger, event_type](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("evt", std::to_string(((int)event_type)));
				msg2send->set_value("fgr", std::to_string(finger));
				msg2send->set_value("xp", std::to_string(xp));
				msg2send->set_value("yp", std::to_string(yp));
				msg2send->set_value("zp", std::to_string(zp));
			}
			);
	}

	void Sequence::enqueue_midi_data(size_t len, const char* data) {
		std::string encoded = encode_byte_array(len, data);
		send_message_to_server(
			req_enqueue_midi_data,
			[encoded](std::shared_ptr<Message> &msg2send) {
				msg2send->set_value("data", encoded);
				SATAN_DEBUG("Sequence::enqueue_midi_data() - data: %s\n", encoded.c_str());
			}
			);
	}

	std::string Sequence::get_name() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return sequence_name;
	}

	void Sequence::add_sequence_listener(std::shared_ptr<Sequence::SequenceListener> sel) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		sequence_listeners.insert(sel);
	}

	void Sequence::add_pattern_listener(uint32_t pattern_id, std::shared_ptr<Sequence::PatternListener> pal) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);

		// only attach it to one pattern at a time
		for(auto ptrn : patterns) {
			ptrn.second->pattern_listeners.erase(pal);
		}

		auto ptrn = require_pattern(pattern_id);
		ptrn->pattern_listeners.insert(pal);

		std::list<Note> note_storage;
		get_notes(pattern_id, note_storage);
		for(auto note : note_storage) {
			pal->note_added(pattern_id, note);
		}
	}

	void Sequence::drop_pattern_listener(std::shared_ptr<Sequence::PatternListener> pal) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);

		for(auto ptrn : patterns) {
			ptrn.second->pattern_listeners.erase(pal);
		}
	}

	);

SERVER_N_CLIENT_CODE(

	Sequence::Pattern* Sequence::require_pattern(uint32_t pattern_id) {
		if(patterns.find(pattern_id) == patterns.end()) {
			auto ptrn = pattern_allocator.allocate();
			ptrn->id = pattern_id;
			ptrn->name = "";
			patterns[pattern_id] = ptrn;
		}
		return patterns[pattern_id];
	}

	bool Sequence::process_add_pattern_instance(uint32_t pattern_id,
						    int start_at,
						    int loop_length,
						    int stop_at) {
		if(stop_at - start_at <= 0) return false;

		(void)require_pattern(pattern_id);

		bool did_overlap = false;
		instance_list.for_each(
			[&did_overlap, start_at, loop_length, stop_at]
			(PatternInstance* _pin) {
				if(
					(
						_pin->start_at >= start_at
						&&
						_pin->start_at < stop_at
						)
					||
					(
						_pin->stop_at > start_at
						&&
						_pin->stop_at <= stop_at
						)
					) {
					// can't do - overlapping
					did_overlap = true;
				}
			}
			);

		if(did_overlap)
			return false;	// can't do - overlapping

		// create instance
		auto new_instance = pattern_instance_allocator.allocate();
		new_instance->pattern_id = pattern_id;
		new_instance->start_at = start_at;
		new_instance->loop_length = loop_length;
		new_instance->stop_at = stop_at;
		new_instance->next = NULL;

		// insert in list
		instance_list.insert_element(new_instance);

		return true;
	}

	void Sequence::process_del_pattern_instance(const PatternInstance& pattern_instance) {
		instance_list.drop_element(
			&pattern_instance,
			[](PatternInstance* _pin) {
				pattern_instance_allocator.recycle(_pin);
			}
			);
	}

	void Sequence::process_add_note(uint32_t pattern_id,
					int channel, int program, int velocity,
					int note, int on_at, int length) {
		auto ptrn = require_pattern(pattern_id);

		auto new_note = note_allocator.allocate();
		new_note->channel = channel;
		new_note->program = program;
		new_note->velocity = velocity;
		new_note->note = note;
		new_note->on_at = on_at;
		new_note->length = length;

		ptrn->note_list.insert_element(new_note);
	}

	void Sequence::process_delete_note(uint32_t pattern_id, const Note& note) {
		auto ptrn_itr = patterns.find(pattern_id);
		if(ptrn_itr == patterns.end()) return;
		auto ptrn = ptrn_itr->second;

		ON_SERVER(
			if(ptrn->next_note_to_play && note == *(ptrn->next_note_to_play)) {
				SATAN_ERROR("(%s) ::process_delete_note() - B-1\n", CLIENTORSERVER_STRING);
				ptrn->next_note_to_play = ptrn->next_note_to_play->next;
			}
			);

		ptrn->note_list.drop_element(
			&note,
			[](Note* _note) {
				note_allocator.recycle(_note);
			}
			);
	}

	template <class SerderClassT>
	void Sequence::Note::serderize_single(Note *trgt, SerderClassT& iserder) {
		iserder.process(trgt->channel);
		iserder.process(trgt->program);
		iserder.process(trgt->velocity);
		iserder.process(trgt->note);
		iserder.process(trgt->on_at);
		iserder.process(trgt->length);
	}

	template <class SerderClassT>
	void Sequence::Note::serderize(SerderClassT& iserder) {
		serderize_single(this, iserder);

		Note **current_note = &(next);
		bool has_next_note;
		do {
			has_next_note = (*current_note) != NULL;
			iserder.process(has_next_note);
			if(has_next_note) {
				if((*current_note) == NULL)
					(*current_note) = Note::allocate();
				serderize_single((*current_note), iserder);
				current_note = &((*current_note)->next);
			}
		} while(has_next_note);

	}

	Sequence::Note* Sequence::Note::allocate() {
		auto retval = note_allocator.allocate();
		retval->next = NULL;
		return retval;
	}

	template <class SerderClassT>
	void Sequence::Pattern::serderize(SerderClassT& iserder) {
		iserder.process(id);
		iserder.process(name);

		bool has_note = note_list.head != NULL;

		iserder.process(has_note);

		if(has_note)
			iserder.process(note_list.head);

	}

	Sequence::Pattern* Sequence::Pattern::allocate() {
		return pattern_allocator.allocate();
	}

	template <class SerderClassT>
	void Sequence::serderize_sequence(SerderClassT& iserder) {
		SATAN_DEBUG("[%s] : ----- GOING IN %p, knob2midi.size(%d)\n", CLIENTORSERVER_STRING, this, knob2midi.size());
		iserder.process(sequence_name);
		SATAN_DEBUG("serderize_sequence() -- sequence_name: %s\n",
			    sequence_name.c_str());

		iserder.process(patterns);
		iserder.process(knob2midi);

		SATAN_DEBUG("[%s] : ----- (post %p) knob2midi.size(%d)\n", CLIENTORSERVER_STRING, this, knob2midi.size());
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(
		const Message &serialized,
		RegisterObjectFunction register_object
		) {
		ON_CLIENT(
			SATAN_DEBUG("Trying to deserialize new Sequence...\n");
			auto nseq = std::make_shared<Sequence>(this, serialized);
			BaseMachine::register_by_name(nseq);
			return register_object(nseq);
			);
		ON_SERVER(
			return nullptr;
			);
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(
		int32_t new_obj_id,
		RegisterObjectFunction register_object
		) {
		ON_SERVER(
			auto nseq_ptr = new Sequence(new_obj_id, this);
			auto nseq = std::dynamic_pointer_cast<Sequence>(nseq_ptr->get_shared_pointer());
			return register_object(nseq);
			);
		ON_CLIENT(
			return nullptr;
			);
	}

	static Sequence::SequenceFactory this_will_register_us_as_a_factory;

	ObjectAllocator<Sequence::Pattern> Sequence::pattern_allocator;
	ObjectAllocator<Sequence::PatternInstance> Sequence::pattern_instance_allocator;
	ObjectAllocator<Sequence::Note> Sequence::note_allocator;

	);
