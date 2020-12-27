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

#include "handle_list.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

CLIENT_CODE(

	std::weak_ptr<HandleList> HandleList::clientside_handle_list;

	std::map<std::string, std::string> HandleList::get_handles_and_hints() {
		if(auto hl = clientside_handle_list.lock()) {
			std::lock_guard<std::mutex> lock_guard(hl->base_object_mutex);
			return hl->handle2hint;
		}
		return std::map<std::string, std::string>();
	}

	void HandleList::create_instance(const std::string &handle, double xpos, double ypos, bool autoconnect) {
		if(auto hl = clientside_handle_list.lock()) {
			hl->send_message_to_server(
				req_create_machine_instance,
				[handle, xpos, ypos, autoconnect]
				(std::shared_ptr<RemoteInterface::Message> &msg2send) {
					SATAN_DEBUG("HandleList::create_instance() filling in...\n");
					msg2send->set_value("handle", handle);
					msg2send->set_value("xpos", std::to_string(xpos));
					msg2send->set_value("ypos", std::to_string(ypos));
					msg2send->set_value("autoconnect", autoconnect ? "true" : "false");
					SATAN_DEBUG("HandleList::create_instance() filled in!\n");
				}
				);
		}
	}

	HandleList::HandleList(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
	{
		register_handlers();
		Serialize::ItemDeserializer serder(serialized.get_value("serderized_data"));
		serderize(serder);
	}

	);

SERVER_CODE(

	void HandleList::handle_req_create_machine_instance(RemoteInterface::Context *context,
							    RemoteInterface::MessageHandler *__src,
							    const RemoteInterface::Message& msg) {
		auto handle = msg.get_value("handle");
		auto xpos = std::stod(msg.get_value("xpos"));
		auto ypos = std::stod(msg.get_value("ypos"));
		bool autoconnect = msg.get_value("autoconnect") == "true" ? true : false;

		auto m = DynamicMachine::instance(handle, (float)xpos, (float)ypos);
		auto sink = Machine::get_sink();
		if(m && sink && autoconnect) {
			std::vector<std::string> inputs = sink->get_input_names();
			std::vector<std::string> outputs = m->get_output_names();

			// currently we are not smart enough to find matching sockets unless there are only one output and one input...
			if(inputs.size() != 1 || outputs.size() != 1) {
				SATAN_ERROR("BaseMachine::handle_req_create_machine_instance() - auto_connect: input.size() = %d, output.size() = %d.\n", inputs.size(), outputs.size());
			} else {
				// OK, so there is only one possible connection to make.. let's try!
				try {
					sink->attach_input(m, outputs[0], inputs[0]);
				} catch(jException e) {
					SATAN_ERROR("BaseMachine::handle_req_create_machine_instance() - auto_connect error - %s.\n", e.message.c_str());
				} catch(...) {
					SATAN_ERROR("BaseMachine::handle_req_create_machine_instance() - unknown auto_connect error.\n");
				}
			}
		}
	}

	void HandleList::serialize(std::shared_ptr<Message> &target) {
		Serialize::ItemSerializer iser;
		serderize(iser);
		target->set_value("serderized_data", iser.result());
	}

	HandleList::HandleList(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory)
	{
		register_handlers();
		SATAN_DEBUG("HandleList() created server side.\n");
	}

	);

SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void HandleList::serderize(SerderClassT& iserder) {
		iserder.process(handle2hint);
		SATAN_DEBUG("[%s] HandleList::serderize() handle2hint size: %d\n", CLIENTORSERVER_STRING, handle2hint.size());
	}

	std::shared_ptr<BaseObject> HandleList::HandleListFactory::create(
		const Message &serialized,
		RegisterObjectFunction register_object
		) {
		ON_CLIENT(
			auto hl = std::make_shared<HandleList>(this, serialized);
			clientside_handle_list = hl;
			return register_object(hl);
			);
		ON_SERVER(
			return nullptr;
			);
	}

	std::shared_ptr<BaseObject> HandleList::HandleListFactory::create(
		int32_t new_obj_id,
		RegisterObjectFunction register_object
		) {
		ON_SERVER(
			auto hl = std::make_shared<HandleList>(new_obj_id, this);
			DynamicMachine::refresh_handle_set();
			std::set<std::string> handles = DynamicMachine::get_handle_set();
			SATAN_DEBUG("[%s] - HandleList got %d handles'n'hints.", CLIENTORSERVER_STRING, handles.size());
			for(auto handle : handles) {
				auto hint = DynamicMachine::get_handle_hint(handle);
				SATAN_DEBUG("[%s] - handles2hint[%s] = %s", CLIENTORSERVER_STRING, handle.c_str(), hint.c_str());
				hl->handle2hint[handle] = hint;
			}
			return register_object(hl);
			);
		ON_CLIENT(
			return nullptr;
			);
	}

	static HandleList::HandleListFactory this_will_register_us_as_a_factory;

	);
