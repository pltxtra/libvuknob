/*
 * vu|KNOB
 * Copyright (C) 2020 by Anton Persson
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

#include "base_machine.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"


/***************************
 *
 *  Class RemoteInterface::BaseMachine
 *
 ***************************/

SERVER_CODE(
	BaseMachine::Controller::Controller(int _ctrl_id, Machine::Controller *ctrl)
	: m_ctrl(ctrl)
	, ctrl_id(_ctrl_id)
	{
		name = ctrl->get_name();
		group_name = ctrl->get_group_name();
		title = ctrl->get_title();

		{
			switch(ctrl->get_type()) {
			case Machine::Controller::c_int:
				ct_type = Controller::ric_int;
				break;
			case Machine::Controller::c_float:
				ct_type = Controller::ric_float;
				break;
			case Machine::Controller::c_bool:
				ct_type = Controller::ric_bool;
				break;
			case Machine::Controller::c_string:
				ct_type = Controller::ric_string;
				break;
			case Machine::Controller::c_enum:
				ct_type = Controller::ric_enum;
				break;
			case Machine::Controller::c_sigid:
				ct_type = Controller::ric_sigid;
				break;
			case Machine::Controller::c_double:
				ct_type = Controller::ric_double;
				break;
			}
		}

		if(ct_type ==  Controller::ric_float) {
			ctrl->get_min(data.f.min);
			ctrl->get_max(data.f.max);
			ctrl->get_step(data.f.step);
			ctrl->get_value(data.f.value);
		} else if(ct_type == Controller::ric_double) {
			ctrl->get_min(data.d.min);
			ctrl->get_max(data.d.max);
			ctrl->get_step(data.d.step);
			ctrl->get_value(data.d.value);
		} else {
			ctrl->get_min(data.i.min);
			ctrl->get_max(data.i.max);
			ctrl->get_step(data.i.step);
			ctrl->get_value(data.i.value);

			if(ct_type == Controller::ric_enum) {
				for(auto k = data.i.min; k <= data.i.max; k += data.i.step) {
					enum_names[k] = ctrl->get_value_name(k);
				}
			}
		}
		if(ct_type == Controller::ric_string)
			ctrl->get_value(str_data);
		else if(ct_type == Controller::ric_bool)
			ctrl->get_value(bl_data);

		(void) /*ignore return value */ ctrl->has_midi_controller(coarse_controller, fine_controller);
	}

	BaseMachine::Controller::~Controller() {
		delete m_ctrl;
	}

	)

