/*
 * vu|KNOB
 * Copyright (C) 2019 by Anton Persson
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

#include "global_control_object.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

SERVER_CODE(
	void GlobalControlObject::handle_req_set_loop_state(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		bool _loop_state = msg.get_value("state") == "true" ? true : false;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_state = _loop_state;
		}
		Machine::set_loop_state(_loop_state);
		send_message(
			cmd_set_loop_state,
			[_loop_state](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("state", _loop_state ? "true" : "false");
			}
			);
	}

	void GlobalControlObject::serialize(std::shared_ptr<Message> &target) {
		Serialize::ItemSerializer iser;
		serderize(iser);
		target->set_value("gco_data", iser.result());
	}

	);

CLIENT_CODE(

	void GlobalControlObject::add_global_control_listener(std::shared_ptr<GlobalControlObject::GlobalControlListener> glol) {
		bool _loop_state;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			gco_listeners.insert(glol);
			_loop_state = loop_state;
		}
		glol->loop_state_changed(_loop_state);
	}

	void GlobalControlObject::set_loop_state(bool new_state) {
		send_message_to_server(
			req_set_loop_state,
			[new_state](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("state", new_state ? "true" : "false");
			}
		);
	}

	void GlobalControlObject::handle_cmd_set_loop_state(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		auto _loop_state = msg.get_value("state") == "true" ? true : false;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_state = _loop_state;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_loop_state() -- %d\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->loop_state_changed(_loop_state);
	}

	);

SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void GlobalControlObject::serderize(SerderClassT& iserder) {
		iserder.process(loop_state);
	}

	GlobalControlObject::GlobalControlObject(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
	{
		register_handlers();
		SATAN_DEBUG("(%s) GCO created - from serialized\n", CLIENTORSERVER_STRING);

		Serialize::ItemDeserializer serder(serialized.get_value("gco_data"));
		serderize(serder);
	}

	GlobalControlObject::GlobalControlObject(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory)
	{
		register_handlers();
		SATAN_DEBUG("(%s) GCO created - from scratch\n", CLIENTORSERVER_STRING);

	}

	std::shared_ptr<BaseObject> GlobalControlObject::GlobalControlObjectFactory::create(const Message &serialized) {
		SATAN_DEBUG("(%s) Factory will create GCO - from serialized\n", CLIENTORSERVER_STRING);
		return std::make_shared<GlobalControlObject>(this, serialized);
	}

	std::shared_ptr<BaseObject> GlobalControlObject::GlobalControlObjectFactory::create(int32_t new_obj_id) {
		SATAN_DEBUG("(%s) Factory will create GCO - from scratch\n", CLIENTORSERVER_STRING);
		return std::make_shared<GlobalControlObject>(new_obj_id, this);
	}

	static GlobalControlObject::GlobalControlObjectFactory this_will_register_us_as_a_factory;

	);
