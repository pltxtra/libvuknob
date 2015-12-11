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

#ifndef MIDI_EVENT_BUILDER_HH
#define MIDI_EVENT_BUILDER_HH

#include "dynlib/dynlib.h"

typedef struct _MidiEventChain {
	uint8_t data[sizeof(size_t) + sizeof(uint8_t) * 4];
	char separator_a[9];
	struct _MidiEventChain *next_in_chain;
	struct _MidiEventChain *last_in_chain;
	char separator_b[9];
} MidiEventChain;

class MidiEventBuilder {
private:
	class MidiEventChainControler {
	private:
		static MidiEventChain *separation_zone;
		static MidiEventChain *next_free_midi_event;

		static void init_free_midi_event_chain();
	public:
		static void check_separation_zone();

		/// please note - the MidiEvent pointers here are really a hack
		/// and really point to MidiEventChain objects.
		///
		/// Do not atempt to push a MidiEvent that was NOT retrieved using pop
		/// in this class. It will epicly FAIL.
		static MidiEvent *pop_next_free_midi(size_t size);
		static void push_next_free_midi(MidiEvent *_element);

		/// external stacks of events allocated from the main stack
		/// same warning here - do not play around with MidiEvent pointers
		/// that you got from external allocators - use only ones received from pop_next_free_midi()
		static MidiEvent *get_chain_head(MidiEventChain **queue);
		static void chain_to_tail(MidiEventChain **chain, MidiEvent *event);
		static void join_chains(MidiEventChain **destination, MidiEventChain **source);
	};

	void **buffer;
	int buffer_size;
	int buffer_position, buffer_p_last_skip;

	// chain of remaining midi events that need to be
	// transmitted ASAP
	MidiEventChain *remaining_midi_chain;

	// chain of freeable midi events that can be returned
	// to the main stack
	MidiEventChain *freeable_midi_chain;

	void chain_event(MidiEvent *mev);
	void process_freeable_chain();
	void process_remaining_chain();


public:
	MidiEventBuilder();

	void use_buffer(void **buffer, int buffer_size);
	void finish_current_buffer();
	bool skip(int skip_length); // return true as long as buffer is not full.

	void queue_midi_data(size_t len, const char *data);

	virtual void queue_note_on(int note, int velocity, int channel = 0);
	virtual void queue_note_off(int note, int velocity, int channel = 0);
	virtual void queue_controller(int controller, int value, int channel = 0);
	virtual void queue_pitch_bend(int value_lsb, int value_msb, int channel = 0);
};

#endif
