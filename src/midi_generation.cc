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

#include <exception>
#include <jngldrum/jexception.hh>
#include "midi_generation.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/*************************************
 *
 * Global free chain of midi events
 *
 *************************************/

#define DEFAULT_MAX_FREE_CHAIN 256
MidiEventChain* MidiEventBuilder::MidiEventChainControler::next_free_midi_event = NULL;
MidiEventChain* MidiEventBuilder::MidiEventChainControler::separation_zone = NULL;

#define container_of(ptr, type, member) ({				\
			const typeof( ((type *)0)->member ) *__mptr = (ptr); \
			(type *)( (char *)__mptr - offsetof(type,member) );})

void MidiEventBuilder::MidiEventChainControler::init_free_midi_event_chain() {
	MidiEventChain *data, *next = NULL;

	data = (MidiEventChain *)malloc(sizeof(MidiEventChain) * (DEFAULT_MAX_FREE_CHAIN + 1));

	if(data == NULL) {
		throw std::bad_alloc();
	}

	memset(data, 0, sizeof(MidiEventChain) * DEFAULT_MAX_FREE_CHAIN);
	separation_zone = &data[DEFAULT_MAX_FREE_CHAIN];

	int k;
	for(k = DEFAULT_MAX_FREE_CHAIN - 1; k >= 0; k--) {
		next_free_midi_event = &data[k];
		next_free_midi_event->next_in_chain = next;

		next = next_free_midi_event;
	}
}

void MidiEventBuilder::MidiEventChainControler::check_separation_zone() {
	if(separation_zone == NULL) return;
}

inline MidiEvent* MidiEventBuilder::MidiEventChainControler::pop_next_free_midi(size_t size) {
	if(size > 4) throw jException("Illegal call to pop_next_free_midi()", jException::sanity_error);

	if(next_free_midi_event == NULL) {
		init_free_midi_event_chain();
	}

	MidiEventChain *retval = next_free_midi_event;

	next_free_midi_event = next_free_midi_event->next_in_chain;

	// drop tail from the chain we return
	retval->next_in_chain = NULL;
	retval->last_in_chain = retval; // first is last..

	// we will return a MidiEvent, not the chain pointer
	MidiEvent *mev = (MidiEvent *)(&(retval->data));

	mev->length = size;

	return mev;
}

inline void __get_element(MidiEventChain **element, MidiEvent *_element) {
	*element = (MidiEventChain *)(((char *)_element) - offsetof(MidiEventChain, data));
}

inline void MidiEventBuilder::MidiEventChainControler::push_next_free_midi(MidiEvent *_element) {
	MidiEventChain *element; __get_element(&element, _element);
//	MidiEventChain *element = container_of(_element, MidiEventChain, data);

	if(next_free_midi_event != NULL) {
		element->last_in_chain = next_free_midi_event->last_in_chain;
	} else {
		element->last_in_chain = element;
	}

	element->next_in_chain = next_free_midi_event;
	next_free_midi_event = element;
}

MidiEvent* MidiEventBuilder::MidiEventChainControler::get_chain_head(MidiEventChain **queue) {
	if((*queue) == NULL) return NULL;

	MidiEventChain *chain = *queue;

	MidiEvent *mev = (MidiEvent *)((*queue)->data);


	if((*queue)->next_in_chain != NULL)
		(*queue)->next_in_chain->last_in_chain = (*queue)->last_in_chain;

	(*queue) = (*queue)->next_in_chain;
	chain->next_in_chain = NULL;
	chain->last_in_chain = chain;

	return mev;
}

void MidiEventBuilder::MidiEventChainControler::chain_to_tail(MidiEventChain **chain, MidiEvent *event) {
	if(event == NULL) return;
	MidiEventChain *source; __get_element(&source, event);
	join_chains(chain, &source);
}

void MidiEventBuilder::MidiEventChainControler::join_chains(MidiEventChain **destination, MidiEventChain **source) {
	if(*source == NULL) return;
	// if destination is empty it's easy
	if(*destination == NULL) {
		*destination = *source;
		*source = NULL;
		return;
	}

	// otherwise it's a bit more work...
	MidiEventChain *last_entry = (*destination)->last_in_chain;

	// OK, chain source to last entry in destination, set last_in_chain to new last, then clear source pointer
	last_entry->next_in_chain = *source;
	(*destination)->last_in_chain = (*source)->last_in_chain;
	*source = NULL;

}

