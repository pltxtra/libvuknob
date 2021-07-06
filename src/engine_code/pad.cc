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

#include "pad.hh"
#include "../machine.hh"

#include "scales_control.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/*************************************
 *
 * Class PadEvent
 *
 *************************************/

Pad::PadEvent::PadEvent() : x(0), y(0), z(0) {}
Pad::PadEvent::PadEvent(int finger_id, PadEvent_t _t, int _x, int _y, int _z)
	: t(_t)
	, finger(finger_id)
	, x(_x)
	, y(_y)
	, z(_z)
{}

/*************************************
 *
 * Class Pad::Arpeggiator::Pattern
 *
 *************************************/

Pad::Arpeggiator::Pattern *Pad::Arpeggiator::Pattern::built_in = NULL;

void Pad::Arpeggiator::Pattern::initiate_built_in() {
	if(built_in != NULL) return;

	built_in = new Pattern [MAX_BUILTIN_ARP_PATTERNS];

	/* create the built in patterns */
	int i = -1; // just for purity

	// new pattern 0
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));

	// new pattern 1
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note((MACHINE_TICKS_PER_LINE >> 1) - (MACHINE_TICKS_PER_LINE >> 2),
				(MACHINE_TICKS_PER_LINE >> 2),
				0, 0));

	// new pattern 2
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	SATAN_DEBUG("--new note\n");
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));
	SATAN_DEBUG("--new note\n");
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				1, 0));
	SATAN_DEBUG("--new note\n");

	// new pattern 3
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				1, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				-1, 0));

	// new pattern 4
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1) + 1,
				(MACHINE_TICKS_PER_LINE >> 1) - 1,
				0, 0));
	// new pattern 5
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1) + 1,
				(MACHINE_TICKS_PER_LINE >> 1) - 1,
				1, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1),
				(MACHINE_TICKS_PER_LINE >> 1),
				0, 0));
	built_in[i].append(Note(MACHINE_TICKS_PER_LINE - (MACHINE_TICKS_PER_LINE >> 1) + 1,
				(MACHINE_TICKS_PER_LINE >> 1) - 1,
				1, 0));
	// new pattern 6
	i++;
	SATAN_DEBUG("new pattern: %d\n", i);
	built_in[i].append(Note((MACHINE_TICKS_PER_LINE >> 2) - (MACHINE_TICKS_PER_LINE >> 3),
				(MACHINE_TICKS_PER_LINE >> 3),
				0, 0));
}

/*************************************
 *
 * Class Pad::Arpeggiator
 *
 *************************************/

Pad::Arpeggiator::Arpeggiator()
	: phase(0), ticks_left(0), note(0), velocity(0), current_finger(0)
	, pattern_index(0)
	, pingpong_direction_forward(true), arp_direction(PadConfiguration::arp_pingpong)
	, current_pattern(NULL), finger_count(0) {
	for(int k = 0; k < MAX_ARP_FINGERS; k++) {
		finger[k].counter = 0;
	}

	Pattern::initiate_built_in();

	current_pattern = &Pattern::built_in[0]; // default
}

void Pad::Arpeggiator::reset() {
	SATAN_DEBUG("Arpeggiator reset.\n");
	finger_count = phase = ticks_left = current_finger = pattern_index = 0;
	note = -1;
	for(int l = 0; l < MAX_ARP_FINGERS; l++) {
		finger[l].key = -1;
		finger[l].velocity = 0;
		finger[l].counter = 0;
	}
}

void Pad::Arpeggiator::swap_keys(ArpFinger &a, ArpFinger &b) {
	ArpFinger temp = b;
	b = a;
	a = temp;
}

void Pad::Arpeggiator::sort_keys() {
	int k, l;

	if(finger_count < 2) return;

	for(k = 0; k < finger_count -1; k++) {
		for(l = k + 1; l < finger_count; l++) {
			if(finger[l].key < finger[k].key) {
				swap_keys(finger[l], finger[k]);
			}
		}
	}
}

int Pad::Arpeggiator::enable_key(int key, int velocity) {
	for(int k = 0; k < finger_count; k++) {
		if(finger[k].key == key) {
			finger[k].counter++;
			finger[k].velocity = velocity;
			return key;
		}
	}
	if(finger_count < MAX_ARP_FINGERS) {
		finger[finger_count].key = key;
		finger[finger_count].velocity = velocity;
		finger[finger_count].counter = 1;
		finger_count++;

		sort_keys();

		return key;
	}

	return -1;
}

void Pad::Arpeggiator::disable_key(int key) {
	for(int k = 0; k < finger_count; k++) {
		if(finger[k].key == key) {
			finger[k].counter--;

			if(finger[k].counter == 0) {
				finger_count--;

				int l;
 				for(l = k; l < finger_count; l++) {
					finger[l] = finger[l + 1];
				}
 				for(; l < MAX_ARP_FINGERS; l++) {
					finger[l].key = -1;
				}
			}

			break;
		}
	}

	sort_keys();

	if(finger_count == 0) {
		current_finger = 0;
	}
}

