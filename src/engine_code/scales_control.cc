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

#include "scales_control.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

SERVER_CODE(
	void ScalesControl::handle_req_set_custom_scale(RemoteInterface::Context *context,
							RemoteInterface::MessageHandler *src,
							const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		auto content = msg.get_value("content");
		Serialize::ItemDeserializer serder(content);
		serder.process(custom_scale);
		send_message(
			cmd_set_custom_scale,
			[content](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("content", content);
			}
			);
	}

	void ScalesControl::serialize(std::shared_ptr<Message> &target) {
		Serialize::ItemSerializer iser;
		serderize(iser);
		target->set_value("scalescontrol_data", iser.result());
	}

	std::shared_ptr<ScalesControl> ScalesControl::get_scales_object_serverside() {
		return serverside_scales_control_reference.lock();
	}

	);

CLIENT_CODE(
	void ScalesControl::handle_cmd_set_custom_scale(RemoteInterface::Context *context,
							RemoteInterface::MessageHandler *src,
							const RemoteInterface::Message& msg) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		Serialize::ItemDeserializer serder(msg.get_value("content"));
		serder.process(custom_scale);
	}
	);

SERVER_N_CLIENT_CODE(

	const char* ScalesControl::get_key_text(int key) {
		static const char *key_text[] = {
			"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
		};
		return key_text[key % 12];
	}

	int ScalesControl::get_number_of_scales() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		return scales.size() + 1;
	}

	std::vector<std::string> ScalesControl::get_scale_names() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		std::vector<std::string> names;
		for(auto s : scales) {
			names.push_back(s.name);
		}
		return names;
	}

	std::vector<int> ScalesControl::get_scale_keys(int index) {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		std::vector<int> keys = {0, 2, 4, 5, 7, 9, 11};
		if((size_t)index == scales.size()) {
			keys.clear();
			for(auto v : custom_scale.keys) {
				keys.push_back(v);
			}
		} else {
			try {
				auto s = scales.at(index);
				keys.clear();
				for(auto v : s.keys) {
					keys.push_back(v);
				}
			} catch(std::out_of_range const& exc) { /* ignore */ }
		}
		return keys;
	}

	std::vector<int> ScalesControl::get_custom_scale_keys() {
		std::lock_guard<std::mutex> lock_guard(base_object_mutex);
		std::vector<int> keys;
		for(auto v : custom_scale.keys) {
			keys.push_back(v);
		}
		return keys;
	}

	void ScalesControl::set_custom_scale_keys(std::vector<int> new_custom_scale_values) {
		Scale new_custom_scale;
		new_custom_scale.name = "custom";
		for(int k = 0; k < 7; k++) {
			new_custom_scale.keys[k] = new_custom_scale_values[k];
		}
		Serialize::ItemSerializer iser;
		iser.process(new_custom_scale);
		auto iser_result = iser.result();
		send_message_to_server(
			req_set_custom_scale,
			[iser_result](std::shared_ptr<RemoteInterface::Message> &msg2send) {
				msg2send->set_value("content", iser_result);
			}
			);
	}

	static bool scales_library_initialized = false;

	struct ScaleEntry {
		int offset;
		const char *name;
		int keys[21];
	};

	static struct ScaleEntry scales_library[] = {
		{ 0x0, "C- ", {0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x0, "C-m", {0x0, 0x2, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x0, "C#m", {0x1, 0x3, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x1, 0x3, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x1, "D- ", {0x1, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x1, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x1, "D-m", {0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x1, "Db ", {0x0, 0x1, 0x3, 0x5, 0x6, 0x8, 0xa,
			       0x0, 0x1, 0x3, 0x5, 0x6, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x1, "D#m", {0x1, 0x3, 0x5, 0x6, 0x8, 0xa, 0xb,
			       0x1, 0x3, 0x5, 0x6, 0x8, 0xa, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x2, "E- ", {0x1, 0x3, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x1, 0x3, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x2, "E-m", {0x0, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x2, "Eb ", {0x0, 0x2, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x3, "F- ", {0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x3, "F-m", {0x0, 0x1, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x1, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x3, "F# ", {0x1, 0x3, 0x5, 0x6, 0x8, 0xa, 0xb,
			       0x1, 0x3, 0x5, 0x6, 0x8, 0xa, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x3, "F#m", {0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x4, "G- ", {0x0, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x4, "G-m", {0x0, 0x2, 0x3, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x3, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x4, "G#m", {0x1, 0x3, 0x4, 0x6, 0x8, 0xa, 0xb,
			       0x1, 0x3, 0x4, 0x6, 0x8, 0xa, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x5, "A- ", {0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x5, "A-m", {0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x5, "Ab ", {0x0, 0x1, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x1, 0x3, 0x5, 0x7, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x6, "B- ", {0x1, 0x3, 0x4, 0x6, 0x8, 0xa, 0xb,
			       0x1, 0x3, 0x4, 0x6, 0x8, 0xa, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x6, "B-m", {0x1, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x1, 0x2, 0x4, 0x6, 0x7, 0x9, 0xb,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x6, "Bb ", {0x0, 0x2, 0x3, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x3, 0x5, 0x7, 0x9, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
		{ 0x6, "Bbm", {0x0, 0x1, 0x3, 0x5, 0x6, 0x8, 0xa,
			       0x0, 0x1, 0x3, 0x5, 0x6, 0x8, 0xa,
			       0x0, 0x2, 0x4, 0x5, 0x7, 0x9, 0xb
			} },
	};

	static void initialize_scale(ScaleEntry *s) {
		for(int x = 0; x < 7; x++) {
			s->keys[x +  7] = s->keys[x] + 12;
			s->keys[x + 14] = s->keys[x] + 24;
		}
	}

	static int max_scales = -1;

	static inline void initialize_scales_library() {
		if(scales_library_initialized) return;

		scales_library_initialized = true;

		int k;

		max_scales = sizeof(scales_library) / sizeof(ScaleEntry);

		for(k = 0; k < max_scales; k++) {
			initialize_scale(&(scales_library[k]));
		}
	}

	void ScalesControl::initialize_scales() {
		initialize_scales_library();

		scales.clear();

		// insert standard scales
		for(auto k = 0; k < max_scales; k++) {
			Scale scl;
			scl.name = scales_library[k].name;
			for(auto l = 0; l < 7; l++) {
				scl.keys[l] = scales_library[k].keys[l + scales_library[k].offset];
			}
			scales.push_back(scl);
		}

		// insert custom scale
		{
			custom_scale.name = "CS1";
			custom_scale.keys[0] =  0;
			custom_scale.keys[1] =  2;
			custom_scale.keys[2] =  4;
			custom_scale.keys[3] =  5;
			custom_scale.keys[4] =  7;
			custom_scale.keys[5] =  9;
			custom_scale.keys[6] = 11;
		}
	}

	template <class SerderClassT>
	void ScalesControl::serderize(SerderClassT& iserder) {
		iserder.process(custom_scale);
		iserder.process(scales);
	}

	ScalesControl::ScalesControl(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
	{
		register_handlers();
		SATAN_DEBUG("(%s) ScalesControl created - from serialized\n", CLIENTORSERVER_STRING);

		Serialize::ItemDeserializer serder(serialized.get_value("scalescontrol_data"));
		serderize(serder);
	}

	ScalesControl::ScalesControl(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory)
	{
		initialize_scales();
		register_handlers();
		SATAN_DEBUG("(%s) ScalesControl created - from scratch\n", CLIENTORSERVER_STRING);
	}

	std::shared_ptr<BaseObject> ScalesControl::ScalesControlFactory::create(
		const Message &serialized,
		RegisterObjectFunction register_object
		) {
		SATAN_DEBUG("(%s) Factory will create ScalesControl - from serialized\n", CLIENTORSERVER_STRING);
		return register_object(std::make_shared<ScalesControl>(this, serialized));
	}

	ON_SERVER(
		std::weak_ptr<ScalesControl> ScalesControl::serverside_scales_control_reference;
		)
	std::shared_ptr<BaseObject> ScalesControl::ScalesControlFactory::create(
		int32_t new_obj_id,
		RegisterObjectFunction register_object
		) {
		SATAN_DEBUG("(%s) Factory will create ScalesControl - from scratch\n", CLIENTORSERVER_STRING);
		auto new_sc = std::make_shared<ScalesControl>(new_obj_id, this);
		ON_SERVER(
			serverside_scales_control_reference = new_sc;
			);
		return register_object(new_sc);
	}

	static ScalesControl::ScalesControlFactory this_will_register_us_as_a_factory;

	);
