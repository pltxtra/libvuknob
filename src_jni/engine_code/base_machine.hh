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

#ifndef BASE_MACHINE_HH
#define BASE_MACHINE_HH

#ifndef __RI__SERVER_SIDE
#ifndef __RI__CLIENT_SIDE
#error __RI__SERVER_SIDE or __RI__CLIENT_SIDE must be defined.
#endif
#endif

#include <list>
#include <tuple>

#include "abstract_knob.hh"

#include "../common.hh"
#include "../linked_list.hh"
#include "../remote_interface.hh"
#include "../serialize.hh"

#ifdef __RI__SERVER_SIDE
#include "../machine.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		struct BaseMachine;

		struct Connection {
			static constexpr const char* serialize_identifier = "BaseMachine::Connection";
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

			std::string source_name, output_name, destination_name, input_name;

			bool equals(const Connection& other) {
				return
					(source_name == other.source_name)
					&&
					(output_name == other.output_name)
					&&
					(destination_name == other.destination_name)
					&&
					(input_name == other.input_name);
			}

			inline bool operator<(const Connection& rhs) const {
				return std::tie(source_name, output_name, destination_name, input_name)
					<
					std::tie(rhs.source_name, rhs.output_name, rhs.destination_name, rhs.input_name);
			}
		};

		class Socket {
		public:
			static constexpr const char* serialize_identifier = "BaseMachine::Socket";
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

			std::string name;
		};

		class BaseMachine
			: public RemoteInterface::SimpleBaseObject
		{
		public:
			class Knob : public AbstractKnob {
			public:
				static constexpr const char* serialize_identifier = "BaseMachine::Knob";
				template <class SerderClassT>
				void serderize(SerderClassT &serder);

				class NoSuchEnumValue : public std::runtime_error {
				public:
					NoSuchEnumValue() : runtime_error("No such enum value in BaseMachine::Knob.") {}
					virtual ~NoSuchEnumValue() {}
				};

				enum Type {
					rik_int = 0,
					rik_float = 1,
					rik_bool = 2,
					rik_string = 3,
					rik_enum = 4, // integer values map to a name
					rik_sigid = 5, // integer values map to sample bank index
					rik_double = 6
				};

				void update_value_from_message_value(const std::string& value);
				static std::shared_ptr<Knob> get_knob(std::set<std::shared_ptr<Knob> > knobs, int knob_id);

				ON_SERVER(
					Knob(int _knb_id, Machine::Controller *ctrl);
					~Knob();
					);
				ON_CLIENT(
					Knob();
					void set_msg_builder(std::function<
							     void(std::function<void(std::shared_ptr<Message> &msg_to_send)> )
							     >  _send_obj_message
						);

					void set_callback(std::function<void()>);

					virtual void set_value_as_double(double new_value) override; // this converts numerical values to the correct type automatic
					void set_value(int val);
					void set_value(float val);
					void set_value(double val);
					void set_value(bool val);
					void set_value(const std::string &val);

					);

				bool has_midi_controller(int &coarse_controller, int &fine_controller);
				std::string get_group(); // the group to which the controller belongs
				std::string get_name(); // name of the control
				virtual std::string get_title() override; // user displayable title

				Type get_type();

				// get data automatically mapped to doubles (i.e bool->double, int->double, float->double)
				// string type knobs will return 0.0 for all..
				virtual double get_value() override;
				virtual double get_min() override;
				virtual double get_max() override;
				virtual double get_step() override;

				void get_min(float &val);
				void get_max(float &val);
				void get_step(float &val);
				void get_min(double &val);
				void get_max(double &val);
				void get_step(double &val);
				void get_min(int &val);
				void get_max(int &val);
				void get_step(int &val);

				void get_value(int &val);
				void get_value(float &val);
				void get_value(double &val);
				void get_value(bool &val);
				void get_value(std::string &val);

				virtual std::string get_value_as_text() override;

				std::string get_value_name(int val);

			private:
				ON_SERVER(
					Machine::Controller *m_ctrl;
					);
				ON_CLIENT(
					std::function<
					void(
						std::function<void(std::shared_ptr<Message> &msg_to_send)>
						)
					>  send_obj_message;

					std::function<void()> callback;
					);

				struct data_f {
					float min, max, step, value;
				};
				struct data_d {
					double min, max, step, value;
				};
				struct data_i {
					int min, max, step, value;
				};
				union {
					data_f f;
					data_d d;
					data_i i;
				} data;

				int knb_id;
				int k_type = rik_int; //because of serderize limits, using enum Type values, but need to be explicitly stored as an integer
				std::string name, title, str_data = "", group_name;
				bool bl_data = false;
				std::map<int, std::string> enum_names;
				int coarse_controller = -1, fine_controller = -1;
			};

			ON_CLIENT(
				class MachineStateListener {
				public:
					virtual void on_move() = 0;
					virtual void on_attach(std::shared_ptr<BaseMachine> src_machine,
							       const std::string src_output,
							       const std::string dst_input) = 0;
					virtual void on_detach(std::shared_ptr<BaseMachine> src_machine,
							       const std::string src_output,
							       const std::string dst_input) = 0;
				};
				// if the client side app wants to be notified when things in this machine changes
				void set_state_change_listener(std::weak_ptr<MachineStateListener> state_listener);
				);

			std::string get_name() { return name; }
			enum SocketType {InputSocket, OutputSocket};
			std::vector<std::string> get_socket_names(SocketType type);
			std::set<Connection> get_connections_on_socket(SocketType type, const std::string& socket_name);
			std::vector<std::string> get_knob_groups();
			std::vector<std::shared_ptr<Knob> > get_knobs_for_group(const std::string& group_name);
			std::vector<std::shared_ptr<Knob> > get_all_knobs();
			static std::shared_ptr<BaseMachine> get_machine_by_name(const std::string& name);
			double get_x_position();
			double get_y_position();
			void set_position(double xpos, double ypos);
			void attach_machine_input(std::shared_ptr<BaseMachine> source, const std::string& output_name, const std::string& input_name);
			void detach_machine_input(std::shared_ptr<BaseMachine> source, const std::string& output_name, const std::string& input_name);
			void delete_machine();
			ON_SERVER(
				void connect_tightly(std::shared_ptr<BaseMachine> sibling);
				);

		private:
			enum AttachmentOperation {AttachInput, DetachInput};
			ON_SERVER(
				std::weak_ptr<Machine> machine_ptr;
				static std::map<std::shared_ptr<Machine>, std::shared_ptr<BaseMachine> > machine2basemachine;
				);
			ON_CLIENT(
				void handle_attachment_command(AttachmentOperation atop, const RemoteInterface::Message& msg);
				// used on client side when somethings changed server side
				std::set<std::weak_ptr<MachineStateListener>,
				std::owner_less<std::weak_ptr<MachineStateListener> > >state_listeners;
				void call_state_listeners(std::function<void(std::shared_ptr<MachineStateListener> listener)> callback);
				);
			static std::mutex base_machine_mutex;
			static std::map<std::string, std::shared_ptr<BaseMachine> > name2machine;

			std::string name;
			std::vector<std::string> groups;
			std::vector<std::shared_ptr<Socket> > inputs, outputs;
			std::set<Connection> input_connections, output_connections;
			std::set<std::shared_ptr<Knob> > knobs;
			double x_position, y_position;

			void add_connection(SocketType socket_type, const Connection& connection);
			void remove_connection(SocketType socket_type, const Connection& connection);

			void create_input_attachment(std::shared_ptr<BaseMachine> source, const std::string& output, const std::string& input);
			void delete_input_attachment(std::shared_ptr<BaseMachine> source, const std::string& output, const std::string& input);

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_delete_machine, "req_delete_machine");
			SERVER_SIDE_HANDLER(req_change_knob_value, "req_change_knob_value");
			SERVER_SIDE_HANDLER(req_attach_input, "req_attach_input");
			SERVER_SIDE_HANDLER(req_detach_input, "req_detach_input");
			SERVER_SIDE_HANDLER(req_set_position, "req_set_position");

			CLIENT_SIDE_HANDLER(cmd_change_knob_value, "cmd_change_knob_value");
			CLIENT_SIDE_HANDLER(cmd_attach_input, "cmd_attach_input");
			CLIENT_SIDE_HANDLER(cmd_detach_input, "cmd_detach_input");
			CLIENT_SIDE_HANDLER(cmd_set_position, "req_set_position");

			void register_handlers() {
				SERVER_REG_HANDLER(BaseMachine,req_delete_machine);
				SERVER_REG_HANDLER(BaseMachine,req_change_knob_value);
				SERVER_REG_HANDLER(BaseMachine,req_attach_input);
				SERVER_REG_HANDLER(BaseMachine,req_detach_input);

				CLIENT_REG_HANDLER(BaseMachine,cmd_change_knob_value);
				CLIENT_REG_HANDLER(BaseMachine,cmd_attach_input);
				CLIENT_REG_HANDLER(BaseMachine,cmd_detach_input);
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize_base_machine(SerderClassT &serder);

			static void send_attachment_command(
				const std::string& command,
				std::shared_ptr<Machine> source_machine,
				std::shared_ptr<Machine> destination_machine,
				const std::string &output_name,
				const std::string &input_name);

		protected:
			static void register_by_name(std::shared_ptr<BaseMachine> bmchn);

		public:
			ON_CLIENT(
				BaseMachine(const Factory *factory, const RemoteInterface::Message &serialized);
				);
			ON_SERVER(
				BaseMachine(int32_t new_obj_id, const Factory *factory);
				void serialize(std::shared_ptr<Message> &target) override;
				void init_from_machine_ptr(std::shared_ptr<BaseMachine> bmchn, std::shared_ptr<Machine> m_ptr);

				static void machine_unregistered(std::shared_ptr<Machine> m_ptr);
				static void machine_input_attached(std::shared_ptr<Machine> source,
								   std::shared_ptr<Machine> destination,
								   const std::string &output_name,
								   const std::string &input_name);
				static void machine_input_detached(std::shared_ptr<Machine> source,
								   std::shared_ptr<Machine> destination,
								   const std::string &output_name,
								   const std::string &input_name);
				);
		};
	};
};

#endif
