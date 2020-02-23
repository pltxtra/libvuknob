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

#include "machine_api.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#ifdef __RI_SERVER_SIDE
#include "server.hh"
#endif

SERVER_N_CLIENT_CODE(
	MachineAPI::MachineAPI(const Factory *factory, const RemoteInterface::Message &serialized)
	: BaseMachine(factory, serialized)
	{
		register_handlers();
		SATAN_DEBUG("MachineAPI() created client side.\n");
	}

	MachineAPI::MachineAPI(int32_t new_obj_id, const Factory *factory)
	: BaseMachine(new_obj_id, factory)
	{
		register_handlers();
		SATAN_DEBUG("MachineAPI() created server side.\n");
	}

	std::shared_ptr<BaseObject> MachineAPI::MachineAPIFactory::create(
		const Message &serialized,
		RegisterObjectFunction register_object
		) {
		auto nseq = std::make_shared<MachineAPI>(this, serialized);
		return register_object(nseq);
	}

	std::shared_ptr<BaseObject> MachineAPI::MachineAPIFactory::create(
		int32_t new_obj_id,
		RegisterObjectFunction register_object
		) {
		auto nseq = std::make_shared<MachineAPI>(new_obj_id, this);
		return register_object(nseq);
	}

	static MachineAPI::MachineAPIFactory this_will_register_us_as_a_factory;

	);
