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
	BaseMachine::Knob::Knob(int _knb_id, Machine::Controller *ctrl)
	: m_ctrl(ctrl)
	, knb_id(_knb_id)
	{
		name = ctrl->get_name();
		group_name = ctrl->get_group_name();
		title = ctrl->get_title();

		{
			switch(ctrl->get_type()) {
			case Machine::Controller::c_int:
				k_type = Knob::rik_int;
				break;
			case Machine::Controller::c_float:
				k_type = Knob::rik_float;
				break;
			case Machine::Controller::c_bool:
				k_type = Knob::rik_bool;
				break;
			case Machine::Controller::c_string:
				k_type = Knob::rik_string;
				break;
			case Machine::Controller::c_enum:
				k_type = Knob::rik_enum;
				break;
			case Machine::Controller::c_sigid:
				k_type = Knob::rik_sigid;
				break;
			case Machine::Controller::c_double:
				k_type = Knob::rik_double;
				break;
			}
		}

		if(k_type ==  Knob::rik_float) {
			ctrl->get_min(data.f.min);
			ctrl->get_max(data.f.max);
			ctrl->get_step(data.f.step);
			ctrl->get_value(data.f.value);
		} else if(k_type == Knob::rik_double) {
			ctrl->get_min(data.d.min);
			ctrl->get_max(data.d.max);
			ctrl->get_step(data.d.step);
			ctrl->get_value(data.d.value);
		} else {
			ctrl->get_min(data.i.min);
			ctrl->get_max(data.i.max);
			ctrl->get_step(data.i.step);
			ctrl->get_value(data.i.value);

			if(k_type == Knob::rik_enum) {
				for(auto k = data.i.min; k <= data.i.max; k += data.i.step) {
					enum_names[k] = ctrl->get_value_name(k);
				}
			}
		}
		if(k_type == Knob::rik_string)
			ctrl->get_value(str_data);
		else if(k_type == Knob::rik_bool)
			ctrl->get_value(bl_data);

		(void) /*ignore return value */ ctrl->has_midi_controller(coarse_controller, fine_controller);
	}

	BaseMachine::Knob::~Knob() {
		delete m_ctrl;
	}

	)