void Pad::Arpeggiator::process_pattern(bool mute, MidiEventBuilder *_meb) {
	if(current_pattern == NULL
	   || current_pattern->length <= 0
	   || arp_direction == PadConfiguration::arp_off
		)
		return;

	if(mute || (finger_count == 0 && note != -1)) {
		_meb->queue_note_off(note, velocity);
		note = -1;
	}

	if(pattern_index >= current_pattern->length) {
		pattern_index = 0;
		phase = 0;
	}

	int old_note = note;

	switch(phase) {
	case 0:
	{
		if((finger_count > 0)) {
			switch(arp_direction) {
			case PadConfiguration::arp_off:
				break;
			default:
			case PadConfiguration::arp_forward:
				current_finger = (current_finger + 1) % finger_count;
				break;
			case PadConfiguration::arp_reverse:
				current_finger--;
				if(current_finger < 0)
					current_finger = finger_count > 0 ? finger_count - 1 : 0;
				break;
			case PadConfiguration::arp_pingpong:
				if(pingpong_direction_forward) {
					if(current_finger + 1 >= finger_count) {
						pingpong_direction_forward = false;
						current_finger = (current_finger - 1) % finger_count;
					} else {
						current_finger = (current_finger + 1) % finger_count;
					}
				} else {
					if(current_finger - 1 < 0) {
						pingpong_direction_forward = true;
						current_finger = (current_finger + 1) % finger_count;
					} else {
						current_finger = (current_finger - 1) % finger_count;
					}
				}
				break;
			}

			int finger_id = (current_finger) % finger_count;

			note = finger[finger_id].key + current_pattern->note[pattern_index].octave_offset * 12;
			velocity = finger[finger_id].velocity;

			// clamp note to [0 127]
			note &= 0x7f;

			if(!mute) _meb->queue_note_on(note, velocity);
		} else {
			note = -1;
		}
		if(old_note != -1) { // this is to enable "sliding"
			_meb->queue_note_off(old_note, velocity);
		}
		phase = 1;
		ticks_left = current_pattern->note[pattern_index].on_length - 1; // -1 to include phase 0 which takes one tick..
	}
	break;

	case 1:
	{
		ticks_left--;
		if(ticks_left <= 0)
			phase = 2;
	}
	break;

	case 2:
	{
		if(note != -1 && (!(current_pattern->note[pattern_index].slide))) {
			_meb->queue_note_off(note, velocity);
			note = -1;
		}
		phase = 3;
		ticks_left = current_pattern->note[pattern_index].off_length - 1; // -1 to include phase 2 which takes one tick..
	}
	break;

	case 3:
	{
		ticks_left--;
		if(ticks_left <= 0) {
			phase = 0;
			pattern_index++;
		}
	}
	break;

	}
}

void Pad::Arpeggiator::set_pattern(int id) {
	if(id < 0) {
		current_pattern = NULL;
		return;
	}

	current_pattern = &Pattern::built_in[id]; // default
}

void Pad::Arpeggiator::set_direction(PadConfiguration::ArpeggioDirection dir) {
	arp_direction = dir;
}

/*************************************
 *
 * Class Pad::PadConfiguration
 *
 *************************************/

Pad::PadConfiguration::PadConfiguration() : chord_mode(chord_off) {}

Pad::PadConfiguration::PadConfiguration(ArpeggioDirection _arp_direction, int _scale, int _octave)
	: arp_direction(_arp_direction), chord_mode(chord_off)
	, scale(_scale), last_scale(-1)
	, octave(_octave)
	, arpeggio_pattern(0)
	, pad_controller_coarse {-1 , -1}
	, pad_controller_fine {-1, -1}
{}

Pad::PadConfiguration::PadConfiguration(const PadConfiguration *parent)
	: arp_direction(parent->arp_direction), chord_mode(parent->chord_mode)
	, scale(parent->scale), last_scale(-1)
	, octave(parent->octave)
	, arpeggio_pattern(parent->arpeggio_pattern)
	, pad_controller_coarse {parent->pad_controller_coarse[0], parent->pad_controller_coarse[1]}
	, pad_controller_fine {parent->pad_controller_fine[0], parent->pad_controller_fine[1] }
{}

void Pad::PadConfiguration::set_coarse_controller(int id, int c) {
	id = (id % 2);
	pad_controller_coarse[id] = c;
}

void Pad::PadConfiguration::set_fine_controller(int id, int c) {
	id = (id % 2);
	pad_controller_fine[id] = c;
}

void Pad::PadConfiguration::set_arpeggio_direction(ArpeggioDirection _arp_direction) {
	arp_direction = _arp_direction;
}

void Pad::PadConfiguration::set_arpeggio_pattern(int arp_pattern) {
	arpeggio_pattern = arp_pattern;
}

void Pad::PadConfiguration::set_chord_mode(ChordMode _mode) {
	chord_mode = _mode;
}

void Pad::PadConfiguration::set_scale(int _scale) {
	scale = _scale;
}