/*************************************
 *
 * Class MidiEventBuilder
 *
 *************************************/

MidiEventBuilder::MidiEventBuilder() : remaining_midi_chain(NULL), freeable_midi_chain(NULL) {}

void MidiEventBuilder::chain_event(MidiEvent *mev) {
	if(buffer_position >= buffer_size) {
		MidiEventChainControler::chain_to_tail(&(remaining_midi_chain), mev);
	} else {
		buffer[buffer_position++] = mev;
		MidiEventChainControler::chain_to_tail(&(freeable_midi_chain), mev);
	}
}

void MidiEventBuilder::process_freeable_chain() {
	while(freeable_midi_chain != NULL) {
		MidiEvent *mev = MidiEventChainControler::get_chain_head(&freeable_midi_chain);
		MidiEventChainControler::push_next_free_midi(mev);
	}
}

void MidiEventBuilder::process_remaining_chain() {
	while(buffer_position < buffer_size && remaining_midi_chain != NULL) {
		MidiEvent *mev = MidiEventChainControler::get_chain_head(&remaining_midi_chain);

		buffer[buffer_position++] = mev;
		MidiEventChainControler::chain_to_tail(&freeable_midi_chain, mev);
	}
}

void MidiEventBuilder::use_buffer(void **_buffer, int _buffer_size) {
	buffer = _buffer;
	buffer_size = _buffer_size;
	buffer_position = 0;
	buffer_p_last_skip = 0;

	process_remaining_chain();
}

void MidiEventBuilder::finish_current_buffer() {
	buffer_size = 0;
	buffer_position = 1;
	buffer = 0;
	process_freeable_chain();
}

bool MidiEventBuilder::skip(int skip_length) {
	buffer_p_last_skip += skip_length;
	buffer_position = buffer_p_last_skip > buffer_position ? buffer_p_last_skip : buffer_position;
	return buffer_position < buffer_size;
}

void MidiEventBuilder::queue_midi_data(size_t len, const char *data) {
	size_t offset = 0;

	while(offset < len) {
		switch(data[offset] & 0xf0) {
		case MIDI_NOTE_OFF:
			queue_note_off(data[offset + 1], data[offset + 2], data[offset] & 0x0f);
			offset += 3;
			break;
		case MIDI_NOTE_ON:
			queue_note_on(data[offset + 1], data[offset + 2], data[offset] & 0x0f);
			offset += 3;
			break;
		case MIDI_CONTROL_CHANGE:
			queue_controller(data[offset + 1], data[offset + 2], data[offset] & 0x0f);
			offset += 3;
			break;
		case MIDI_PITCH_BEND:
			queue_pitch_bend(data[offset + 1], data[offset + 2], data[offset] & 0x0f);
			offset += 3;
			break;
		default: // not implemented - skip rest of data
			SATAN_ERROR("MidiEventBuilder::queue_midi_data() - Unimplemented status byte - skip rest of data.\n");
			return;
		}
	}
}

void MidiEventBuilder::queue_note_on(int note, int velocity, int channel) {
	MidiEvent *mev = MidiEventChainControler::pop_next_free_midi(3);

	SET_MIDI_DATA_3(
		mev,
		(MIDI_NOTE_ON) | (channel & 0x0f),
		note,
		velocity);

	chain_event(mev);
}

void MidiEventBuilder::queue_note_off(int note, int velocity, int channel) {
	MidiEvent *mev = MidiEventChainControler::pop_next_free_midi(3);

	SET_MIDI_DATA_3(
		mev,
		(MIDI_NOTE_OFF) | (channel & 0x0f),
		note,
		velocity);

	chain_event(mev);
}

void MidiEventBuilder::queue_controller(int controller, int value, int channel) {
	MidiEvent *mev = MidiEventChainControler::pop_next_free_midi(3);

	SATAN_DEBUG("queue_controller(%d, %d, %d)\n", controller, value, channel);

	SET_MIDI_DATA_3(
		mev,
		(MIDI_CONTROL_CHANGE) | (channel & 0x0f),
		controller,
		value);

	chain_event(mev);
}

void MidiEventBuilder::queue_pitch_bend(int value_lsb, int value_msb, int channel) {
	MidiEvent *mev = MidiEventChainControler::pop_next_free_midi(3);

	SET_MIDI_DATA_3(
		mev,
		(MIDI_PITCH_BEND) | (channel & 0x0f),
		value_lsb,
		value_msb);

	chain_event(mev);
}
