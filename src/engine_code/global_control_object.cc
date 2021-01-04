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

//#define __DO_SATAN_DEBUG
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

	void GlobalControlObject::playback_state_changed(bool _playing) {
		SATAN_ERROR("GlobalControlObject::playback_state_changed() called...\n");
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			playing = _playing;
		}
		send_message(
			cmd_set_playback_state,
			[_playing](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("playing", _playing ? "true" : "false");
				SATAN_ERROR("GlobalControlObject::playback_state_changed() sending command...\n");
			}
			);
	}

	void GlobalControlObject::record_state_changed(bool _record) {
		SATAN_ERROR("GlobalControlObject::record_state_changed() called...\n");
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			record = _record;
		}
		send_message(
			cmd_set_record_state,
			[_record](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("record", _record ? "true" : "false");
				SATAN_ERROR("GlobalControlObject::record_state_changed() sending command...\n");
			}
			);
	}

	void GlobalControlObject::bpm_changed(int _bpm) {
		SATAN_ERROR("GlobalControlObject::bpm_changed() --- %d\n", _bpm);
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			bpm = _bpm;
		}
		send_message(
			cmd_set_bpm,
			[_bpm](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("bpm", std::to_string(_bpm));
				SATAN_ERROR("GlobalControlObject::bpm_changed() sending command... %d\n", _bpm);
			}
			);
	}

	void GlobalControlObject::lpb_changed(int _lpb) {
		SATAN_ERROR("GlobalControlObject::lpb_changed() --- %d\n", _lpb);
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			lpb = _lpb;
		}
		send_message(
			cmd_set_lpb,
			[_lpb](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("lpb", std::to_string(_lpb));
				SATAN_ERROR("GlobalControlObject::lpb_changed() sending command... %d\n", _lpb);
			}
			);
	}

	void GlobalControlObject::shuffle_factor_changed(int _sf) {
		SATAN_ERROR("GlobalControlObject::shuffle_factor_changed() --- %d\n", _sf);
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			shuffle_factor = _sf;
		}
		send_message(
			cmd_set_sf,
			[_sf](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("sf", std::to_string(_sf));
				SATAN_ERROR("GlobalControlObject::shuffle_factor_changed() sending command... %d\n", _sf);
			}
			);
	}

	void GlobalControlObject::send_periodic_update(int row) {
		send_message(
			cmd_per_upd,
			[row](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("row", std::to_string(row));
				SATAN_DEBUG("GlobalControlObject::send_periodic_update(%d) sending command...\n", row);
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

	void GlobalControlObject::handle_req_set_playback_state(RemoteInterface::Context *context,
								RemoteInterface::MessageHandler *src,
								const RemoteInterface::Message& msg) {
		SATAN_ERROR("GlobalControlObject::handle_req_set_playback_state() called...\n");
		bool _playing = msg.get_value("playing") == "true" ? true : false;
		try {
			if(_playing)
				Machine::play();
			else
				Machine::stop();
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_record_state(RemoteInterface::Context *context,
								 RemoteInterface::MessageHandler *src,
								 const RemoteInterface::Message& msg) {
		SATAN_ERROR("GlobalControlObject::handle_req_set_record_state() called...\n");
		bool _record = msg.get_value("record") == "true" ? true : false;
		try {
			Machine::set_record_state(_record);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_bpm(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		int _bpm = std::stoi(msg.get_value("bpm"));
		SATAN_ERROR("GlobalControlObject::handle_req_set_bpm() --- %d\n", _bpm);
		try {
			Machine::set_bpm(_bpm);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_lpb(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		int _lpb = std::stoi(msg.get_value("lpb"));
		SATAN_ERROR("GlobalControlObject::handle_req_set_lpb() --- %d\n", _lpb);
		try {
			Machine::set_lpb(_lpb);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_set_sf(RemoteInterface::Context *context,
						    RemoteInterface::MessageHandler *src,
						    const RemoteInterface::Message& msg) {
		int _sf = std::stoi(msg.get_value("sf"));
		SATAN_ERROR("GlobalControlObject::handle_req_set_sf() --- %d\n", _sf);
		try {
			Machine::set_shuffle_factor(_sf);
		} catch(...) { /* ignore */ }
	}

	void GlobalControlObject::handle_req_jump_to(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		int to_row = std::stoi(msg.get_value("jump"));
		SATAN_ERROR("GlobalControlObject::handle_req_jump_to() --- %d\n", to_row);
		try {
			Machine::jump_to(to_row);
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
		bool _loop_state, _playing, _record;
		int _loop_start, _loop_length, _bpm, _lpb;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			gco_listeners.insert(glol);
			_loop_state = loop_state;
			_loop_start = loop_start;
			_loop_length = loop_length;
			_playing = playing;
			_record = record;
			_bpm = bpm;
			_lpb = lpb;
		}
		glol->loop_state_changed(_loop_state);
		glol->loop_start_changed(_loop_start);
		glol->loop_length_changed(_loop_length);
		glol->playback_state_changed(_playing);
		glol->record_state_changed(_record);
		glol->bpm_changed(_bpm);
		glol->lpb_changed(_lpb);
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

	bool GlobalControlObject::is_playing() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return playing;
	}

	void GlobalControlObject::play() {
		send_message_to_server(
			req_set_playback_state,
			[](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("playing", "true");
			}
		);
	}

	void GlobalControlObject::stop() {
		send_message_to_server(
			req_set_playback_state,
			[](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("playing", "false");
			}
		);
	}

	void GlobalControlObject::jump(int to_row) {
		send_message_to_server(
			req_jump_to,
			[to_row](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_DEBUG("GlobalControlObject::jump(%d)\n", to_row);
				msg2send->set_value("jump", std::to_string(to_row));
			}
		);
	}

	void GlobalControlObject::set_record_state(bool _record) {
		send_message_to_server(
			req_set_record_state,
			[_record](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("record", _record ? "true" : "false");
			}
		);
	}

	void GlobalControlObject::set_bpm(int new_bpm) {
		send_message_to_server(
			req_set_bpm,
			[new_bpm](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_ERROR("GlobalControlObject::set_bpm() sending new bpm value: %d\n", new_bpm);
				msg2send->set_value("bpm", std::to_string(new_bpm));
			}
		);
	}

	void GlobalControlObject::set_lpb(int new_lpb) {
		send_message_to_server(
			req_set_lpb,
			[new_lpb](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_ERROR("GlobalControlObject::set_lpb() sending new lpb value: %d\n", new_lpb);
				msg2send->set_value("lpb", std::to_string(new_lpb));
			}
		);
	}

	void GlobalControlObject::set_shuffle_factor(int new_sf) {
		SATAN_ERROR("GlobalControlObject::set_shuffle_factor(%d)\n", new_sf);
		send_message_to_server(
			req_set_sf,
			[new_sf](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				SATAN_ERROR("GlobalControlObject::set_shuffle_factor() sending new sf value: %d\n", new_sf);
				msg2send->set_value("sf", std::to_string(new_sf));
			}
		);
	}

	int GlobalControlObject::get_bpm() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return bpm;
	}

	int GlobalControlObject::get_lpb() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return lpb;
	}

	int GlobalControlObject::get_shuffle_factor() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return shuffle_factor;
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

		SATAN_ERROR("(client) ::handle_cmd_set_loop_state() -- %zu\n", gco_listeners.size());

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

		SATAN_ERROR("(client) ::handle_cmd_set_loop_start() -- %zu\n", gco_listeners.size());

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

		SATAN_ERROR("(client) ::handle_cmd_set_loop_length() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->loop_length_changed(_loop_length);
	}

	void GlobalControlObject::handle_cmd_set_playback_state(RemoteInterface::Context *context,
								RemoteInterface::MessageHandler *src,
								const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		auto _playing = msg.get_value("playing") == "true" ? true : false;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			playing = _playing;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_playback_state() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->playback_state_changed(_playing);
	}

	void GlobalControlObject::handle_cmd_set_record_state(RemoteInterface::Context *context,
								 RemoteInterface::MessageHandler *src,
								 const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		auto _record = msg.get_value("record") == "true" ? true : false;
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			record = _record;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_record_state() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->record_state_changed(_record);
	}

	void GlobalControlObject::handle_cmd_set_bpm(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _bpm = std::stoi(msg.get_value("bpm"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			bpm = _bpm;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_bpm() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->bpm_changed(_bpm);
	}

	void GlobalControlObject::handle_cmd_set_lpb(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _lpb = std::stoi(msg.get_value("lpb"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			lpb = _lpb;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_lpb() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->lpb_changed(_lpb);
	}

	void GlobalControlObject::handle_cmd_set_sf(RemoteInterface::Context *context,
						    RemoteInterface::MessageHandler *src,
						    const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _sf = std::stoi(msg.get_value("sf"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			shuffle_factor = _sf;
			_gco_listeners = gco_listeners;
		}

		SATAN_ERROR("(client) ::handle_cmd_set_sf() -- %zu\n", gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->shuffle_factor_changed(_sf);
	}

	void GlobalControlObject::handle_cmd_per_upd(RemoteInterface::Context *context,
						     RemoteInterface::MessageHandler *src,
						     const RemoteInterface::Message& msg) {
		std::set<std::shared_ptr<GlobalControlListener> > _gco_listeners;
		int _row = std::stoi(msg.get_value("row"));
		{
			std::lock_guard<std::mutex> lock_guard(base_object_mutex);
			_gco_listeners = gco_listeners;
		}

		SATAN_DEBUG("(client) ::handle_cmd_row_upd(%d) -- %zu\n", _row, gco_listeners.size());

		for(auto glol : _gco_listeners)
			glol->row_update(_row);
	}

	);

SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void GlobalControlObject::serderize(SerderClassT& iserder) {
		iserder.process(loop_state);
		iserder.process(playing);
		iserder.process(record);
		iserder.process(loop_start);
		iserder.process(loop_length);
		iserder.process(bpm);
		iserder.process(lpb);
		iserder.process(shuffle_factor);
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
		auto new_gco = std::make_shared<GlobalControlObject>(new_obj_id, this);
		auto o = register_object(new_gco);
		ON_SERVER(
			auto casted = std::dynamic_pointer_cast<PlaybackStateListener>(o);
			Machine::register_playback_state_listener(casted);
			Machine::register_periodic(
				[new_gco](int row) {
					new_gco->send_periodic_update(row);
				}
				);
			);
		return o;
	}

	static GlobalControlObject::GlobalControlObjectFactory this_will_register_us_as_a_factory;

	);