void Pad::PadConfiguration::set_octave(int _octave) {
	octave = _octave;
}

void Pad::PadConfiguration::get_configuration_xml(std::ostringstream &stream) {
	stream << "<c "
	       << "m=\"" << (int)arp_direction << "\" "
	       << "c=\"" << (int)chord_mode << "\" "
	       << "s=\"" << scale << "\" "
	       << "o=\"" << octave << "\" "
	       << "arp=\"" << arpeggio_pattern << "\" "
	       << "cc=\"" << pad_controller_coarse[0] << "\" "
	       << "cf=\"" << pad_controller_fine[0] << "\" "
	       << "czc=\"" << pad_controller_coarse[1] << "\" "
	       << "czf=\"" << pad_controller_fine[1] << "\" "
	       << "/>\n";
}

void Pad::PadConfiguration::load_configuration_from_xml(const KXMLDoc &pad_xml) {
	KXMLDoc c = pad_xml;
	try {
		c = pad_xml["c"];
	} catch(...) { /* ignore */ }

	int _arp_direction = 0;
	KXML_GET_NUMBER(c, "m", _arp_direction, _arp_direction);
	arp_direction = (ArpeggioDirection)_arp_direction;

	int c_mode = 0;
	KXML_GET_NUMBER(c, "c", c_mode, c_mode);
	chord_mode = (ChordMode)c_mode;

	KXML_GET_NUMBER(c, "s", scale, scale);
	KXML_GET_NUMBER(c, "o", octave, octave);
	KXML_GET_NUMBER(c, "arp", arpeggio_pattern, arpeggio_pattern);

	KXML_GET_NUMBER(c, "cc", pad_controller_coarse[0], -1);
	KXML_GET_NUMBER(c, "cf", pad_controller_fine[0], -1);

	KXML_GET_NUMBER(c, "czc", pad_controller_coarse[1], -1);
	KXML_GET_NUMBER(c, "czf", pad_controller_fine[1], -1);
}

/*************************************
 *
 * Class Pad::PadMotion
 *
 *************************************/

void Pad::PadMotion::get_padmotion_xml(int finger, std::ostringstream &stream) {
	stream << "<m f=\"" << finger << "\" start=\"" << start_tick << "\">\n";

	get_configuration_xml(stream);

	unsigned int k;
	for(k = 0; k < x.size(); k++) {
		stream << "<d x=\"" << x[k] << "\" y=\"" << y[k] << "\" z=\"" << z[k] << "\" t=\"" << t[k] << "\" />\n";
	}

	stream << "</m>\n";
}

Pad::PadMotion::PadMotion(PadConfiguration* parent_config,
			  int session_position, int _x, int _y, int _z) :
	PadConfiguration(parent_config), index(-1), crnt_tick(-1), start_tick(session_position),
	terminated(false), to_be_deleted(false), prev(NULL), next(NULL)
{
	for(int x = 0; x < MAX_PAD_CHORD; x++)
		last_chord[x] = -1;

	add_position(_x, _y, _z);

}

Pad::PadMotion::PadMotion(PadConfiguration* parent_config, int &start_offset_return, const KXMLDoc &pad_xml) :
	PadConfiguration(parent_config), index(-1), start_tick(0),
	terminated(true), to_be_deleted(false), prev(NULL), next(NULL)
{
	for(int x = 0; x < MAX_PAD_CHORD; x++)
		last_chord[x] = -1;

	load_configuration_from_xml(pad_xml);

	int mk = 0;
	try {
		mk = pad_xml["d"].get_count();
	} catch(...) { /* ignore */ }

	int first_tick = 0;

	// because of the difference between project_level 4 and 5
	// this code get's a bit ugly (it converts level 4 representation to level 5 and beyond
	for(int k = 0; k < mk; k++) {
		KXMLDoc dxml = pad_xml["d"][k];
		int _x, _y, _abs_time, _t;

		KXML_GET_NUMBER(dxml, "x", _x, -1);
		KXML_GET_NUMBER(dxml, "y", _y, -1);
		KXML_GET_NUMBER(dxml, "t", _abs_time, -1);

		// the x and y coordinate need to be converted to 14bit values
		_x = ((_x & 0xfe0) << 2) | (_x & 0x1f); // shift coarse data up 2 bits to make room for 7 bit fine data
		_y = ((_y & 0xfe0) << 2) | (_y & 0x1f); // shift coarse data up 2 bits to make room for 7 bit fine data

		int lin = PAD_TIME_LINE(_abs_time);
		int tick = PAD_TIME_TICK(_abs_time);

		if(k == 0) {
			start_offset_return = _abs_time;
			first_tick = (lin * MACHINE_TICKS_PER_LINE) + tick;
			_t = 0;
		} else {
			_t = (lin * MACHINE_TICKS_PER_LINE) + tick - first_tick;
		}

		// _t might be < 0 - a.k.a. wrap failure
		// this was the major problem with the old style, it did not wrap very good..
		// so here we just discard it
		if(_t >= 0) {
			x.push_back(_x);
			y.push_back(_y);
			z.push_back(0); // prior to level 8 there was no z coordinate, default to 0
			t.push_back(_t);
		}
	}
}

