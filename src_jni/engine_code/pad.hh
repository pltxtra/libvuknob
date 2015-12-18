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

#ifndef CLASS_PAD
#define CLASS_PAD

#include <vector>
#include <iostream>
#include <fstream>
#include <kamo_xml.hh>

#include "../readerwriterqueue/readerwriterqueue.h"
#include "../midi_generation.hh"

#define MAX_ARP_FINGERS 5
#define MAX_PAD_FINGERS 5
#define MAX_PAD_CHORD 6
#define PAD_HZONES 8
#define PAD_VZONES 8
#define MAX_BUILTIN_ARP_PATTERNS 7
#define MAX_ARP_PATTERN_LENGTH 16

// PAD_TIME(line,tick)
#define PAD_TIME(A,B) ((int)(((A << BITS_PER_LINE) & (0xffffffff & (0xffffffff << BITS_PER_LINE))) | (B)))
#define PAD_TIME_LINE(A) (A >> BITS_PER_LINE)
#define PAD_TIME_TICK(B) (B & (((0xffffffff << BITS_PER_LINE) & 0xffffffff) ^ 0xffffffff))

class Pad {
public:
	class PadConfiguration {
	private:
		PadConfiguration();
	public:
		enum PadAxis {
			pad_x_axis = 0,
			pad_y_axis = 1,
			pad_z_axis = 2
		};
		enum ArpeggioDirection {
			arp_off = 0,
			arp_forward = 1,
			arp_reverse = 2,
			arp_pingpong = 3
		};

		enum ChordMode {
			chord_off = 0,
			chord_triad = 1,
			chord_quad = 2
		};

		PadConfiguration(ArpeggioDirection arp_dir, int scale, int octave);
		PadConfiguration(const PadConfiguration *config);

		ArpeggioDirection arp_direction;
		ChordMode chord_mode;
		int scale, last_scale, octave, arpeggio_pattern;
		int scale_data[21];

		// if coarse == -1 default to using pad to set velocity.. otherwise pad will set the assigned controller
		int pad_controller_coarse[2], pad_controller_fine[2];

		void set_coarse_controller(int id, int c);
		void set_fine_controller(int id, int c);
		void set_arpeggio_direction(ArpeggioDirection arp_direction);
		void set_arpeggio_pattern(int arp_pattern);
		void set_chord_mode(ChordMode mode);
		void set_scale(int scale);
		void set_octave(int octave);

		void get_configuration_xml(std::ostringstream &stream);
		void load_configuration_from_xml(const KXMLDoc &pad_xml);
	};

	class PadEvent {
	public:
		enum PadEvent_t {
			ms_pad_press, ms_pad_slide, ms_pad_release, ms_pad_no_event
		};

		PadEvent();
		PadEvent(int finger_id, PadEvent_t t, int x, int y, int z);

		PadEvent_t t;
		int finger, x, y, z;
	};

private:
	class Arpeggiator {
	public:
		class Note {
		public:
			int on_length, off_length;
			int octave_offset;
			bool slide;

			Note() : on_length(0), off_length(0), octave_offset(0), slide(false) {}
			Note(int _on_length, int _off_length, int _octave_offset, bool _slide) :
				on_length(_on_length), off_length(_off_length), octave_offset(_octave_offset),
				slide(_slide) {}
		};

		class Pattern {
		public:
			int length;
			Note note[MAX_ARP_PATTERN_LENGTH];

			Pattern() : length(0) {}
			void append(const Note &nn) {
				// disallow too long patterns
				if(length >= MAX_ARP_PATTERN_LENGTH) return;

				note[length++] = nn; // copy
			}

			static Pattern *built_in;

			static void initiate_built_in();
		};

	private:
		// pattern playback
		int phase; // 0 = go to on, 1 = on, 2 = go to off, 3 = off
		int ticks_left, note, velocity;
		int current_finger;
		int pattern_index;
		bool pingpong_direction_forward;
		PadConfiguration::ArpeggioDirection arp_direction;
		Pattern *current_pattern;

		// Finger data
		class ArpFinger {
		public:
			int key, velocity, counter; // counter is a reference counter
		};

		int finger_count;
		ArpFinger finger[MAX_ARP_FINGERS];

		void swap_keys(ArpFinger &a, ArpFinger &b);
		void sort_keys();
	public:

		Arpeggiator();

		int enable_key(int key, int velocity); // returns the key if succesfull, -1 if not
		void disable_key(int key);

		void process_pattern(bool mute, MidiEventBuilder *_meb);

		void set_pattern(int id);
		void set_direction(PadConfiguration::ArpeggioDirection direction);

		void reset();
	};

	class PadMotion : public PadConfiguration {
	private:
		int index; // when replaying a motion, this is used to keep track of where we are..
		int crnt_tick; // number of ticks since we started this motion

		int last_chord[MAX_PAD_CHORD];
		int last_x;

		std::vector<int> x;
		std::vector<int> y;
		std::vector<int> z;
		std::vector<int> t; // relative number of ticks from the start_tick

		Arpeggiator arperator;

		int start_tick;

		bool terminated, to_be_deleted;

		// notes should be an array with the size of MAX_PAD_CHORD
		// unused entries will be marked with a -1
		void build_chord(ChordMode chord_mode, const int *scale_data, int scale_offset, int *notes, int pad_column);

	public:
		void get_padmotion_xml(int finger, std::ostringstream &stream);

		PadMotion(PadConfiguration *parent_config,
			  int sequence_position, int x, int y, int z);

