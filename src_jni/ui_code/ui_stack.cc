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

#include "ui_stack.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

std::shared_ptr<AbstractHideable> UIStack::current;
std::stack<std::shared_ptr<AbstractHideable> > UIStack::previous;

void UIStack::print_string(const std::string& str) {
	SATAN_ERROR("%s", str.c_str());
}