Pad::PadMotion::PadMotion(int project_interface_level, PadConfiguration* parent_config, const KXMLDoc &pad_xml)
	: PadConfiguration(parent_config)
	, index(-1), terminated(true), to_be_deleted(false), prev(NULL), next(NULL)
{
	for(int x = 0; x < MAX_PAD_CHORD; x++)
		last_chord[x] = -1;

	load_configuration_from_xml(pad_xml);

	KXML_GET_NUMBER(pad_xml, "start", start_tick, -1);

	SATAN_DEBUG("(XML) PadMotion::PadMotion(%d)\n", project_interface_level);

	int mk = 0;
	try {
		mk = pad_xml["d"].get_count();
	} catch(...) { /* ignore */ }

	for(int k = 0; k < mk; k++) {
		KXMLDoc dxml = pad_xml["d"][k];
		int _x, _y, _z, _t;

		KXML_GET_NUMBER(dxml, "x", _x, -1);
		KXML_GET_NUMBER(dxml, "y", _y, -1);

		if(project_interface_level < 8) {
			_z = 0; // no z available, default to 0

			// the x and y coordinate need to be converted to 14bit values
			_x = ((_x & 0xfe0) << 2) | (_x & 0x1f); // shift coarse data up 2 bits to make room for 7 bit fine data
			_y = ((_y & 0xfe0) << 2) | (_y & 0x1f); // shift coarse data up 2 bits to make room for 7 bit fine data
		} else {
			KXML_GET_NUMBER(dxml, "z", _z, 0);
			SATAN_DEBUG("(XML) Got PadMotion _z value:: %d\n", _z);
		}

		KXML_GET_NUMBER(dxml, "t", _t, -1);

		x.push_back(_x);
		y.push_back(_y);
		z.push_back(_z);
		t.push_back(_t);
	}
}

void Pad::PadMotion::quantize() {
#if 0
	if(prev) {
		int prev_dist = quantize_tick(start_tick - prev->start_tick);
		prev_dist = prev->start_tick + prev_dist;
		SATAN_DEBUG("quantizing PadMotion...(%d -> %d)\n", start_tick, prev_dist);
		start_tick = prev_dist;

	}
#else
	int qtick = quantize_tick(start_tick);
	SATAN_DEBUG("quantizing PadMotion...(%d -> %d)\n", start_tick, qtick);
	start_tick = qtick;
#endif
}

void Pad::PadMotion::add_position(int _x, int _y, int _z) {
	if(terminated) return;

	x.push_back(_x);
	y.push_back(_y);
	z.push_back(_z);
	t.push_back(crnt_tick + 1); // when add_position is called the crnt_tick has not been upreved yet by "process_motion", so we have to do + 1
}

void Pad::PadMotion::terminate() {
	terminated = true;

	// ensure that we stop at least one tick after we start.
	int last = ((int)t.size()) - 1;
	if(last > 0 && t[0] == t[last]) {
		t[last] = t[last] + 1;
	}
}

void Pad::PadMotion::can_be_deleted_now(PadMotion *to_delete) {
	if(to_delete->to_be_deleted) {
		delete to_delete;
	}
}

void Pad::PadMotion::delete_motion(PadMotion *to_delete) {
	if(to_delete->index != -1) {
		// it's being played, mark for deletion later
		to_delete->to_be_deleted = true;
		return;
	}

	// it's not in use, delete straight away
	delete to_delete;
}

bool Pad::PadMotion::start_motion(int session_position) {
	// first check if the motion is not running (i.e. index == -1)
	if(index == -1) {
		if(session_position == start_tick) {
			index = 0;
			crnt_tick = -1; // we start at a negative index, it will be stepped up in process motion
			last_x = -1;
			arperator.reset();
			arperator.set_direction(arp_direction);
			arperator.set_pattern(arpeggio_pattern);
			return true;
		}
	}

	return false;
}

void Pad::PadMotion::reset() {
	index = -1;
	crnt_tick = -1;
	last_x = -1;
}

void Pad::PadMotion::build_chord(ChordMode chord_mode,
					      const int *scale_data, int scale_offset,
					      int *chord, int pad_column) {
	switch(chord_mode) {
	case chord_triad:
		chord[0] = octave * 12 + scale_data[pad_column + 0 + scale_offset];
		chord[1] = octave * 12 + scale_data[pad_column + 2 + scale_offset];
		chord[2] = octave * 12 + scale_data[pad_column + 4 + scale_offset];
		chord[3] = -1; // indicate end
		chord[4] = -1; // indicate end
		chord[5] = -1; // indicate end
		break;

	case chord_quad:
		chord[0] = octave * 12 + scale_data[pad_column + 0 + scale_offset];
		chord[1] = octave * 12 + scale_data[pad_column + 2 + scale_offset];
		chord[2] = octave * 12 + scale_data[pad_column + 4 + scale_offset];
		chord[3] = octave * 12 + scale_data[pad_column + 0 + scale_offset] + 12;
		chord[4] = -1; // indicate end
		chord[5] = -1; // indicate end
		break;

	case chord_off:
	default:
		break;
	}
}