		// used to parse PadMotion xml when using project level < 5
		PadMotion(PadConfiguration *parent_config,
			  int &start_offset_return,
			  const KXMLDoc &motion_xml);

		// used to parse PadMotion xml when using project level >= 5
		PadMotion(int project_interface_level, PadConfiguration *parent_config,
			  const KXMLDoc &motion_xml);

		void quantize();
		void add_position(int x, int y, int z);
		void terminate();
		static void can_be_deleted_now(PadMotion *can_be_deleted);
		static void delete_motion(PadMotion *to_be_deleted);

		// returns true if the motion could start at the current position
		bool start_motion(int session_position);

		// resets a currently playing motion
		void reset();

		// returns true if the motion finished playing
		bool process_motion(bool mute, MidiEventBuilder *_meb);

		PadMotion *prev, *next;

		static void record_motion(PadMotion **head, PadMotion *target);
	};

	class PadFinger {
	private:
		bool to_be_deleted;

	public:
		PadMotion *recorded;
		PadMotion *next_motion_to_play;

		std::vector<PadMotion *> playing_motions; // currently playing recorded motions
		PadMotion *current; // current user controlled motion

		PadFinger();
		~PadFinger();

		void start_from_the_top();

		void process_finger_events(PadConfiguration* pad_config,
					   const PadEvent &pe, int session_position);

		// Returns true if we've completed all the recorded motions for this finger
		bool process_finger_motions(bool do_record, bool mute,
					    int session_position,
					    MidiEventBuilder *_meb,
					    bool quantize);

		void reset();

		// Designate a finger object for deletion.
		void delete_pad_finger();

		// Terminate all incomplete motions during recording
		void terminate();
	};


	class PadSession {
	private:
		bool to_be_deleted, terminated;

	public:
		PadSession(int start_at_tick, bool terminated);
		PadSession(int start_at_tick, PadMotion *padmot, int finger_id);
		PadSession(int project_interface_level, PadConfiguration* config, const KXMLDoc &ps_xml);

		PadFinger finger[MAX_PAD_FINGERS];

		int start_at_tick;
		int playback_position;
		bool in_play;

		bool start_play(int crnt_tick);

		// if this object has been designated for deletion, this function will return true when all currently playing
		// motions has been completed. If this returns true, you can delete this object.
		bool process_session(
			bool do_record, bool mute,
			MidiEventBuilder *_meb,
			bool quantize);

		void reset();

		// Designate a session object for deletion.
		void delete_session();

		// Indicate that the recording ended, terminate all open motions in each finger
		void terminate();

		void get_session_xml(std::ostringstream &stream);
	};

	friend class PadMotion;

	moodycamel::ReaderWriterQueue<PadEvent> *padEventQueue;

	PadSession *current_session;
	std::vector<PadSession *>recorded_sessions;

	bool do_record;
	bool do_quantize;

	void process_events(int tick);
	void process_sessions(bool mute, int tick, MidiEventBuilder *_meb);

	void internal_set_record(bool do_record);
	void internal_clear_pad();

public: // not really public... (not lock protected..)
	// please - don't call this from anything else but inside MachineSequencer class function...

	PadConfiguration config;

	void process(bool mute, int tick, MidiEventBuilder *_meb);
	void get_pad_xml(std::ostringstream &stream);
	void load_pad_from_xml(int project_interface_level, const KXMLDoc &pad_xml);

	template<class ElementT, class ContainerT>
	void export_to_loop(int start_tick, int stop_tick,
			    PadMidiExportBuilder<ElementT, ContainerT>& pmxb);

	void reset();

public: // actually public (lock protected)
	void enqueue_event(int finger_id, PadEvent::PadEvent_t t,
			   int x, int y, int z); // x, y, z should be 14 bit values
	void set_record(bool do_record);
	void set_quantize(bool do_quantize);
	void clear_pad();

	Pad();
	~Pad();
};

template<class ElementT, class ContainerT>
void Pad::export_to_loop(int start_tick, int stop_tick,
					   PadMidiExportBuilder<ElementT, ContainerT>& pmxb) {
	if(stop_tick < start_tick) {
		// force stop_tick to the highest integer value (always limited to 32 bit)
		stop_tick = 0x7fffffff;
	}

	std::vector<PadSession *> remaining_sessions;
	std::vector<PadSession *> active_sessions;

	// reset all prior to exporting
	reset();

	// copy all the recorded sessions that we should
	// export to the remaining sessions vector
	for(auto session : recorded_sessions) {
		if(session != current_session &&
		   session->start_at_tick >= start_tick &&
		   session->start_at_tick < stop_tick) {
			remaining_sessions.push_back(session);
		}
	}

	while(pmxb.current_tick < stop_tick || active_sessions.size() > 0) {
		{
			// scan remaining sessions to see if anyone can start at this tick
			auto session = remaining_sessions.begin();
			while(session != remaining_sessions.end()) {
				if((*session)->start_play(pmxb.current_tick)) {
					active_sessions.push_back(*session);
					session = remaining_sessions.erase(session);
				} else {
					session++;
				}
			}
		}

		{
			// process the active_sessions for export
			auto session = active_sessions.begin();
			while(session != active_sessions.end()) {
				(void)(*session)->process_session(false, false, &pmxb, false);
				if((*session)->start_play(-1)) { // check if it's currently playing
					session++;
				} else { //  if not, erase it from the active vector
 					session = active_sessions.erase(session);
				}
			}
		}

		// step no next tick
		pmxb.step_tick();
	}

	// we also reset all after exporting
	reset();
}

#endif
