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
 *  Class RemoteInterface::Connection
 *
 ***************************/
SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void Connection::serderize(SerderClassT& iserder) {
		iserder.process(source_name);
		iserder.process(output_name);
		iserder.process(input_name);
		iserder.process(destination_name);

		SATAN_DEBUG("Connection[%s:%s -> %s:%s]\n",
			    source_name.c_str(), output_name.c_str(),
			    destination_name.c_str(), input_name.c_str()
			);
	}

	);

/***************************
 *
 *  Class RemoteInterface::Socket
 *
 ***************************/

SERVER_N_CLIENT_CODE(

	template <class SerderClassT>
	void Socket::serderize(SerderClassT& iserder) {
		iserder.process(name);
		iserder.process(connections);
		SATAN_DEBUG("Socket[%s]\n", name.c_str());
	}

	);

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

	void BaseMachine::Knob::set_callback(std::function<void()> new_callback) {
		callback = new_callback;
	}

	void BaseMachine::Knob::set_value_as_double(double new_value) {
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			set_value((int)new_value);
			break;
		case Knob::rik_float:
			set_value((float)new_value);
			break;
		case Knob::rik_double:
			set_value(new_value);
			break;
		case Knob::rik_bool:
			set_value(new_value > 0.5 ? true : false);
			break;
		case Knob::rik_string:
			// no numerical representation
			break;
		}
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

	double BaseMachine::Knob::get_value() {
		double retval = 0.0;
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			retval = (double)data.i.value;
			break;
		case Knob::rik_bool:
			retval = (bl_data ? 1.0 : 0.0);
			break;
		case Knob::rik_float:
			retval = (double)data.f.value;
			break;
		case Knob::rik_double:
			retval = data.d.value;
			break;
		case Knob::rik_string:
			retval = 0.0;
			break;
		}
		return retval;
	}

	double BaseMachine::Knob::get_min() {
		double retval = 0.0;
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			retval = (double)data.i.min;
			break;
		case Knob::rik_bool:
			retval = 0.0;
			break;
		case Knob::rik_float:
			retval = (double)data.f.min;
			break;
		case Knob::rik_double:
			retval = data.d.min;
			break;
		case Knob::rik_string:
			retval = 0.0;
			break;
		}
		return retval;
	}

	double BaseMachine::Knob::get_max() {
		double retval = 0.0;
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			retval = (double)data.i.max;
			break;
		case Knob::rik_bool:
			retval = 1.0;
			break;
		case Knob::rik_float:
			retval = (double)data.f.max;
			break;
		case Knob::rik_double:
			retval = data.d.max;
			break;
		case Knob::rik_string:
			retval = 0.0;
			break;
		}
		return retval;
	}

	double BaseMachine::Knob::get_step() {
		double retval = 0.0;
		switch(k_type) {
		case Knob::rik_int:
		case Knob::rik_enum:
		case Knob::rik_sigid:
			retval = (double)data.i.step;
			break;
		case Knob::rik_bool:
			retval = 1.0;
			break;
		case Knob::rik_float:
			retval = (double)data.f.step;
			break;
		case Knob::rik_double:
			retval = data.d.step;
			break;
		case Knob::rik_string:
			retval = 0.0;
			break;
		}
		return retval;
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

	std::string BaseMachine::Knob::get_value_as_text() {
		std::stringstream ss;
		switch(k_type) {
		case Knob::rik_int:
			ss <<  data.i.value;
			break;
		case Knob::rik_enum:
		case Knob::rik_sigid:
			ss << get_value_name(data.i.value);
			break;
		case Knob::rik_bool:
			ss << (bl_data ? "true" : "false");
			break;
		case Knob::rik_float:
			ss << data.f.value;
			break;
		case Knob::rik_double:
			ss << data.d.value;
			break;
		case Knob::rik_string:
			ss << str_data;
			break;
		}
		return ss.str();
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
			return "illegal value";

		return (*enam).second;
	}

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
		ON_CLIENT(
			if(callback)
				callback();
			);
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
		iserder.process(knb_id);
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

	std::vector<std::string> BaseMachine::get_socket_names(SocketType type) {
		auto f = [this](std::vector<std::shared_ptr<Socket> > sockets, std::vector<std::string> &result) {
			for(auto socket : sockets) {
				result.push_back(socket->name);
			}
		};
		std::vector<std::string> result;
		switch(type) {
		case InputSocket:
			f(inputs, result);
			break;
		case OutputSocket:
			f(outputs, result);
			break;
		}
		return result;
	}

	std::vector<std::shared_ptr<Connection> > BaseMachine::get_connections_on_socket(SocketType type, const std::string& socket_name) {
		auto f = [this](std::vector<std::shared_ptr<Socket> > sockets,
				const std::string& socket_name) -> std::vector<std::shared_ptr<Connection> >
			{
			std::vector<std::shared_ptr<Connection> > result;
			for(auto socket : sockets) {
				if(socket->name == socket_name) {
					result = socket->connections;
				}
			}
			return result;
		};
		std::vector<std::shared_ptr<Connection> > result;
		switch(type) {
		case InputSocket:
			result = f(inputs, socket_name);
			break;
		case OutputSocket:
			result = f(outputs, socket_name);
			break;
		}
		return result;
	}

	std::vector<std::string> BaseMachine::get_knob_groups() {
		return groups;
	}

	std::vector<std::shared_ptr<BaseMachine::Knob> > BaseMachine::get_knobs_for_group(const std::string& group_name) {
		std::vector<std::shared_ptr<Knob> > retval;
		for(auto knob : knobs) {
			if(group_name == "")
				retval.push_back(knob);
			else if(knob->get_group() == group_name)
				retval.push_back(knob);
		}
		return retval;
	}

	std::shared_ptr<BaseMachine> BaseMachine::get_machine_by_name(const std::string& name) {
		if(name2machine.count(name))
			return name2machine[name];
		return nullptr;
	}

	std::map<std::string, std::shared_ptr<BaseMachine> > BaseMachine::name2machine;

	void BaseMachine::register_by_name(std::shared_ptr<BaseMachine> bmchn) {
		SATAN_DEBUG("[%s] register_by_name(%s)\n", CLIENTORSERVER_STRING, bmchn->name.c_str());
		name2machine[bmchn->name] = bmchn;
	}

	template <class SerderClassT>
	void BaseMachine::serderize_base_machine(SerderClassT& iserder) {
		SATAN_DEBUG("serderize_base_machine() -- .....\n");
		iserder.process(name);
		iserder.process(groups);
		iserder.process(knobs);
		iserder.process(inputs);
		iserder.process(outputs);
		SATAN_DEBUG("serderize_base_machine() -- base_machine_name: %s\n",
			    name.c_str());
	}

	void BaseMachine::add_connection(SocketType socket_type, const Connection& new_connection) {
		auto f = [this](const Connection& new_connection, std::vector<std::shared_ptr<Socket> > sockets, const std::string& socket_name) {
			for(auto socket : sockets) {
				if(socket->name == socket_name) {
					for(auto connection : socket->connections) {
						if(connection->equals(new_connection)) {
							return;
						}
					}
					socket->connections.push_back(std::make_shared<Connection>(new_connection));
				}
			}
		};
		switch(socket_type) {
		case InputSocket:
		{
			f(new_connection, inputs, new_connection.input_name);
		}
		break;
		case OutputSocket:
			f(new_connection, outputs, new_connection.output_name);
		{
		}
		break;
		}
	}

	void BaseMachine::remove_connection(SocketType socket_type, const Connection& connection2remove) {
		auto f = [this](const Connection& connection2remove, std::vector<std::shared_ptr<Socket> > sockets, const std::string& socket_name) {
			for(auto socket : sockets) {
				if(socket->name == socket_name) {
					for(auto cn = socket->connections.begin();
					    cn != socket->connections.end();
					    cn++) {
						if((*cn)->equals(connection2remove)) {
							socket->connections.erase(cn);
							return;
						}
					}
				}
			}
		};
		switch(socket_type) {
		case InputSocket:
		{
			f(connection2remove, inputs, connection2remove.input_name);
		}
		break;
		case OutputSocket:
			f(connection2remove, outputs, connection2remove.output_name);
		{
		}
		break;
		}
	}

	void BaseMachine::attach_input(std::shared_ptr<BaseMachine> source, std::string output, std::string input) {
		auto cnxn = Connection{.source_name = source->name, .output_name = output,
				       .destination_name = name, .input_name = input};
		source->add_connection(OutputSocket, cnxn);
		this->add_connection(InputSocket, cnxn);
		SATAN_DEBUG("::attach_input() done with [%s:%s] -> [%s:%s].\n",
			    cnxn.source_name.c_str(),
			    cnxn.output_name.c_str(),
			    cnxn.destination_name.c_str(),
			    cnxn.input_name.c_str()
			);
	}

	void BaseMachine::detach_input(std::shared_ptr<BaseMachine> source, std::string output, std::string input) {
		auto cnxn = Connection{.source_name = source->name, .output_name = output,
				       .destination_name = name, .input_name = input};
		source->remove_connection(OutputSocket, cnxn);
		this->remove_connection(InputSocket, cnxn);
		SATAN_DEBUG("::detach_input() done.\n");
	}

	);