bool Pad::PadMotion::process_motion(bool mute, MidiEventBuilder *_meb) {
	// check if we have reached the end of the line
	if(terminated && index >= (int)x.size()) {
		index = -1; // reset index
		arperator.process_pattern(mute, _meb);
		return true;
	}

	crnt_tick++;

	int scale_offset = 0;
	if(scale != last_scale) {
		last_scale = scale;

		if(auto scalo = RemoteInterface::ServerSpace::ScalesControl::get_scales_object_serverside()) {
			auto current_scale_keys = scalo->get_scale_keys(scale);

			for(auto k = 0; k < 7; k++) {
				scale_data[     k] = current_scale_keys[k];
				scale_data[ 7 + k] = current_scale_keys[k];
				scale_data[14 + k] = current_scale_keys[k];
			}
		} else {
			// default to standard C scale
			static const int def_scale_data[] = {
				0,  2,   4,  5,  7,  9, 11,
				12, 14, 16, 17, 19, 21, 23,
				24, 26, 28, 29, 31, 33, 35
			};
			for(auto k = 0; k < 21; k++)
				scale_data[k] = def_scale_data[k];
		}
	}

	int max = (int)x.size();

	while( (index < max) && (t[index] <= crnt_tick) ) {
		int pad_column = (x[index] >> 7) >> 4; // first right shift 7 for "coarse" data, then shift by 4 to get wich of the 8 columns we are in..
		int note = octave * 12 + scale_data[pad_column + scale_offset];
		int chord[MAX_PAD_CHORD];

		if( (!to_be_deleted) && (chord_mode != chord_off) && ((!terminated) || (index < (max - 1))) ) {
			build_chord(chord_mode, scale_data, scale_offset, chord, pad_column);
		} else {
			for(int k = 0; k < MAX_PAD_CHORD; k++) {
				chord[k] = -1;
			}
		}

		int c_c[] = {(y[index] >> 7), (z[index] >> 7)}; // coarse value
		int c_f[] = {(y[index] & 0x7f), (z[index] & 0x7f)}; // fine value

		// if y axis is set to control velocity, then pad_controller_coarse[0] will be set to -1
		int velocity = (pad_controller_coarse[0] == -1) ? c_c[0] : 0x7f;

		for(auto k = 0; k < 2; k++) {
			auto pcc = pad_controller_coarse[k];
			auto pcf = pad_controller_fine[k];

			if((pcc & (~0x7f)) == 0) { // if value is in the range [0 127]
				SATAN_DEBUG("pcc: %d -> c_c[%d] = %d\n",
					    pcc, k, c_c[k]);

				_meb->queue_controller(pcc, c_c[k]);
				SATAN_DEBUG("  --- pcf = %d\n", pcf);
				if(pcf != -1) {
					_meb->queue_controller(pcf, c_f[k]);
				}
			} else if(pcc >= Machine::Controller::sc_special_first) {
				switch(pcc) {
				case Machine::Controller::sc_pitch_bend:
					SATAN_DEBUG("pcc: %d -> pitch bend(%d): %d, %d\n",
						    pcc, k, c_f[k], c_c[k]);
					_meb->queue_pitch_bend(c_f[k], c_c[k], 0);
					break;
				}
			}
		}

		if(arp_direction == arp_off) {
			if(chord_mode != chord_off) {
				for(int k = 0; k < MAX_PAD_CHORD; k++) {
					if(last_chord[k] != chord[k]) {
						if(last_chord[k] != -1) {
							_meb->queue_note_off(last_chord[k], 0x7f);
						}
						if(chord[k] != -1 && !mute) {
							_meb->queue_note_on(chord[k], velocity);
						}
					}
				}
			} else {
				if((!to_be_deleted) && (last_x != note) ) {
					if((!mute) && ((!terminated) || (index < (max - 1))))
						_meb->queue_note_on(note, velocity);

					if(last_x != -1)
						_meb->queue_note_off(last_x, 0x7f);

				} else if( to_be_deleted || (terminated && (index == (max - 1)) )  ) {
					_meb->queue_note_off(last_x, 0x7f);
				}
			}
		} else {
			if(chord_mode != chord_off) {
				for(int k = 0; k < MAX_PAD_CHORD; k++) {
					if(last_chord[k] != -1) {
						arperator.disable_key(last_chord[k]);
					}
					if(chord[k] != -1) {
						chord[k] = arperator.enable_key(chord[k], velocity);
					}
				}
			} else {
				if(!to_be_deleted) {
					if(last_x != -1) {
						arperator.disable_key(last_x);
					}
					if((!terminated) || (index < (max - 1))) {
						note = arperator.enable_key(note, velocity);
					}
				} else if( to_be_deleted || (terminated && (index == (max - 1)) ) ) {
					arperator.disable_key(last_x);
				}
			}
		}

		for(int k = 0; k < MAX_PAD_CHORD; k++)
			last_chord[k] = chord[k];
		last_x = note;

		// step to next index
		index++;
	}

	arperator.process_pattern(mute, _meb);

	return false;
}

