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

#include "../common.hh"
#include "../linked_list.hh"
#include "../remote_interface.hh"
#include "../serialize.hh"

#ifdef __RI_SERVER_SIDE
#include "../machine.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class BaseMachine
			: public RemoteInterface::SimpleBaseObject
		{
		public:
			class Controller {
			public:
				class NoSuchEnumValue : public std::runtime_error {
				public:
					NoSuchEnumValue() : runtime_error("No such enum value in BaseMachine::Controller.") {}
					virtual ~NoSuchEnumValue() {}
				};

				enum Type {
					ric_int = 0,
					ric_float = 1,
					ric_bool = 2,
					ric_string = 3,
					ric_enum = 4, // integer values map to a name
					ric_sigid = 5, // integer values map to sample bank index
					ric_double = 6
				};

				ON_SERVER(
					Controller(int ctrl_id, Machine::Controller *ctrl);

					std::string get_serialized_controller();
					);
				ON_CLIENT(
					Controller(std::function<
						   void(std::function<void(std::shared_ptr<Message> &msg_to_send)> )
						   >  _send_obj_message,
						   const std::string &serialized);

					std::string get_name(); // name of the control
					std::string get_title(); // user displayable title

					Type get_type();

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

					std::string get_value_name(int val);

					void set_value(int val);
					void set_value(float val);
					void set_value(double val);
					void set_value(bool val);
					void set_value(const std::string &val);

					bool has_midi_controller(int &coarse_controller, int &fine_controller);
					);

			private:
				ON_CLIENT(
					std::function<
					void(
						std::function<void(std::shared_ptr<Message> &msg_to_send)>
						)
					>  send_obj_message;
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

				int ctrl_id = -1;
				int ct_type = ric_int; //because of serderize limits, using enum Type values, but need to be explicitly stored as an integer
				std::string name, title, str_data = "";
				bool bl_data = false;
				std::map<int, std::string> enum_names;
				int coarse_controller = -1, fine_controller = -1;

				template <class SerderClassT>
				void serderize_controller(SerderClassT &serder);
			};

		private:
			std::string name;

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			void register_handlers() {
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize_base_machine(SerderClassT &serder);

		public:
			BaseMachine(const Factory *factory, const RemoteInterface::Message &serialized);
			BaseMachine(int32_t new_obj_id, const Factory *factory);
			ON_SERVER(
				void serialize(std::shared_ptr<Message> &target) override;
				void init_from_machine_ptr(std::shared_ptr<Machine> m_ptr);
				);
		};
	};
};

#endif
