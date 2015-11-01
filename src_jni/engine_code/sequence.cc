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

	void Sequence::handle_req_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto new_name = msg.get_value("name");
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

	void Sequence::handle_req_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto pattern_id = std::stol(msg.get_value("pattern_id"));
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

	void Sequence::handle_req_add_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
	}

	void Sequence::handle_req_del_pattern_instance(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
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

	);

CLIENT_CODE(

	void Sequence::handle_cmd_add_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto new_name = msg.get_value("name");
		auto new_id = std::stol(msg.get_value("pattern_id"));
		auto ptrn = pattern_allocator.allocate();

		ptrn->id = new_id;
		ptrn->name = new_name;
		ptrn->first_note = NULL;

		patterns[ptrn->id] = ptrn;
	}

	void Sequence::handle_cmd_del_pattern(RemoteInterface::Context *context,
					      RemoteInterface::MessageHandler *src,
					      const RemoteInterface::Message& msg) {
		auto pattern_id = std::stol(msg.get_value("pattern_id"));
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

	Sequence::Sequence(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized) {
		register_handlers();
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
	ObjectAllocator<Sequence::Note> Sequence::note_allocator;

	);