CLIENT_CODE(
	void BaseMachine::handle_cmd_change_knob_value(RemoteInterface::Context *context,
						       RemoteInterface::MessageHandler *src,
						       const RemoteInterface::Message& msg) {
		int kid = std::stoi(msg.get_value("knb_id"));
		std::string value = msg.get_value("value");
		SATAN_DEBUG("handle_cmd_change_knob_valu(%d, %s)\n", kid, value.c_str());
		auto knob = Knob::get_knob(knobs, kid);
		if(knob) {
			knob->update_value_from_message_value(value);
		}
	}

	void BaseMachine::handle_attachment_command(AttachmentOperation atop, const RemoteInterface::Message& msg) {
		auto source_name = msg.get_value("src");
		auto output = msg.get_value("oup");
		auto destination_name = msg.get_value("dst");
		auto input = msg.get_value("inp");
		SATAN_ERROR("[%d] handle_cmd_attach([%s:%s] -> [%s:%s]\n",
			    name2machine.size(),
			    source_name.c_str(), output.c_str(), destination_name.c_str(), input.c_str());

		if(name2machine.count(source_name) && name2machine.count(destination_name)) {
			auto source = name2machine[source_name];
			auto destination = name2machine[destination_name];
			SATAN_ERROR("  -> found source (%p) and destination (%p)\n", source.get(), destination.get());
			switch(atop) {
			case AttachInput:
				destination->attach_input(source, output, input);
				break;
			case DetachInput:
				destination->detach_input(source, output, input);
				break;
			}
		}
	}

	void BaseMachine::handle_cmd_attach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {
		handle_attachment_command(AttachInput, msg);
	}

	void BaseMachine::handle_cmd_detach_input(RemoteInterface::Context *context,
						  RemoteInterface::MessageHandler *src,
						  const RemoteInterface::Message& msg) {
		handle_attachment_command(DetachInput, msg);
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
		SATAN_DEBUG("        -- handle_req_change_knob_value(%d, %s)\n", kid, value.c_str());
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
		name = m_ptr->get_name();
		machine2basemachine[m_ptr] = bmchn;
		register_by_name(bmchn);

		groups = m_ptr->get_controller_groups();
		int knb_id = 0;
		for(auto ctrname : m_ptr->get_controller_names()) {
			SATAN_DEBUG(" --- adding ctr %s\n", ctrname.c_str());
			auto m_ctrl = m_ptr->get_controller(ctrname);
			SATAN_DEBUG(" .. m_ctrl is %p, id %d\n", m_ctrl, knb_id);
			auto c = std::make_shared<Knob>(knb_id++, m_ctrl);
			knobs.insert(c);
		}

		for(auto input_name : m_ptr->get_input_names()) {
			inputs.push_back(std::make_shared<Socket>(Socket{.name = input_name}));
		}
		for(auto output_name : m_ptr->get_output_names()) {
			outputs.push_back(std::make_shared<Socket>(Socket{.name = output_name}));
		}
	}

	void BaseMachine::machine_unregistered(std::shared_ptr<Machine> m_ptr) {
		auto found = machine2basemachine.find(m_ptr);
		if(found != machine2basemachine.end()) {
			machine2basemachine.erase(found);
		}
	}

	void BaseMachine::send_attachment_command(
		const std::string& command,
		std::shared_ptr<Machine> source_machine,
		std::shared_ptr<Machine> destination_machine,
		const std::string &output_name,
		const std::string &input_name) {
		auto s = machine2basemachine.find(source_machine);
		auto d = machine2basemachine.find(destination_machine);
		if(s == machine2basemachine.end()) {
			SATAN_ERROR("machine_input_attached(): Did not find source basemachine to match %p (%s)\n",
				    source_machine.get(), source_machine->get_name().c_str());
			return;
		}
		if(d == machine2basemachine.end()) {
			SATAN_ERROR("machine_input_attached(): Did not find destination basemachine to match %p (%s)\n",
				    destination_machine.get(), destination_machine->get_name().c_str());
			return;
		}
		auto source = s->second;
		auto destination = d->second;
		auto source_name = source->name;
		auto destination_name = destination->name;
		auto f =
			[source_name, output_name, destination_name, input_name](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("src", source_name);
				msg_to_send->set_value("oup", output_name);
				msg_to_send->set_value("dst", destination_name);
				msg_to_send->set_value("inp", input_name);
		};

		destination->send_message(command, f);
	}

	void BaseMachine::machine_input_attached(std::shared_ptr<Machine> source_machine,
						 std::shared_ptr<Machine> destination_machine,
						 const std::string &output_name,
						 const std::string &input_name) {
		send_attachment_command(cmd_attach_input, source_machine, destination_machine, output_name, input_name);
	}

	void BaseMachine::machine_input_detached(std::shared_ptr<Machine> source_machine,
						 std::shared_ptr<Machine> destination_machine,
						 const std::string &output_name,
						 const std::string &input_name) {
		send_attachment_command(cmd_detach_input, source_machine, destination_machine, output_name, input_name);
	}

	);
