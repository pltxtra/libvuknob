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

#ifndef MACHINE_API_HH
#define MACHINE_API_HH

#ifndef __RI__SERVER_SIDE
#ifndef __RI__CLIENT_SIDE
#error __RI__SERVER_SIDE or __RI__CLIENT_SIDE must be defined.
#endif
#endif

#include "../common.hh"
#include "../linked_list.hh"
#include "../remote_interface.hh"
#include "../serialize.hh"

#include "base_machine.hh"

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class MachineAPI
			: public BaseMachine
		{
		public:
			static constexpr const char* FACTORY_NAME		= "MachineAPI";
		private:

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			void register_handlers() {
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize_machine_api(SerderClassT &serder);

		public:
			MachineAPI(const Factory *factory, const RemoteInterface::Message &serialized);
			MachineAPI(int32_t new_obj_id, const Factory *factory);

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

			class MachineAPIFactory
				: public FactoryTemplate<MachineAPI>
			{
			public:
				ON_SERVER(MachineAPIFactory()
					  : FactoryTemplate<MachineAPI>(ServerSide, FACTORY_NAME)
					  {}
					);
				ON_CLIENT(MachineAPIFactory()
					  : FactoryTemplate<MachineAPI>(ClientSide, FACTORY_NAME)
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
