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

#ifndef ABSTRACT_KNOB_HH
#define ABSTRACT_KNOB_HH

#include "../remote_interface.hh"

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class AbstractKnob {
		public:
			ON_CLIENT(
				virtual void set_callback(std::function<void()>) {};
				virtual void set_value_as_double(double new_value_d) = 0;
				);
			virtual double get_value() = 0;
			virtual double get_min() = 0;
			virtual double get_max() = 0;
			virtual double get_step() = 0;
			virtual std::string get_title() = 0;
			virtual std::string get_value_as_text() = 0;
		};
	};
};

#endif
