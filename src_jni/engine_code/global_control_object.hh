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

#ifndef GLOBAL_CONTROL_OBJECT_HH
#define GLOBAL_CONTROL_OBJECT_HH

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

		class GlobalControlObject
			: public RemoteInterface::SimpleBaseObject
		{
		public:
			static constexpr const char* FACTORY_NAME		= "GlobCon";

			ON_CLIENT(
				class GlobalControlListener {
				public:
					virtual void loop_state_changed(bool new_state) = 0;
				};

				void add_global_control_listener(std::shared_ptr<GlobalControlListener> glol);

				void set_loop_state(bool new_state);
				);

			GlobalControlObject(const Factory *factory, const RemoteInterface::Message &serialized);
			GlobalControlObject(int32_t new_obj_id, const Factory *factory);

			ON_SERVER(
				virtual void serialize(std::shared_ptr<Message> &target) override;
				)

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

		private:
			bool loop_state;

			ON_CLIENT(
				std::set<std::shared_ptr<GlobalControlListener> > gco_listeners;
				);

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_set_loop_state, "req_set_loop_state");

			CLIENT_SIDE_HANDLER(cmd_set_loop_state, "cmd_set_loop_state");

			void register_handlers() {
				SERVER_REG_HANDLER(GlobalControlObject,req_set_loop_state);

				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_loop_state);
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

		public:
			class GlobalControlObjectFactory
				: public FactoryTemplate<GlobalControlObject>
			{
			public:
				ON_SERVER(GlobalControlObjectFactory()
					  : FactoryTemplate<GlobalControlObject>(ServerSide, FACTORY_NAME, true)
					  {}
					);
				ON_CLIENT(GlobalControlObjectFactory()
					  : FactoryTemplate<GlobalControlObject>(ClientSide, FACTORY_NAME)
					  {}
					);
				virtual std::shared_ptr<BaseObject> create(const Message &serialized) override;
				virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) override;

			};
		};
	};
};

#endif
