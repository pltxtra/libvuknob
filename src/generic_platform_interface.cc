/*
 * vu|KNOB (c) 2015 by Anton Persson
 *
 * -------------------------------------------------
 *
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
 * Copyright (C) 2008 by Anton Persson
 *
 * About SATAN:
 *   Originally developed as a small subproject in
 *   a course about applied signal processing.
 * Original Developers:
 *   Anton Persson
 *   Johan Thim
 *
 * http://www.733kru.org/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2; see COPYING for the complete License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <config.h>

#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <kamogui.hh>

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#include "generic_platform_interface.hh"

void GenericPlatformInterface::call_tar(std::vector<std::string > &args) {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

bool GenericPlatformInterface::share_musicfile(const std::string &path_to_file) {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
	return false;
}

void GenericPlatformInterface::announce_service(int port) {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

void GenericPlatformInterface::prepare_for_exit() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

void GenericPlatformInterface::discover_services() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

std::map<std::string, std::pair<std::string, int> > GenericPlatformInterface::list_services() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
	std::map<std::string, std::pair<std::string, int> > current_services;
	return current_services;
}

void GenericPlatformInterface::preview_16bit_wav_start(int channels, int samples, int frequency, int16_t *data) {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

bool GenericPlatformInterface::preview_16bit_wav_next_buffer() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
	return false;
}

void GenericPlatformInterface::preview_16bit_wav_stop() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}

bool GenericPlatformInterface::start_audio_recorder(const std::string &fname) {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
	return false;
}

void GenericPlatformInterface::stop_audio_recorder() {
	SATAN_ERROR("Function %s is not implemented.", __PRETTY_FUNCTION__);
}