void Pad::PadMotion::record_motion(PadMotion **head, PadMotion *target) {
	// if the head is empty, then we have a simple task...
	if((*head) == NULL) {
		*head = target;
		target->next = NULL;
		target->prev = NULL;
		return;
	}

	// ouch - more difficult...
	PadMotion *top = *head;

	while(top != NULL) {

		// if we can, insert before top
		if(target->start_tick < top->start_tick) {
			target->next = top;
			target->prev = top->prev;

			if(target->prev == NULL) {
				// in this case we are to insert before the head ...
				*head = target;
			} else {
				// in this case we are to insert somewhere inside the chain...
				top->prev->next = target;
			}

			top->prev = target;

			return;
		}

		// if we reached the tail, attach to it...
		if(top->next == NULL) {
			top->next = target;
			target->prev = top;
			target->next = NULL;

			return;
		}

		// no matchin position found yet, check next..
		top = top->next;
	}

	// this should actually never be reached...
}

/*************************************
 *
 * Class Pad::PadFinger
 *
 *************************************/

#define PAD_RESOLUTION 4096

Pad::PadFinger::PadFinger() : to_be_deleted(false), recorded(NULL), next_motion_to_play(NULL), current(NULL) {
}

Pad::PadFinger::~PadFinger() {
	PadMotion *t = recorded;
	while(t != NULL) {
		recorded = t->next;
		delete t;
		t = recorded;
	}

	if(current) delete current;
}

void Pad::PadFinger::start_from_the_top() {
	next_motion_to_play = recorded;
	playing_motions.clear();
}

void Pad::PadFinger::process_finger_events(PadConfiguration* pad_config,
					   const PadEvent &pe, int session_position) {
	if(to_be_deleted) return;

	session_position++; // at this stage the session_position is at the last position still, we need to increase it by one.

	int x = pe.x, y = pe.y, z = pe.z;
	PadEvent::PadEvent_t t = pe.t;

	switch(t) {
	case PadEvent::ms_pad_press:
		current = new PadMotion(pad_config, session_position, x, y, z);
		if(current->start_motion(session_position)) {
			playing_motions.push_back(current);
		} else {
			SATAN_ERROR("Pad::PadFinger::process_finger_events() - Failed to start motion.\n");
			delete current; // highly unlikely - shouldn't happen, but if it does just silently fail...
			current = NULL;
		}

		break;

	case PadEvent::ms_pad_release:
		if(current) {
			current->add_position(x, y, z);
			current->terminate();
		}
		break;
	case PadEvent::ms_pad_slide:
		if(current) {
			current->add_position(x, y, z);
		}
		break;
	case PadEvent::ms_pad_no_event:
		break;
	}
}

bool Pad::PadFinger::process_finger_motions(bool do_record, bool mute, int session_position,
					    MidiEventBuilder *_meb,
					    bool quantize) {
	PadMotion *top = next_motion_to_play;

	// OK, check if we have anything at all now..
	if((!to_be_deleted) && (!mute) && top) {
		while(top && top->start_motion(session_position)) {
			playing_motions.push_back(top);
			top = top->next;
		}

		next_motion_to_play = top;
	} else if(mute) next_motion_to_play = NULL;

	std::vector<PadMotion *>::iterator k = playing_motions.begin();
	while(k != playing_motions.end()) {
		if((*k)->process_motion(mute, _meb)) {
			if((*k) == current) {
				if(do_record) {
					PadMotion::record_motion(&recorded, current);
					if(quantize)
						 current->quantize();
				} else {
					delete current;
				}
				current = NULL;
			} else {
				PadMotion::can_be_deleted_now((*k));
			}

			k = playing_motions.erase(k);
		} else {
			k++;
		}
	}

	if(playing_motions.size() == 0 && next_motion_to_play == NULL) {
		return true; // OK, we've completed all recorded motions here
	}
	return false; // We still got work to do
}

void Pad::PadFinger::reset() {
	for(auto motion : playing_motions) {
		motion->reset();
	}

	playing_motions.clear();
	next_motion_to_play = recorded;
}

void Pad::PadFinger::delete_pad_finger() {
	while(recorded) {
		PadMotion *to_delete = recorded;
		recorded = recorded->next;
		PadMotion::delete_motion(to_delete);
	}
	recorded = NULL;
	next_motion_to_play = NULL;

	to_be_deleted = true;
}

void Pad::PadFinger::terminate() {
	if(current) {
		current->terminate();
	}
}

/*************************************
 *
 * Class Pad::PadSession
 *
 *************************************/

Pad::PadSession::PadSession(int start, bool _terminated) :
	to_be_deleted(false), terminated(_terminated), start_at_tick(start),
	playback_position(-1), in_play(false)
{}

