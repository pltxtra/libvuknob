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

SERVER_CODE(

	static IDAllocator pattern_id_allocator;

	void Sequence::add_pattern(const std::string& new_name) {
		auto new_id = pattern_id_allocator.get_id();

		process_add_pattern(new_name, new_id);

		send_message(
			cmd_add_pattern,
			[new_id, new_name](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("pattern_id", std::to_string(new_id));
				msg_to_send->set_value("name", new_name);
			}
			);
	}

	void Sequence::delete_pattern(uint32_t pattern_id) {
		if(process_del_pattern(pattern_id)) {
			pattern_id_allocator.free_id(pattern_id);

			send_message(
				cmd_del_pattern,
				[pattern_id](std::shared_ptr<Message> &msg_to_send) {
					msg_to_send->set_value("pattern_id", std::to_string(pattern_id));
				}
				);
		}
	}

	void Sequence::insert_pattern_in_sequence(uint32_t pattern_id,
						  int start_at,
						  int loop_length,
						  int stop_at) {
		if(process_add_pattern_instance(pattern_id, start_at, loop_length, stop_at)) {
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

	void Sequence::delete_pattern_from_sequence(const PatternInstance& pattern_instance) {
		process_del_pattern_instance(pattern_instance);

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


	void Sequence::add_note(
		uint32_t pattern_id,
		int channel, int program, int velocity,
		int note, int on_at, int length
		) {
		process_add_note(
			pattern_id,
			channel, program, velocity,
			note, on_at, length);

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

	void Sequence::delete_note(
		uint32_t pattern_id,
		const Note& note
		) {
		process_delete_note(pattern_id, note);

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

	void Sequence::handle_req_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		add_pattern(msg.get_value("name"));
	}

	void Sequence::handle_req_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		delete_pattern((uint32_t)(std::stoul(msg.get_value("pattern_id"))));
	}

	void Sequence::handle_req_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		uint32_t pattern_id = std::stoul(msg.get_value("pattern_id"));
		int start_at = std::stoi(msg.get_value("start_at"));
		int loop_length = std::stoi(msg.get_value("loop_length"));
		int stop_at = std::stoi(msg.get_value("stop_at"));
		insert_pattern_in_sequence(pattern_id,
					   start_at,
					   loop_length,
					   stop_at);
	}

	void Sequence::handle_req_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		PatternInstance to_del = {
			.pattern_id = std::stoul(msg.get_value("pattern_id")),
			.start_at = std::stoi(msg.get_value("start_at")),
			.loop_length = std::stoi(msg.get_value("loop_length")),
			.stop_at = std::stoi(msg.get_value("stop_at")),
			.next = NULL,
		};

		delete_pattern_from_sequence(to_del);
	}

	void Sequence::handle_req_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {

		add_note(
			std::stoul(msg.get_value("pattern_id")),
			std::stoi(msg.get_value("channel")),
			std::stoi(msg.get_value("program")),
			std::stoi(msg.get_value("velocity")),
			std::stoi(msg.get_value("note")),
			std::stoi(msg.get_value("on_at")),
			std::stoi(msg.get_value("length"))
			);
	}

	void Sequence::handle_req_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		Note note_to_delete =
		{
			.channel = std::stoi(msg.get_value("channel")),
			.program = std::stoi(msg.get_value("program")),
			.velocity = std::stoi(msg.get_value("velocity")),
			.note = std::stoi(msg.get_value("note")),
			.on_at = std::stoi(msg.get_value("on_at")),
			.length = std::stoi(msg.get_value("length"))
		};
		delete_note(std::stoul(msg.get_value("pattern_id")), note_to_delete);
	}

	void Sequence::init_from_machine_sequencer(MachineSequencer *__m_seq) {
		m_seq = __m_seq;
	}

	void Sequence::serialize(std::shared_ptr<Message> &target) {
		Serialize::ItemSerializer iser;
		serderize_sequence(iser);
		target->set_value("sequence_data", iser.result());
	}

	);

