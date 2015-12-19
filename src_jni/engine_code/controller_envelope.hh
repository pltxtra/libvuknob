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

#ifndef CLASS_CONTROLLER_ENVELOPE
#define CLASS_CONTROLLER_ENVELOPE

#include <map>
#include <kamo_xml.hh>

#include "../midi_generation.hh"

class ControllerEnvelope {
private:
	// playback data
	int t; // last processed t value
	std::map<int,int>::iterator next_c; // next control point
	std::map<int,int>::iterator previous_c; // previous control point

	// control_point maps t to y, where
	// t = PAD_TIME(line,tick)
	// y = 14 bit value (coarse + fine, as defined by the MIDI standard)
	// coarse value = (y & 0x7f80) >> 7; fine value = (y & 0x007f)
	std::map<int, int> control_point;

	void refresh_playback_data();

public:
	// factual data
	bool enabled;
	int controller_coarse, controller_fine;

	void process_envelope(int tick, MidiEventBuilder *_meb);

	ControllerEnvelope();
	ControllerEnvelope(const ControllerEnvelope *orig);

	// replace the control points of this envelope with the ones from other
	void set_to(const ControllerEnvelope *other);

	// set a control point at PAD_TIME(line,tick) to y (either change an exisint value, or set a new control point)
	void set_control_point(int line, int tick, int y);

	// create a line between two control points, eliminating any control point between them
	void set_control_point_line(
		int first_line, int first_tick, int first_y,
		int second_line, int second_tick, int second_y
		);

	// delete a range of control points
	void delete_control_point_range(
		int first_line, int first_tick,
		int second_line, int second_tick
		);

	// delete an existing control point at PAD_TIME(line,tick), silently fail if no point at t exists.
	void delete_control_point(int line, int tick);

	// get a const reference to the content of this envelope
	const std::map<int, int> &get_control_points();

	void write_to_xml(const std::string &id, std::ostringstream &stream);
	std::string read_from_xml(const KXMLDoc &env_xml);
};

#endif
