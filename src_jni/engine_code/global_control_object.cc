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
	void GlobalControlObject::loop_state_changed(bool _loop_state) {
		SATAN_ERROR("GlobalControlObject::loop_state_changed() called...\n");
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_state = _loop_state;
		}
		send_message(
			cmd_set_loop_state,
			[_loop_state](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("state", _loop_state ? "true" : "false");
				SATAN_ERROR("GlobalControlObject::loop_state_changed() sending command...\n");
			}
			);
	}

	void GlobalControlObject::loop_start_changed(int _loop_start) {
		SATAN_ERROR("GlobalControlObject::loop_start_changed() --- %d\n", _loop_start);
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_start = _loop_start;
		}
		send_message(
			cmd_set_loop_start,
			[_loop_start](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("start", std::to_string(_loop_start));
				SATAN_ERROR("GlobalControlObject::loop_start_changed() sending command... %d\n", _loop_start);
			}
			);
	}

	void GlobalControlObject::loop_length_changed(int _loop_length) {
		SATAN_ERROR("GlobalControlObject::loop_length_changed() --- %d\n", _loop_length);
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_length = _loop_length;
		}
		send_message(
			cmd_set_loop_length,
			[_loop_length](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("length", std::to_string(_loop_length));
				SATAN_ERROR("GlobalControlObject::loop_length_changed() sending command... %d\n", _loop_length);
			}
			);
	}

	void GlobalControlObject::handle_req_set_loop_state(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		SATAN_ERROR("GlobalControlObject::handle_req_set_loop_state() called...\n");
		bool _loop_state = msg.get_value("state") == "true" ? true : false;
		try {
			Machine::set_loop_state(_loop_state);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_loop_start(RemoteInterface::Context *context,
							    RemoteInterface::MessageHandler *src,
							    const RemoteInterface::Message& msg) {
		int _loop_start = std::stoi(msg.get_value("start"));
		SATAN_ERROR("GlobalControlObject::handle_req_set_loop_start() --- %d\n", _loop_start);
		try {
			Machine::set_loop_start(_loop_start);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_loop_length(RemoteInterface::Context *context,
							     RemoteInterface::MessageHandler *src,
							     const RemoteInterface::Message& msg) {
		int _loop_length = std::stoi(msg.get_value("length"));
		SATAN_ERROR("GlobalControlObject::handle_req_set_loop_length() --- %d\n", _loop_length);
		try {
			Machine::set_loop_length(_loop_length);
		} catch(...) { /* ignore */ }
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

	void GlobalControlObject::set_loop_start(int new_start) {
		send_message_to_server(
			req_set_loop_start,
			[new_start](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_ERROR("GlobalControlObject::set_loop_start() sending new start value: %d\n", new_start);
				msg2send->set_value("start", std::to_string(new_start));
			}
		);
	}

	void GlobalControlObject::set_loop_length(int new_length) {
		send_message_to_server(
			req_set_loop_length,
			[new_length](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_ERROR("GlobalControlObject::set_loop_length() sending new length value: %d\n", new_length);
				msg2send->set_value("length", std::to_string(new_length));
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

	void GlobalControlObject::handle_cmd_set_loop_start(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _loop_start = std::stoi(msg.get_value("start"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_start = _loop_start;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_loop_start() -- %d\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->loop_start_changed(_loop_start);
	}

	void GlobalControlObject::handle_cmd_set_loop_length(RemoteInterface::Context *context,
							   RemoteInterface::MessageHandler *src,
							   const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _loop_length = std::stoi(msg.get_value("length"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			loop_length = _loop_length;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_loop_length() -- %d\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->loop_length_changed(_loop_length);
	}

	);

SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void GlobalControlObject::serderize(SerderClassT& iserder) {
		iserder.process(loop_state);
		iserder.process(loop_start);
		iserder.process(loop_length);
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

	std::shared_ptr<BaseObject> GlobalControlObject::GlobalControlObjectFactory::create(
		const Message &serialized,
		RegisterObjectFunction register_object
		) {
		SATAN_DEBUG("(%s) Factory will create GCO - from serialized\n", CLIENTORSERVER_STRING);
		return register_object(std::make_shared<GlobalControlObject>(this, serialized));
	}

	std::shared_ptr<BaseObject> GlobalControlObject::GlobalControlObjectFactory::create(
		int32_t new_obj_id,
		RegisterObjectFunction register_object
		) {
		SATAN_DEBUG("(%s) Factory will create GCO - from scratch\n", CLIENTORSERVER_STRING);
		auto o = register_object(std::make_shared<GlobalControlObject>(new_obj_id, this));
		ON_SERVER(
			auto casted = std::dynamic_pointer_cast<PlaybackStateListener>(o);
			Machine::register_playback_state_listener(casted);
			);
		return o;
	}

	static GlobalControlObject::GlobalControlObjectFactory this_will_register_us_as_a_factory;

	);