Pad::PadSession::PadSession(int start, PadMotion *padmot, int finger_id) :
	to_be_deleted(false), terminated(true), start_at_tick(start),
	playback_position(-1), in_play(false) {

	PadMotion::record_motion(&(finger[finger_id].recorded), padmot);
}

Pad::PadSession::PadSession(int project_interface_level, PadConfiguration* config, const KXMLDoc &ps_xml) :
	to_be_deleted(false), terminated(true),
	playback_position(-1), in_play(false)
{
	KXML_GET_NUMBER(ps_xml, "start", start_at_tick, 0);

	int mx = 0;
	mx = ps_xml["m"].get_count();

	SATAN_DEBUG("(XML) PadSession::PadSession() - m count: %d\n", mx);

	for(int k = 0; k < mx; k++) {
		KXMLDoc mxml = ps_xml["m"][k];

		int _f;
		KXML_GET_NUMBER(mxml, "f", _f, 0);

		PadMotion *m = new PadMotion(project_interface_level, config, mxml);

		SATAN_DEBUG("(XML) PadSessiion::PadSession() created new PadMotion %p - time to record: %p\n",
			    m, finger[_f].recorded);

		PadMotion::record_motion(&(finger[_f].recorded), m);
	}

}


bool Pad::PadSession::start_play(int crnt_tick) {
	if(in_play) return true;

	if(start_at_tick == crnt_tick) {
		SATAN_DEBUG(" starting session.\n");

		in_play = true;
		playback_position = -1;

		for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
			finger[_f].start_from_the_top();
		}

		return true;
	}

	return false;
}

bool Pad::PadSession::process_session(bool do_record, bool mute,
				      MidiEventBuilder*_meb,
				      bool quantize) {
	playback_position++;

	bool session_completed = true;
	for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
		bool finger_completed =
			finger[_f].process_finger_motions(do_record, mute, playback_position,
							  _meb,
							  quantize);
		session_completed = session_completed && finger_completed;

	}

	// if session is completed, we are no longer in play
	in_play = !(session_completed && terminated);

	if(!in_play)
		SATAN_DEBUG("    session_completed\n");

	return session_completed && to_be_deleted;
}

void Pad::PadSession::reset() {
	in_play = false;

	for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
		finger[_f].reset();
	}
}

void Pad::PadSession::delete_session() {
	to_be_deleted = true;

	for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
		finger[_f].delete_pad_finger();
	}
}

void Pad::PadSession::terminate() {
	terminated = true;
	for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
		finger[_f].terminate();
	}
}

void Pad::PadSession::get_session_xml(std::ostringstream &stream) {
	stream << "<s start=\"" << start_at_tick << "\">\n";
	for(int _f = 0; _f < MAX_PAD_FINGERS; _f++) {
		PadMotion *k = finger[_f].recorded;

		while(k != NULL) {
			k->get_padmotion_xml(_f, stream);
			k = k->next;
		}
	}
	stream << "</s>\n";
}

/*************************************
 *
 * Class Pad::Pad
 *
 *************************************/

Pad::Pad()
	: current_session(NULL)
	, do_record(false)
	, do_quantize(false)
	, config(PadConfiguration::arp_off, 0, 4)
{
	padEventQueue = new moodycamel::ReaderWriterQueue<PadEvent>(100);
}

Pad::~Pad() {
	delete padEventQueue;
}

void Pad::reset() {
	auto session_iterator = recorded_sessions.begin();
	while(session_iterator != recorded_sessions.end()) {
		if((*session_iterator) == current_session) {
			session_iterator = recorded_sessions.erase(session_iterator);
			delete current_session;
			current_session = NULL;
		} else {
			(*session_iterator)->reset();
			session_iterator++;
		}
	}
}

void Pad::process_events(int tick) {
	PadEvent pe;
	while(padEventQueue->try_dequeue(pe)) {
		printf("Pad::process_events() got event.\n");
		if(pe.finger >= 0 && pe.finger < MAX_PAD_FINGERS) {
			if(current_session == NULL) {
				int start_tick = tick;
				if(do_quantize) {
					int qtick = quantize_tick(start_tick);
					SATAN_DEBUG("  WILL QUANTIZE THIS SESSION. (%x -> %x)\n", start_tick, qtick);
					start_tick = qtick;
				}

				// if we are looping and we end up at or after __loop_stop, move us back
				if(Machine::get_loop_state()) {
					int loop_start = Machine::get_loop_start();
					int loop_end = Machine::get_loop_length() + loop_start;
					if(start_tick >= (loop_end << BITS_PER_LINE))
						start_tick -= (loop_start << BITS_PER_LINE);
				}

				current_session = new PadSession(start_tick, false);
				current_session->in_play = true;
				current_session->playback_position = -1;
				recorded_sessions.push_back(current_session);
			}

			current_session->finger[pe.finger].process_finger_events(
				&config,
				pe, current_session->playback_position);
		}
	}
}

