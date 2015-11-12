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
		auto ptrn = pattern_allocator.allocate();

		ptrn->id = new_id;
		ptrn->name = new_name;
		ptrn->first_note = NULL;

		patterns[ptrn->id] = ptrn;

		send_message(
			cmd_add_pattern,
			[new_id, new_name](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("pattern_id", std::to_string(new_id));
				msg_to_send->set_value("name", new_name);
			}
			);
	}

	void Sequence::delete_pattern(uint32_t pattern_id) {
		auto ptrn_itr = patterns.find(pattern_id);

		if(ptrn_itr != patterns.end()) {
			pattern_id_allocator.free_id(ptrn_itr->second->id);
			pattern_allocator.recycle(ptrn_itr->second);
			patterns.erase(ptrn_itr);

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
		if(stop_at - start_at <= 0) return;

		auto ptrn_itr = patterns.find(pattern_id);

		if(ptrn_itr != patterns.end()) {

			// don't add overlapping
			auto c_inst = first_instance;
			while(c_inst != NULL) {
				if(
					(
						c_inst->start_at >= start_at
						&&
						c_inst->start_at <= stop_at
						)
					||
					(
						c_inst->stop_at >= start_at
						&&
						c_inst->stop_at <= stop_at
						)
					) {
					// can't do - overlapping
					return;
				}
				c_inst = c_inst->next_instance;
			}

			// create instance
			auto new_instance = pattern_instance_allocator.allocate();
			new_instance->pattern_id = pattern_id;
			new_instance->start_at = start_at;
			new_instance->loop_length = loop_length;
			new_instance->stop_at = stop_at;
			new_instance->next_instance = NULL;

			// insert in chain
			if(first_instance == NULL || new_instance->start_at < first_instance->start_at) {
				new_instance->next_instance = first_instance;
				first_instance = new_instance;
			} else {
				auto this_instance = first_instance;
				while(this_instance->next_instance != NULL &&
				      this_instance->next_instance->start_at < new_instance->start_at) {
					this_instance = this_instance->next_instance;
				}
				new_instance->next_instance = this_instance->next_instance;
				this_instance->next_instance = new_instance;

			}

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
		if(first_instance == NULL) return;

		// find match in chain
		if(pattern_instance == *first_instance) {
			auto recycle_instance = first_instance;
			first_instance = first_instance->next_instance;
			pattern_instance_allocator.recycle(recycle_instance);
		} else {
			auto this_instance = first_instance;
			while(this_instance->next_instance != NULL &&
			      *(this_instance->next_instance) != pattern_instance) {
				this_instance = this_instance->next_instance;
			}
			if(this_instance &&
			   this_instance->next_instance &&
			   *(this_instance->next_instance) == pattern_instance) {
				auto next = this_instance->next_instance;
				this_instance->next_instance = next->next_instance;
				pattern_instance_allocator.recycle(next);
			}
		}

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
		int start_at = std::stol(msg.get_value("start_at"));
		int loop_length = std::stol(msg.get_value("loop_length"));
		int stop_at = std::stol(msg.get_value("stop_at"));
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
			.start_at = std::stol(msg.get_value("start_at")),
			.loop_length = std::stol(msg.get_value("loop_length")),
			.stop_at = std::stol(msg.get_value("stop_at")),
			.next_instance = NULL,
		};

		delete_pattern_from_sequence(to_del);
	}

	void Sequence::handle_req_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_req_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
	}

	void Sequence::init_from_machine_sequencer(MachineSequencer *m_seq) {

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
		auto ptrn = pattern_allocator.allocate();

		ptrn->id = new_id;
		ptrn->name = new_name;
		ptrn->first_note = NULL;

		patterns[ptrn->id] = ptrn;
	}

	void Sequence::handle_cmd_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto pattern_id = std::stoul(msg.get_value("pattern_id"));
		auto ptrn_itr = patterns.find(pattern_id);

		if(ptrn_itr != patterns.end()) {
			pattern_allocator.recycle(ptrn_itr->second);
			patterns.erase(ptrn_itr);
		}
	}

	void Sequence::handle_cmd_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_cmd_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_cmd_add_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_cmd_del_note(RemoteInterface::Context *context,
					   RemoteInterface::MessageHandler *src,
					   const RemoteInterface::Message& msg) {
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
	}

	void Sequence::get_sequence(std::list<PatternInstance> &storage)  {
	}

	void Sequence::delete_pattern_from_sequence(const PatternInstance& pattern_instance)  {
	}


	void Sequence::insert_note(uint32_t pattern_id,
				   int note, int velocity,
				   int start_at, int length)  {
	}

	void Sequence::get_notes(uint32_t pattern_id, std::list<Note> &storage)  {
	}

	void Sequence::delete_note(uint32_t pattern_id, const Note& note)  {
	}

	);

SERVER_N_CLIENT_CODE(
	template <class SerderClassT>
	void Sequence::Note::serderize_single(Note *trgt, SerderClassT& iserder) {
		iserder.process(trgt->note);
		iserder.process(trgt->velocity);
		iserder.process(trgt->start_at);
		iserder.process(trgt->length);
	}

	template <class SerderClassT>
	void Sequence::Note::serderize(SerderClassT& iserder) {
		serderize_single(this, iserder);

		Note **current_note = &(next_note);
		bool has_next_note;
		do {
			has_next_note = (*current_note) != NULL;
			iserder.process(has_next_note);
			if(has_next_note) {
				if((*current_note) == NULL)
					(*current_note) = Note::allocate();
				serderize_single((*current_note), iserder);
				current_note = &((*current_note)->next_note);
			}
		} while(has_next_note);

	}

	Sequence::Note* Sequence::Note::allocate() {
		auto retval = note_allocator.allocate();
		retval->next_note = NULL;
		return retval;
	}

	template <class SerderClassT>
	void Sequence::Pattern::serderize(SerderClassT& iserder) {
		iserder.process(id);
		iserder.process(name);

		bool has_note = first_note != NULL;

		iserder.process(has_note);

		if(has_note)
			iserder.process(first_note);

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

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(const Message &serialized) {
		return std::make_shared<Sequence>(this, serialized);
	}

	std::shared_ptr<BaseObject> Sequence::SequenceFactory::create(int32_t new_obj_id) {
		return std::make_shared<Sequence>(new_obj_id, this);
	}

	static Sequence::SequenceFactory this_will_register_us_as_a_factory;

	ObjectAllocator<Sequence::Pattern> Sequence::pattern_allocator;
	ObjectAllocator<Sequence::PatternInstance> Sequence::pattern_instance_allocator;
	ObjectAllocator<Sequence::Note> Sequence::note_allocator;

	);
