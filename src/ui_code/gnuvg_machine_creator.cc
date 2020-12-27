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

#include "gnuvg_machine_creator.hh"
#include "../engine_code/handle_list.hh"
#include "popup_menu.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

typedef RemoteInterface::ClientSpace::HandleList HandleList;

void GnuVGMachineCreator::create_machine(const std::string& hint, bool autoconnect) {
	SATAN_DEBUG("GnuVGMachineCreator::create_machine() - hint: %s\n", hint.c_str());
	std::vector<std::string> items;

	for(auto hnh : HandleList::get_handles_and_hints()) {
		SATAN_DEBUG("GnuVGMachineCreator::create_machine() - hint[%s] = %s\n", hnh.first.c_str(), hnh.second.c_str());
		if(hint == "" || hnh.second.find(hint) != std::string::npos) {
			SATAN_DEBUG("GnuVGMachineCreator::create_machine() - adding to items.\n");
			items.push_back(hnh.first);
		}
	}

	if(items.size() > 0) {
		SATAN_DEBUG("GnuVGMachineCreator::create_machine() - bringing up menu with %zu items\n", items.size());
		PopupMenu::show_menu(
			items,
			[items, autoconnect](int selection) {
				if(selection >= 0 && selection < (int)items.size()) {
					HandleList::create_instance(items[selection], 0.0, 0.0, autoconnect);
				}
			}
			);
	}
}
