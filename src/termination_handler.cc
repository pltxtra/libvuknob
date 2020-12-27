/*
 * vu|KNOB
 * Copyright (C) 2019 by Anton Persson
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

/* This file relies on the fact that a dynamically loaded
 * library will allocate the force_run_at_init object
 * during initalization of the library. It won't work
 * for a statically linked librray, since that will
 * optimize and not allocate the force_run_at_init
 * object. (When linking statically, the linker will
 * detect that the object is not in use and remove it...
 */

#include <cxxabi.h>
#include <stdlib.h>
#include "satan_error.hh"

void satan_terminate_handler() {
	int status = 0;
	char * buff = __cxxabiv1::__cxa_demangle(
		__cxxabiv1::__cxa_current_exception_type()->name(),
		NULL, NULL, &status);
	SATAN_ERROR("Unhandled exception caught by vuknob_server, terminating: %s\n", buff);
	abort();
}

struct AutoSetTerminationHandler {
	AutoSetTerminationHandler() {
		SATAN_ERROR("vuknob_server configured custom C++ terminate handler.\n");
		std::set_terminate(satan_terminate_handler);
	}
};

AutoSetTerminationHandler force_run_at_init;
