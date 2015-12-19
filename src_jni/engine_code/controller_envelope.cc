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

#include "controller_envelope.hh"
#include "../machine.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

ControllerEnvelope::ControllerEnvelope() : enabled(true), controller_coarse(-1), controller_fine(-1) {
}

ControllerEnvelope::ControllerEnvelope(const ControllerEnvelope *original) : t(0), enabled(true), controller_coarse(-1), controller_fine(-1) {
	controller_coarse = controller_fine = -1;
	this->set_to(original);
}

void ControllerEnvelope::refresh_playback_data() {
	if(control_point.size() == 0) return;

	next_c = control_point.lower_bound(t);
	previous_c = next_c;

	if(next_c == control_point.end() || next_c == control_point.begin()) {
		return;
	}

	previous_c--;
}

// returns true if the controller finished playing
void ControllerEnvelope::process_envelope(int t, MidiEventBuilder *_meb) {
	if(!enabled || control_point.size() == 0) {
		return;
	} else if(next_c == control_point.end()) {
		next_c = control_point.lower_bound(t);
		previous_c = next_c;
		if(next_c == control_point.end()) {
			return;
		} else if(next_c != control_point.begin()) {
			previous_c--;
		}
	}

	if((*next_c).first > t && next_c == control_point.begin())
		return;

	int y;

	if((*next_c).first == t) {
		y = (*next_c).second;
		next_c++;
	} else {
		// interpolate
		y = (*previous_c).second + ((((*next_c).second - (*previous_c).second) * (t - (*previous_c).first)) / ((*next_c).first - (*previous_c).first));
	}
	if(controller_coarse >= 0 && controller_coarse < 128) {
		int y_c = (y >> 7) & 0x7f;
		SATAN_DEBUG("[%5x]y: %d (%x) -> coarse: %x\n", t, y, y, y_c);
		_meb->queue_controller(controller_coarse, y_c);
		if(controller_fine != -1) {
			int y_f = y & 0x7f;
			_meb->queue_controller(controller_fine, y_f);
		}
	} else if(controller_coarse >= Machine::Controller::sc_special_first) {
		switch(controller_coarse) {
		case Machine::Controller::sc_pitch_bend:
			_meb->queue_pitch_bend(0x7f & y, 0x7f & (y >> 7));
			break;
		}
	}
}

void ControllerEnvelope::set_to(const ControllerEnvelope *original) {
	control_point = original->control_point;
	enabled = original->enabled;

	refresh_playback_data();
}

void ControllerEnvelope::set_control_point(int line, int tick, int y) {
	control_point[PAD_TIME(line, tick)] = y;
	refresh_playback_data();
}

void ControllerEnvelope::set_control_point_line(
	int first_line, int first_tick, int first_y,
	int second_line, int second_tick, int second_y) {

	int first_x = PAD_TIME(first_line, first_tick);
	int second_x = PAD_TIME(second_line, second_tick);

	std::map<int, int>::iterator k1, k2;

	k1 = control_point.lower_bound(first_x);
	k2 = control_point.upper_bound(second_x);

	// this code erases all points between the first and the second
	if(k1 != control_point.end()) {
		control_point.erase(k1, k2);
	}

	// then we update the points at first and second
	control_point[first_x] = first_y;
	control_point[second_x] = second_y;

	refresh_playback_data();
}

void ControllerEnvelope::delete_control_point_range(
	int first_line, int first_tick,
	int second_line, int second_tick) {

	int first_x = PAD_TIME(first_line, first_tick);
	int second_x = PAD_TIME(second_line, second_tick);

	std::map<int, int>::iterator k1, k2;

	k1 = control_point.lower_bound(first_x);
	k2 = control_point.upper_bound(second_x);

	// this code erases all points between the first and the second
	if(k1 != control_point.end()) {
		control_point.erase(k1, k2);
	}

	refresh_playback_data();
}

void ControllerEnvelope::delete_control_point(int line, int tick) {
	std::map<int, int>::iterator k;

	k = control_point.find(PAD_TIME(line, tick));

	if(k != control_point.end()) {
		control_point.erase(k);
	}
	refresh_playback_data();
}

const std::map<int, int> & ControllerEnvelope::get_control_points() {
	return control_point;
}

void ControllerEnvelope::write_to_xml(const std::string &id, std::ostringstream &stream) {
	stream << "<envelope id=\"" << id << "\" enabled=\"" << (enabled ? "true" : "false") << "\" "
	       << "coarse=\"" << controller_coarse << "\" "
	       << "fine=\"" << controller_fine << "\" >\n";

	std::map<int, int>::iterator k;
	for(k  = control_point.begin();
	    k != control_point.end();
	    k++) {
		stream << "<p t=\"" << (*k).first << "\" y=\"" << (*k).second << "\" />\n";
	}

	stream << "</envelope>\n";
}

std::string ControllerEnvelope::read_from_xml(const KXMLDoc &env_xml) {
	std::string id = env_xml.get_attr("id");
	std::string enabled_str = env_xml.get_attr("enabled");
	KXML_GET_NUMBER(env_xml,"coarse",controller_coarse,-1);
	KXML_GET_NUMBER(env_xml,"fine",controller_fine,-1);

	int c, c_max = 0;
	try {
		c_max = env_xml["p"].get_count();
	} catch(...) {
		// ignore
	}

	for(c = 0; c < c_max; c++) {
		KXMLDoc p_xml = env_xml["p"][c];
		int _t, _y;

		KXML_GET_NUMBER(p_xml,"t",_t,-1);
		KXML_GET_NUMBER(p_xml,"y",_y,-1);
		if(!(_t == -1 || _y == -1)) {
			control_point[_t] = _y;
		}
	}

	enabled = (enabled_str == "true") ? true : false;

	refresh_playback_data();

	return id;
}