void Pad::process_sessions(bool mute, int tick, MidiEventBuilder *_meb) {
	// Process recorded sessions
	std::vector<PadSession *>::iterator t;
	for(t = recorded_sessions.begin(); t != recorded_sessions.end(); ) {
		bool no_sound = (*t) == current_session ? false : mute;

		// check if in play, or if we should start it
		if((*t)->start_play(tick)) {
			if((*t)->process_session(do_record, no_sound,
						 _meb, do_quantize)) {
				SATAN_DEBUG(" --- session object will be deleted %p\n", (*t));
				delete (*t);
				t = recorded_sessions.erase(t);
			} else {
				t++;
			}
		} else {
			t++;
		}
	}
}

void Pad::process(bool mute, int tick, MidiEventBuilder *_meb) {
	process_events(tick);
	process_sessions(mute, tick, _meb);
}

void Pad::get_pad_xml(std::ostringstream &stream) {
	stream << "<pad>\n";

	config.get_configuration_xml(stream);

	for(auto k : recorded_sessions) {
		if(k != current_session) {
			k->get_session_xml(stream);
		}
	}

	stream << "</pad>\n";
}

void Pad::load_pad_from_xml(int project_interface_level, const KXMLDoc &p_xml) {
	//
	//  project_interface_level  < 5 == PadMotion xml uses old absolute coordinates
	//                          => 5 == PadMotion xml uses new relative coordinates
	//			    => 7 == PadEvent coordinates in 14 bit resolution, also added a z coordinate
	//
	try {
		KXMLDoc pad_xml = p_xml["pad"];

		config.load_configuration_from_xml(pad_xml);

		SATAN_DEBUG("Loading Pad xml data, project interface level: %d\n", project_interface_level);

		if(project_interface_level < 5) {
			int mx = 0;
			mx = pad_xml["m"].get_count();

			SATAN_DEBUG("Pad (old) - #m : %d\n", mx);

			for(int k = 0; k < mx; k++) {
				KXMLDoc mxml = pad_xml["m"][k];

				int _f;
				KXML_GET_NUMBER(mxml, "f", _f, 0);

				int start_offset;
				PadMotion *m = new PadMotion(&config, start_offset, mxml);

				PadSession *nses = new PadSession(start_offset, m, _f);
				recorded_sessions.push_back(nses);
			}
		} else {
			int mx = 0;
			mx = pad_xml["s"].get_count();

			SATAN_DEBUG("Pad (new) - #s : %d\n", mx);

			SATAN_DEBUG("   Pad (new) will debug XML:\n");
			pad_xml.debug();
			SATAN_DEBUG("   Pad (new) XML debugged.\n");

			for(int k = 0; k < mx; k++) {
				KXMLDoc ps_xml = pad_xml["s"][k];

				SATAN_DEBUG("   Pad (new) will debug SUB XML:\n");
				ps_xml.debug();
				SATAN_DEBUG("   Pad (new) SUB XML debugged.\n");

				try {
					PadSession *nses = new PadSession(project_interface_level,
									  &config, ps_xml);
					recorded_sessions.push_back(nses);
				} catch(...) {
					SATAN_DEBUG("Failed to create session - probably an empty session.\n");
				}
			}
		}
	} catch(jException &e) {
		SATAN_ERROR("Pad::load_pad_from_xml() caught a jException: %s\n", e.message.c_str());
	} catch(std::exception &e) {
		SATAN_ERROR("Pad::load_pad_from_xml() caught a std::exception: %s\n", e.what());
	} catch(...) {
		SATAN_ERROR("Pad::load_pad_from_xml() caught an unexpected exception.\n");
	}
}

void Pad::enqueue_event(int finger_id, PadEvent::PadEvent_t t, int x, int y, int z) {
	x &= 0x00003fff;
	y &= 0x00003fff;
	z &= 0x00003fff;
	padEventQueue->enqueue(PadEvent(finger_id, t, x, y, z));
}

void Pad::internal_set_record(bool _do_record) {
	if(do_record == _do_record) return; // no change...

	if(current_session) {
		if(!do_record) {
			SATAN_DEBUG(" session deleted.\n");
			current_session->delete_session(); // mark it for deletion
		} else {
			SATAN_DEBUG(" session terminated.\n");
			current_session->terminate(); // terminate recording
		}
		current_session = NULL;
		SATAN_DEBUG(" recorded session count: %zu\n", recorded_sessions.size());
	}
	do_record = _do_record;
}

void Pad::set_record(bool _do_record) {
	Machine::machine_operation_enqueue(
		[this, _do_record] () {
			internal_set_record(_do_record);
		}, true);
}

void Pad::set_quantize(bool _do_quantize) {
	Machine::machine_operation_enqueue(
		[this, _do_quantize] () {
			do_quantize = _do_quantize;
		}, true);
}

void Pad::internal_clear_pad() {
	// Mark all recorded sessions for deletion
	for(auto k : recorded_sessions) {
		k->delete_session();
	}

	// forget current session
	current_session = NULL;

}

void Pad::clear_pad() {
	Machine::machine_operation_enqueue(
		[this] () {
			internal_clear_pad();
		}, true);
}