CLIENT_CODE(
	BaseMachine::Controller::Controller() {}

	void BaseMachine::Controller::set_msg_builder(std::function<
						      void(std::function<void(std::shared_ptr<Message> &msg_to_send)> )
						      >  _send_obj_message) {
		send_obj_message = _send_obj_message;
	}

	std::string BaseMachine::Controller::get_name() {
		return name;
	}

	std::string BaseMachine::Controller::get_group() {
		return group_name;
	}

	std::string BaseMachine::Controller::get_title() {
		return title;
	}

	auto BaseMachine::Controller::get_type() -> Type {
		return (Type)ct_type;
	}

	void BaseMachine::Controller::get_min(float &val) {
		val = data.f.min;
	}

	void BaseMachine::Controller::get_max(float &val) {
		val = data.f.max;
	}

	void BaseMachine::Controller::get_step(float &val) {
		val = data.f.step;
	}

	void BaseMachine::Controller::get_min(double &val) {
		val = data.d.min;
	}

	void BaseMachine::Controller::get_max(double &val) {
		val = data.d.max;
	}

	void BaseMachine::Controller::get_step(double &val) {
		val = data.d.step;
	}

	void BaseMachine::Controller::get_min(int &val) {
		val = data.i.min;
	}

	void BaseMachine::Controller::get_max(int &val) {
		val = data.i.max;
	}

	void BaseMachine::Controller::get_step(int &val) {
		val = data.i.step;
	}

	void BaseMachine::Controller::get_value(int &val) {
		val = data.i.value;
	}

	void BaseMachine::Controller::get_value(float &val) {
		val = data.f.value;
	}

	void BaseMachine::Controller::get_value(double &val) {
		val = data.d.value;
	}

	void BaseMachine::Controller::get_value(bool &val) {
		val = bl_data;
	}

	void BaseMachine::Controller::get_value(std::string &val) {
		val = str_data;
	}

	std::string BaseMachine::Controller::get_value_name(int val) {
		if(ct_type == Controller::ric_sigid) {
			std::string retval = "no file loaded";

			auto global_sb = SampleBank::get_bank("");
			if(global_sb) {
				try {
					retval = global_sb->get_sample_name(val);
				} catch(SampleBank::NoSampleLoaded &e) { /* ignore */ }
			}

			return retval;
		}

		auto enam = enum_names.find(val);

		if(enam == enum_names.end())
			throw NoSuchEnumValue();

		return (*enam).second;
	}

	void BaseMachine::Controller::set_value(int val) {
		data.i.value = val;
		auto crid = ctrl_id;
		send_obj_message(
			[crid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("command", "setctrval");
				msg_to_send->set_value("ctrl_id", std::to_string(crid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Controller::set_value(float val) {
		data.f.value = val;
		auto crid = ctrl_id;
		send_obj_message(
			[crid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("command", "setctrval");
				msg_to_send->set_value("ctrl_id", std::to_string(crid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Controller::set_value(double val) {
		data.d.value = val;
		auto crid = ctrl_id;
		send_obj_message(
			[crid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("command", "setctrval");
				msg_to_send->set_value("ctrl_id", std::to_string(crid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Controller::set_value(bool val) {
		bl_data = val;
		auto crid = ctrl_id;
		send_obj_message(
			[crid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("command", "setctrval");
				msg_to_send->set_value("ctrl_id", std::to_string(crid));
				msg_to_send->set_value("value", val ? "true" : "false");
			}
			);
	}

	void BaseMachine::Controller::set_value(const std::string &val) {
		str_data = val;
		auto crid = ctrl_id;
		send_obj_message(
			[crid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("command", "setctrval");
				msg_to_send->set_value("ctrl_id", std::to_string(crid));
				msg_to_send->set_value("value", val);
			}
			);
	}

	bool BaseMachine::Controller::has_midi_controller(int &__coarse_controller, int &__fine_controller) {
		if(coarse_controller == -1 && fine_controller == -1) return false;

		__coarse_controller = coarse_controller;
		__fine_controller = fine_controller;

		return true;
	}

	)

SERVER_N_CLIENT_CODE(
	template <class SerderClassT>
	void BaseMachine::Controller::serderize(SerderClassT& iserder) {
		iserder.process(name);
		iserder.process(group_name);
		iserder.process(title);

		iserder.process(ct_type);

		switch(ct_type) {
		case Controller::ric_float: {
			iserder.process(data.f.min);
			iserder.process(data.f.max);
			iserder.process(data.f.step);
			iserder.process(data.f.value);
		} break;

		case Controller::ric_double: {
			iserder.process(data.d.min);
			iserder.process(data.d.max);
			iserder.process(data.d.step);
			iserder.process(data.d.value);
		} break;

		case Controller::ric_int:
		case Controller::ric_enum:
		case Controller::ric_sigid: {
			iserder.process(data.i.min);
			iserder.process(data.i.max);
			iserder.process(data.i.step);
			iserder.process(data.i.value);

			if(ct_type == Controller::ric_enum) {
				iserder.process(enum_names);
			}
		} break;

		case Controller::ric_bool: {
			iserder.process(bl_data);
		} break;

		case Controller::ric_string: {
			iserder.process(str_data);
		} break;

		}

		iserder.process(coarse_controller);
		iserder.process(fine_controller);
	}
	);

/***************************
 *
 *  Class RemoteInterface::BaseMachine
 *
 ***************************/

SERVER_N_CLIENT_CODE(
	template <class SerderClassT>
	void BaseMachine::serderize_base_machine(SerderClassT& iserder) {
		iserder.process(name);
		iserder.process(groups);
		iserder.process(controllers);
		SATAN_DEBUG("serderize_base_machine() -- base_machine_name: %s\n",
			    name.c_str());
	}
	);

CLIENT_CODE(
	BaseMachine::BaseMachine(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
	{
		register_handlers();

		Serialize::ItemDeserializer serder(serialized.get_value("basemachine_data"));
		serderize_base_machine(serder);

		SATAN_DEBUG("BaseMachine(%s) created client side:\n", name.c_str());
		for(auto group : groups) {
			SATAN_DEBUG("  : %s\n", group.c_str());
		}

		SATAN_DEBUG("BaseMachine(%s) controllers:\n", name.c_str());
		for(auto ctrl : controllers) {
			auto n = ctrl->get_name().c_str();
			auto g = ctrl->get_group().c_str();
			SATAN_DEBUG("  : %s (%s)\n", n, g);
			ctrl->set_msg_builder(
				[this](std::function<void(std::shared_ptr<Message> &)> fill_in_msg) {
					send_object_message(fill_in_msg);
				}
				);
		}
	}

	);

SERVER_CODE(
	BaseMachine::BaseMachine(int32_t new_obj_id, const Factory *factory)
	: SimpleBaseObject(new_obj_id, factory)
	{
		register_handlers();
		SATAN_DEBUG("BaseMachine() created server side.\n");
	}

	void BaseMachine::serialize(std::shared_ptr<Message> &target) {
		Serialize::ItemSerializer iser;
		serderize_base_machine(iser);
		target->set_value("basemachine_data", iser.result());
	}

	void BaseMachine::init_from_machine_ptr(std::shared_ptr<Machine> m_ptr) {
		name = m_ptr->get_name();
		groups = m_ptr->get_controller_groups();
		int ctrl_id = 0;
		for(auto ctrname : m_ptr->get_controller_names()) {
			SATAN_DEBUG(" --- adding ctr %s\n", ctrname.c_str());
			auto m_ctrl = m_ptr->get_controller(ctrname);
			SATAN_DEBUG(" .. m_ctrl is %p\n", m_ctrl);
			auto c = std::make_shared<Controller>(ctrl_id++, m_ctrl);
			controllers.insert(c);
		}
	}
	);