CLIENT_CODE(

	void Sequence::handle_cmd_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto new_name = msg.get_value("name");
		auto new_id = std::stoul(msg.get_value("pattern_id"));

		process_add_pattern(new_name, new_id);
	}

	void Sequence::handle_cmd_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));

		(void) process_del_pattern(pattern_id);
	}

	void Sequence::handle_cmd_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));
		auto start_at = std::stoi(msg.get_value("start_at"));
		auto loop_length = std::stoi(msg.get_value("loop_length"));
		auto stop_at = std::stoi(msg.get_value("stop_at"));

		(void) process_add_pattern_instance(pattern_id, start_at, loop_length, stop_at);
	}

	void Sequence::handle_cmd_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		PatternInstance to_del = {
			.pattern_id = std::stoul(msg.get_value("pattern_id")),
			.start_at = std::stoi(msg.get_value("start_at")),
			.loop_length = std::stoi(msg.get_value("loop_length")),
			.stop_at = std::stoi(msg.get_value("stop_at")),
			.next = NULL,
		};

		process_del_pattern_instance(to_del);
	}

	void Sequence::handle_cmd_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		process_add_note(
			std::stoul(msg.get_value("pattern_id")),
			std::stoi(msg.get_value("channel")),
			std::stoi(msg.get_value("program")),
			std::stoi(msg.get_value("velocity")),
			std::stoi(msg.get_value("note")),
			std::stoi(msg.get_value("on_at")),
			std::stoi(msg.get_value("length"))
			);
	}

	void Sequence::handle_cmd_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
		Note note_to_delete =
		{
			.channel = std::stoi(msg.get_value("channel")),
			.program = std::stoi(msg.get_value("program")),
			.velocity = std::stoi(msg.get_value("velocity")),
			.note = std::stoi(msg.get_value("note")),
			.on_at = std::stoi(msg.get_value("on_at")),
			.length = std::stoi(msg.get_value("length"))
		};

		process_delete_note(
			std::stoul(msg.get_value("pattern_id")),
			note_to_delete);
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
				msg2send->set_value("id", std::to_string(pattern_id));
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
				msg2send->set_value("id", std::to_string(pattern_id));
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
				msg2send->set_value("id", std::to_string(pattern_id));
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
				msg2send->set_value("id", std::to_string(pattern_id));
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
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);

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
				msg2send->set_value("id", std::to_string(pattern_id));
				msg2send->set_value("channel", std::to_string(channel));
				msg2send->set_value("program", std::to_string(program));
				msg2send->set_value("velocity", std::to_string(velocity));
				msg2send->set_value("note", std::to_string(note));
				msg2send->set_value("on_at", std::to_string(on_at));
				msg2send->set_value("length", std::to_string(length));
			}
		);
	}

	);

SERVER_N_CLIENT_CODE(
	static std::mutex sequence_global_mutex;

	std::set<
	std::weak_ptr<Sequence::SequenceListener>,
	std::owner_less<std::weak_ptr<Sequence::SequenceListener> >
	> Sequence::seq_listeners;

	std::set<std::shared_ptr<Sequence> > Sequence::sequences;

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
						_pin->start_at <= stop_at
						)
					||
					(
						_pin->stop_at >= start_at
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
		iserder.process(patterns);
	}

	Sequence::Sequence(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized) {
		register_handlers();

		Serialize::ItemDeserializer serder(serialized.get_value("sequence_data"));
		serderize_sequence(serder);

		SATAN_DEBUG("Sequence() created client side.\n");
	}

	Sequence::Sequence(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory) {
		register_handlers();
		SATAN_DEBUG("Sequence() created server side.\n");
	}

	void Sequence::register_sequence_listener(std::weak_ptr<SequenceListener> weak_lstnr) {
		std::lock_guard<std::mutex> lock_guard(sequence_global_mutex);
		seq_listeners.insert(weak_lstnr);

		if(auto lstnr = weak_lstnr.lock()) {
			for(auto seq : sequences) {
				lstnr->sequence_added(seq);
			}
		}
	}

	void Sequence::remember_sequence(std::shared_ptr<Sequence> seq) {
		std::lock_guard<std::mutex> lock_guard(sequence_global_mutex);
		sequences.insert(seq);
		for(auto weak_lstnr : seq_listeners) {
			if(auto lstnr = weak_lstnr.lock()) {
				lstnr->sequence_added(seq);
			}
		}
	}

	void Sequence::forget_sequence(std::shared_ptr<Sequence> seq) {
		std::lock_guard<std::mutex> lock_guard(sequence_global_mutex);
		for(auto weak_lstnr : seq_listeners) {
			if(auto lstnr = weak_lstnr.lock()) {
				lstnr->sequence_added(seq);
			}
		}
		sequences.erase(seq);
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(const Message &serialized) {
		auto nseq = std::make_shared<Sequence>(this, serialized);
		remember_sequence(nseq);
		return nseq;
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(int32_t new_obj_id) {
		auto nseq = std::make_shared<Sequence>(new_obj_id, this);
		remember_sequence(nseq);
		return nseq;
	}

	static Sequence::SequenceFactory this_will_register_us_as_a_factory;

	ObjectAllocator<Sequence::Pattern> Sequence::pattern_allocator;
	ObjectAllocator<Sequence::PatternInstance> Sequence::pattern_instance_allocator;
	ObjectAllocator<Sequence::Note> Sequence::note_allocator;

	);
