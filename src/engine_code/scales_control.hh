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

#ifndef SCALES_CONTROL_OBJECT_HH
#define SCALES_CONTROL_OBJECT_HH

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

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class ScalesControl
			: public RemoteInterface::SimpleBaseObject
		{
		public:
			static constexpr const char* FACTORY_NAME		= "ScalesControl";

			ON_CLIENT(
				static const char* get_key_text(int key);

				int get_number_of_scales();

				std::vector<std::string> get_scale_names();
				std::vector<int> get_scale_keys(int index);

				std::vector<int> get_custom_scale_keys();
				void set_custom_scale_keys(std::vector<int>);
				);

			ScalesControl(const Factory *factory, const RemoteInterface::Message &serialized);
			ScalesControl(int32_t new_obj_id, const Factory *factory);

			ON_SERVER(
				virtual void serialize(std::shared_ptr<Message> &target) override;
				)

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

		private:
			struct Scale {
				static constexpr const char* serialize_identifier = "Scales::Scale";
				std::string name;
				int keys[7];

				template <class SerderClassT>
				void serderize(SerderClassT& iserder) {
					iserder.process(name);
					iserder.process(7, keys);
				}
			};

			Scale custom_scale;
			std::vector<Scale> scales;

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_set_custom_scale, "req_set_custom_scale");

			CLIENT_SIDE_HANDLER(cmd_set_custom_scale, "cmd_set_custom_scale");

			void register_handlers() {
				SERVER_REG_HANDLER(ScalesControl,req_set_custom_scale);

				CLIENT_REG_HANDLER(ScalesControl,cmd_set_custom_scale);
			}

			void initialize_scales();

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

		public:
			class ScalesControlFactory
				: public FactoryTemplate<ScalesControl>
			{
			public:
				ON_SERVER(ScalesControlFactory()
					  : FactoryTemplate<ScalesControl>(ServerSide, FACTORY_NAME, true)
					  {}
					);
				ON_CLIENT(ScalesControlFactory()
					  : FactoryTemplate<ScalesControl>(ClientSide, FACTORY_NAME)
					  {}
					);
				virtual std::shared_ptr<BaseObject> create(
					const Message &serialized,
					RegisterObjectFunction register_object
					) override;
				virtual std::shared_ptr<BaseObject> create(
					int32_t new_obj_id,
					RegisterObjectFunction register_object
					) override;

			};
		};
	};
};

#endif
