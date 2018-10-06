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
	static IDAllocator pattern_id_allocator;

	void Sequence::handle_req_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		std::string new_name = msg.get_value("name");
		uint32_t new_id = 0;
		SATAN_ERROR("::handle_req_add_pattern()\n");
		Machine::machine_operation_enqueue(
			[this, &new_id, new_name] {
				new_id = pattern_id_allocator.get_id();
				process_add_pattern(new_name, new_id);
			});

		send_message(
			cmd_add_pattern,
			[new_id, new_name](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("pattern_id", std::to_string(new_id));
				msg_to_send->set_value("name", new_name);
			}
			);
	}

	void Sequence::handle_req_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		uint32_t pattern_id = (uint32_t)(std::stoul(msg.get_value("pattern_id")));
		bool operation_successfull = false;
		Machine::machine_operation_enqueue(
			[this, &operation_successfull, pattern_id] {
				if(process_del_pattern(pattern_id)) {
					pattern_id_allocator.free_id(pattern_id);
					operation_successfull = true;
				}
			});

		if(operation_successfull) {
			send_message(
				cmd_del_pattern,
				[pattern_id](std::shared_ptr<Message> &msg_to_send) {
					msg_to_send->set_value("pattern_id", std::to_string(pattern_id));
				}
				);
		}

	}

	void Sequence::handle_req_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		uint32_t pattern_id = std::stoul(msg.get_value("pattern_id"));
		int start_at = std::stoi(msg.get_value("start_at"));
		int loop_length = std::stoi(msg.get_value("loop_length"));
		int stop_at = std::stoi(msg.get_value("stop_at"));
		bool operation_successfull = false;

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
			cmd_del_pattern_instance,
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

	std::map<std::shared_ptr<Machine>, std::shared_ptr<Sequence> > Sequence::machine2sequence;

	void Sequence::create_sequence_for_machine(std::shared_ptr<Machine> sibling) {
		{
			bool midi_input_found = false;

			try {
				(void) sibling->get_input_index(MACHINE_SEQUENCER_MIDI_INPUT_NAME);
				midi_input_found = true;
			} catch(...) {
				// ignore
			}

			if(!midi_input_found) {
				return; // no midi input to attach MachineSequencer too
			}
		}

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

				sibling->attach_input(seq.get(),
						      MACHINE_SEQUENCER_MIDI_OUTPUT_NAME,
						      MACHINE_SEQUENCER_MIDI_INPUT_NAME);
				seq->sequence_name = sibling->get_name();

				SATAN_DEBUG("Sequence::create_sequence_for_machine() -- sequence_name = %s\n",
					    seq->sequence_name.c_str());

				Machine::machine_operation_enqueue(
					[sibling, seq]() {
						seq->tightly_connect(sibling.get());
						machine2sequence[sibling] = seq;
					}
					);
			}
			);
	}

	void Sequence::serialize(std::shared_ptr<Message> &target) {
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

	void Sequence::fill_buffers() {
		// get output signal buffer and clear it.
		Signal *out_sig = output[MACHINE_SEQUENCER_MIDI_OUTPUT_NAME];

		int output_limit = out_sig->get_samples();
		void **output_buffer = (void **)out_sig->get_buffer();

		memset(output_buffer,
		       0,
		       sizeof(void *) * output_limit
			);
	}

	void Sequence::reset() {
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

	void Sequence::handle_cmd_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		auto new_name = msg.get_value("name");
		auto new_id = std::stoul(msg.get_value("pattern_id"));

		SATAN_ERROR("(client) ::handle_cmd_add_pattern() -- %d\n", sequence_listeners.size());
		process_add_pattern(new_name, new_id);

		for(auto sl : sequence_listeners)
			sl->pattern_added(new_name, new_id);
	}

	void Sequence::handle_cmd_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));

		if(patterns.find(pattern_id) != patterns.end()) {
			for(auto pl : patterns[pattern_id]->pattern_listeners)
				pl->pattern_deleted(pattern_id);
		}

		(void) process_del_pattern(pattern_id);
		for(auto sl : sequence_listeners)
			sl->pattern_deleted(pattern_id);
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

		if(patterns.find(pattern_id) == patterns.end())
			return;

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

	void Sequence::add_pattern(const std::string& name) {
		send_message_to_server(
			req_add_pattern,
			[name](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("name", name);
			}
		);
	}

	void Sequence::get_pattern_ids(std::list<uint32_t> &storage) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);

		storage.clear();

		for(auto pattern : patterns) {
			storage.push_back(pattern.second->id);
		}
	}

	void Sequence::delete_pattern(uint32_t pattern_id)  {
		send_message_to_server(
			req_del_pattern,
			[pattern_id](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("pattern_id", std::to_string(pattern_id));
			}
		);
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

		// if the requested pattern does not exist, quietly return
		if(patterns.find(pattern_id) == patterns.end()) return;

		patterns[pattern_id]->pattern_listeners.insert(pal);

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

	void Sequence::process_add_pattern(const std::string& new_name, uint32_t new_id) {
		auto ptrn = pattern_allocator.allocate();
		ptrn->id = new_id;
		ptrn->name = new_name;
		patterns[ptrn->id] = ptrn;
	}

	bool Sequence::process_del_pattern(uint32_t pattern_id) {
		auto ptrn_itr = patterns.find(pattern_id);

		if(ptrn_itr != patterns.end()) {
			ptrn_itr->second->note_list.clear(
				[](Note* to_drop) {
					note_allocator.recycle(to_drop);
				}
				);
			pattern_allocator.recycle(ptrn_itr->second);
			patterns.erase(ptrn_itr);

			return true;
		}
		return false;
	}

	bool Sequence::process_add_pattern_instance(uint32_t pattern_id,
						    int start_at,
						    int loop_length,
						    int stop_at) {
		if(stop_at - start_at <= 0) return false;

		auto ptrn_itr = patterns.find(pattern_id);

		if(ptrn_itr != patterns.end()) {
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
		return false;
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
		auto ptrn_itr = patterns.find(pattern_id);
		if(ptrn_itr == patterns.end()) return;
		auto ptrn = ptrn_itr->second;

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
		iserder.process(sequence_name);
		SATAN_DEBUG("serderize_sequence() -- sequence_name: %s\n",
			    sequence_name.c_str());

		iserder.process(patterns);
	}

	Sequence::Sequence(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
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

	Sequence::Sequence(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory)
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

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(const Message &serialized) {
		ON_SERVER(
			auto nseq_ptr = new Sequence(this, serialized);
			auto nseq = std::dynamic_pointer_cast<Sequence>(nseq_ptr->get_shared_pointer());
			);
		ON_CLIENT(
			auto nseq = std::make_shared<Sequence>(this, serialized);
			);
		return nseq;
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(int32_t new_obj_id) {
		ON_SERVER(
			auto nseq_ptr = new Sequence(new_obj_id, this);
			auto nseq = std::dynamic_pointer_cast<Sequence>(nseq_ptr->get_shared_pointer());
			);
		ON_CLIENT(
			auto nseq = std::make_shared<Sequence>(new_obj_id, this);
			);
		return nseq;
	}

	static Sequence::SequenceFactory this_will_register_us_as_a_factory;

	ObjectAllocator<Sequence::Pattern> Sequence::pattern_allocator;
	ObjectAllocator<Sequence::PatternInstance> Sequence::pattern_instance_allocator;
	ObjectAllocator<Sequence::Note> Sequence::note_allocator;

	);