CLIENT_CODE(
	BaseMachine::Knob::Knob() {}

	void BaseMachine::Knob::set_msg_builder(std::function<
						      void(std::function<void(std::shared_ptr<Message> &msg_to_send)> )
						      >  _send_obj_message) {
		send_obj_message = _send_obj_message;
	}

	std::string BaseMachine::Knob::get_name() {
		return name;
	}

	std::string BaseMachine::Knob::get_group() {
		return group_name;
	}

	std::string BaseMachine::Knob::get_title() {
		return title;
	}

	auto BaseMachine::Knob::get_type() -> Type {
		return (Type)k_type;
	}

	void BaseMachine::Knob::get_min(float &val) {
		val = data.f.min;
	}

	void BaseMachine::Knob::get_max(float &val) {
		val = data.f.max;
	}

	void BaseMachine::Knob::get_step(float &val) {
		val = data.f.step;
	}

	void BaseMachine::Knob::get_min(double &val) {
		val = data.d.min;
	}

	void BaseMachine::Knob::get_max(double &val) {
		val = data.d.max;
	}

	void BaseMachine::Knob::get_step(double &val) {
		val = data.d.step;
	}

	void BaseMachine::Knob::get_min(int &val) {
		val = data.i.min;
	}

	void BaseMachine::Knob::get_max(int &val) {
		val = data.i.max;
	}

	void BaseMachine::Knob::get_step(int &val) {
		val = data.i.step;
	}

	void BaseMachine::Knob::get_value(int &val) {
		val = data.i.value;
	}

	void BaseMachine::Knob::get_value(float &val) {
		val = data.f.value;
	}

	void BaseMachine::Knob::get_value(double &val) {
		val = data.d.value;
	}

	void BaseMachine::Knob::get_value(bool &val) {
		val = bl_data;
	}

	void BaseMachine::Knob::get_value(std::string &val) {
		val = str_data;
	}

	std::string BaseMachine::Knob::get_value_name(int val) {
		if(k_type == Knob::rik_sigid) {
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

	void BaseMachine::Knob::set_value(int val) {
		data.i.value = val;
		auto kid = knb_id;
		send_obj_message(
			[kid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Knob::set_value(float val) {
		data.f.value = val;
		auto kid = knb_id;
		send_obj_message(
			[kid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Knob::set_value(double val) {
		data.d.value = val;
		auto kid = knb_id;
		send_obj_message(
			[kid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", std::to_string(val));
			}
			);
	}

	void BaseMachine::Knob::set_value(bool val) {
		bl_data = val;
		auto kid = knb_id;
		send_obj_message(
			[kid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", val ? "true" : "false");
			}
			);
	}

	void BaseMachine::Knob::set_value(const std::string &val) {
		str_data = val;
		auto kid = knb_id;
		send_obj_message(
			[kid, val](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", val);
			}
			);
	}

	bool BaseMachine::Knob::has_midi_controller(int &__coarse_controller, int &__fine_controller) {
		if(coarse_controller == -1 && fine_controller == -1) return false;

		__coarse_controller = coarse_controller;
		__fine_controller = fine_controller;

		return true;
	}

	)

SERVER_N_CLIENT_CODE(
	void BaseMachine::Knob::update_value_from_message_value(const std::string& value) {
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			data.i.value = std::stoi(value);
			break;
		case Knob::rik_bool:
			bl_data = value == "true";
			break;
		case Knob::rik_float:
			data.f.value = std::stof(value);
			break;
		case Knob::rik_double:
			data.d.value = std::stod(value);
			break;
		case Knob::rik_string:
			str_data = value;
			break;
		}
	}

	std::shared_ptr<BaseMachine::Knob> BaseMachine::Knob::get_knob(std::set<std::shared_ptr<Knob> > knobs, int knob_id) {
		for(auto knob : knobs) {
			if(knob->knb_id == knob_id)
				return knob;
		}
		return nullptr;
	}

	template <class SerderClassT>
	void BaseMachine::Knob::serderize(SerderClassT& iserder) {
		iserder.process(name);
		iserder.process(group_name);
		iserder.process(title);

		iserder.process(k_type);

		switch(k_type) {
		case Knob::rik_float: {
			iserder.process(data.f.min);
			iserder.process(data.f.max);
			iserder.process(data.f.step);
			iserder.process(data.f.value);
		} break;

		case Knob::rik_double: {
			iserder.process(data.d.min);
			iserder.process(data.d.max);
			iserder.process(data.d.step);
			iserder.process(data.d.value);
		} break;

		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid: {
			iserder.process(data.i.min);
			iserder.process(data.i.max);
			iserder.process(data.i.step);
			iserder.process(data.i.value);

			if(k_type == Knob::rik_enum) {
				iserder.process(enum_names);
			}
		} break;

		case Knob::rik_bool: {
			iserder.process(bl_data);
		} break;

		case Knob::rik_string: {
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
		iserder.process(knobs);
		SATAN_DEBUG("serderize_base_machine() -- base_machine_name: %s\n",
			    name.c_str());
	}
	);

CLIENT_CODE(
	void BaseMachine::handle_cmd_change_knob_value(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		int kid = std::stoi(msg.get_value("knb_id"));
		std::string value = msg.get_value("value");
		auto knob = Knob::get_knob(knobs, kid);
		if(knob) {
			knob->update_value_from_message_value(value);
		}
	}

	void BaseMachine::handle_cmd_attach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {

	}

	void BaseMachine::handle_cmd_detach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {
	}

	BaseMachine::BaseMachine(const Factory *factory, const RemoteInterface::Message &serialized)
	: SimpleBaseObject(factory, serialized)
	{
		register_handlers();

		SATAN_DEBUG("Trying to create client side BaseMachine...\n");

		Serialize::ItemDeserializer serder(serialized.get_value("basemachine_data"));
		serderize_base_machine(serder);

		SATAN_DEBUG("BaseMachine(%s) created client side:\n", name.c_str());
		for(auto group : groups) {
			SATAN_DEBUG("  : %s\n", group.c_str());
		}

		SATAN_DEBUG("BaseMachine(%s) knobs:\n", name.c_str());
		for(auto knb : knobs) {
			auto n = knb->get_name().c_str();
			auto g = knb->get_group().c_str();
			SATAN_DEBUG("  : %s (%s)\n", n, g);
			knb->set_msg_builder(
				[this](std::function<void(std::shared_ptr<Message> &)> fill_in_msg) {
					send_message_to_server(req_change_knob_value, fill_in_msg);
				}
				);
		}
	}

	);

SERVER_CODE(
	std::map<std::shared_ptr<Machine>, std::shared_ptr<BaseMachine> > BaseMachine::machine2basemachine;

	void BaseMachine::handle_req_change_knob_value(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		int kid = std::stoi(msg.get_value("knb_id"));
		std::string value = msg.get_value("value");
		auto knob = Knob::get_knob(knobs, kid);
		if(knob) {
			knob->update_value_from_message_value(value);
		}
		send_message(
			cmd_change_knob_value,
			[kid, value](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("knb_id", std::to_string(kid));
				msg_to_send->set_value("value", value);
			}
			);
	}

	void BaseMachine::handle_req_attach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {
	}

	void BaseMachine::handle_req_detach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {
	}

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

	void BaseMachine::init_from_machine_ptr(std::shared_ptr<BaseMachine> bmchn, std::shared_ptr<Machine> m_ptr) {
		machine2basemachine[m_ptr] = bmchn;

		name = m_ptr->get_name();
		groups = m_ptr->get_controller_groups();
		int knb_id = 0;
		for(auto ctrname : m_ptr->get_controller_names()) {
			SATAN_DEBUG(" --- adding ctr %s\n", ctrname.c_str());
			auto m_ctrl = m_ptr->get_controller(ctrname);
			SATAN_DEBUG(" .. m_ctrl is %p\n", m_ctrl);
			auto c = std::make_shared<Knob>(knb_id++, m_ctrl);
			knobs.insert(c);
		}

		for(auto input_name : m_ptr->get_input_names()) {
			inputs.emplace_back(Socket{.name = input_name});
		}
		for(auto output_name : m_ptr->get_output_names()) {
			outputs.emplace_back(Socket{.name = output_name});
		}
	}

	void BaseMachine::machine_unregistered(std::shared_ptr<Machine> m_ptr) {
		auto found = machine2basemachine.find(m_ptr);
		if(found != machine2basemachine.end()) {
			machine2basemachine.erase(found);
		}
	}

	void BaseMachine::machine_input_attached(std::shared_ptr<Machine> source,
						 std::shared_ptr<Machine> destination,
						 const std::string &output_name,
						 const std::string &input_name) {
		auto s = machine2basemachine.find(source);
		auto d = machine2basemachine.find(destination);
		if(s != machine2basemachine.end())
			SATAN_DEBUG("Found source basemachine %p to match %p\n", s->first.get(), source.get());
		if(d != machine2basemachine.end())
			SATAN_DEBUG("Found destination basemachine %p to match %p\n", d->first.get(), destination.get());
	}

	void BaseMachine::machine_input_detached(std::shared_ptr<Machine> source,
						 std::shared_ptr<Machine> destination,
						 const std::string &output_name,
						 const std::string &input_name) {
	}

	);
