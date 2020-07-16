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

#ifndef HANDLE_LIST_HH
#define HANDLE_LIST_HH

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

#ifdef __RI__SERVER_SIDE
#include "../dynamic_machine.hh"
#include "../machine.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class HandleList
			: public RemoteInterface::SimpleBaseObject
		{
		public:
			ON_CLIENT(
				static std::map<std::string, std::string> get_handles_and_hints();
				static void create_instance(const std::string &handle, double xpos, double ypos, bool autoconnect = false);
				);
		private:
			static constexpr const char* FACTORY_NAME		= "HandList";

			std::map<std::string, std::string> handle2hint;
			ON_CLIENT(
				static std::weak_ptr<HandleList> clientside_handle_list;
				);

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_create_machine_instance, "req_create_machine_instance");

			void register_handlers() {
				SERVER_REG_HANDLER(HandleList,req_create_machine_instance);
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

		public:
			ON_CLIENT(
				HandleList(const Factory *factory, const Message &serialized);
				);
			ON_SERVER(
				HandleList(int32_t new_obj_id, const Factory *factory);
				virtual void serialize(std::shared_ptr<Message> &target) override;
				);

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

			class HandleListFactory : public FactoryTemplate<HandleList> {
			public:
				ON_SERVER(HandleListFactory()
					  : FactoryTemplate<HandleList>(ServerSide, FACTORY_NAME, true)
					  {}
					);
				ON_CLIENT(HandleListFactory()
					  : FactoryTemplate<HandleList>(ClientSide, FACTORY_NAME)
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
