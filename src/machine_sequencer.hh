/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
 * Copyright (C) 2011 by Anton Persson
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

#ifndef CLASS_MACHINE_SEQUENCER
#define CLASS_MACHINE_SEQUENCER

#include <stdint.h>

#include <jngldrum/jthread.hh>
#include "machine.hh"
#include "readerwriterqueue/readerwriterqueue.h"

#include "midi_export.hh"

#include <iostream>
#include <fstream>
#include <queue>

#include "midi_generation.hh"
#include "engine_code/pad.hh"
#include "engine_code/controller_envelope.hh"

// Each machine with a "midi" input will automatically
// be attached to a MachineSequencer machine (invisible from the
// user interface...) The MachineSequencer will be "tightly connected"
// to the "midi"-machine which means that it will be automatically
// destroyed when the "midi"-machine is destroyed.

// I intentionaly kept it at 28 bits, so don't change it..
#define MAX_SEQUENCE_LENGTH (0x0fffffff)
#define NOTE_NOT_SET -1
#define LOOP_NOT_SET -1
#define MACHINE_SEQUENCER_MIDI_INPUT_NAME "midi"
#define MACHINE_SEQUENCER_MIDI_OUTPUT_NAME "midi_OUTPUT"
#define MACHINE_SEQUENCER_INITIAL_SEQUENCE_LENGTH 16
#define MACHINE_SEQUENCER_MAX_EDITABLE_LOOP_OFFSET 4096

#define MAX_ACTIVE_NOTES 16

// PAD_TIME(line,tick)
#define PAD_TIME(A,B) ((int)(((A << BITS_PER_LINE) & (0xffffffff & (0xffffffff << BITS_PER_LINE))) | (B)))
#define PAD_TIME_LINE(A) (A >> BITS_PER_LINE)
#define PAD_TIME_TICK(B) (B & (((0xffffffff << BITS_PER_LINE) & 0xffffffff) ^ 0xffffffff))

struct ltstr {
	bool operator()(std::string s1, std::string s2) const {
		return strcmp(s1.c_str(), s2.c_str()) < 0;
	}
};

typedef void (*MachineSequencerSetChangeCallbackF_t)(void *);

class MachineSequencer : public Machine {
	/***** OK - REAL WORK AHEAD *****/

public:
	/*
	 * exception classes
	 *
	 */
	class NoFreeLoopsAvailable : public std::exception {
	public:
		virtual const char* what() const noexcept {
			return "The application cannot allocate more loops for this machine.";
		};
	};

	class NoSuchLoop : public std::exception {
	public:
		virtual const char* what() const noexcept {
			return "A bad loop id was used.";
		};
	};

	// declare comming classes
	class Loop;
	class NoteEntry;

public:
	class NoSuchController : public jException {
	public:
		NoSuchController(const std::string &name);
	};

	class NoteEntry {
	public:
		uint8_t channel, // only values that are within (channel & 0x0f)
			program, // only values that are within (program & 0x7f)
			velocity,// only values that are within (velocity & 0x7f)
			note; // only values that are within (note & 0x7f)

		// note on position and length in the loop, encoded with PAD_TIME(line,tick)
		int on_at, length;

		// double linked list of notes in a Loop object
		// please observe that the last note in the loop does _NOT_ link
		// to the first to actually create a looping linked list..
		NoteEntry *prev;
		NoteEntry *next;

		// playback counters
		int ticks2off;

		NoteEntry(const NoteEntry *original);
		NoteEntry();
		virtual ~NoteEntry();

		void set_to(const NoteEntry *new_values);
	};


private:

	class LoopCreator {
	public:
		static Loop *create(const KXMLDoc &loop_xml);
		static Loop *create();
	};

public:

	class Loop {
	private:
		// during playback this is used to keep track of the
		// current playing line in the loop
		int loop_position;

		/******** notes *******/
		NoteEntry *note;                      // all notes
		NoteEntry *next_note_to_play;         // short cut to the next note to play
		NoteEntry *active_note[MAX_ACTIVE_NOTES];

		// let the loop creator create objects of our class
		friend class LoopCreator;

		Loop(const KXMLDoc &loop_xml);
		void process_note_on(bool mute, MidiEventBuilder *_meb);
		void process_note_off(MidiEventBuilder *_meb);

		NoteEntry *internal_delete_note(const NoteEntry *net);
		void internal_update_note(const NoteEntry *original, const NoteEntry *new_entry);
		NoteEntry *internal_clear();
		NoteEntry *internal_insert_notes(NoteEntry *new_notes);

		bool activate_note(NoteEntry *);
		void deactivate_note(NoteEntry *);
	public:
		const NoteEntry *internal_insert_note(NoteEntry *new_entry);

		Loop();
		~Loop();

		void get_loop_xml(std::ostringstream &stream, int id);

		void start_to_play();

		void process(bool mute, MidiEventBuilder *meb);

		const NoteEntry *notes_get() const;

		void note_delete(const NoteEntry *net);
		void note_update(const NoteEntry *original, const NoteEntry *new_entry);
		const NoteEntry *note_insert(const NoteEntry *new_entry);
		void clear_loop();
		void copy_loop(const Loop *source);

		NoteEntry* new_note() {
			return new NoteEntry();
		}
	};

	/*************
	 *
	 * Public interface
	 *
	 *************/
public:
	// id == -1 for "no entry"
	// id >= 0 for valid entry
	void get_loop_ids_at(int *position_vector, int length);
	int get_loop_id_at(int position);
	void set_loop_id_at(int position, int loop_id);
	Loop *get_loop(int loop_id);
	int get_nr_of_loops();
	int add_new_loop(); // returns id of new loop
	void delete_loop(int id);

	bool is_mute();
	void set_mute(bool muted);

       	// used to list controllers that can be accessed via MIDI
	std::set<std::string> available_midi_controllers();
	// assign the pad to a specific MIDI controller - if "" or if the string does not match a proper MIDI controller
	// the pad will be assigned to velocity
	// if the pad is assigned to a midi controller, the velocity will default to full amplitude
	void assign_pad_to_midi_controller(Pad::PadConfiguration::PadAxis p_axis, const std::string &);

	Pad *get_pad();

	void set_pad_arpeggio_pattern(const std::string identity);
	void set_pad_arpeggio_direction(Pad::PadConfiguration::ArpeggioDirection arp_dir);
	void set_pad_chord_mode(Pad::PadConfiguration::ChordMode pconf);
	void export_pad_to_loop(int loop_id = LOOP_NOT_SET);

	const ControllerEnvelope *get_controller_envelope(const std::string &name) const;
	void update_controller_envelope(const ControllerEnvelope *original, const ControllerEnvelope *new_entry);

	void enqueue_midi_data(size_t len, const char* data);

	/// returns a pointer to the machine this MachineSequencer is feeding.
	Machine *get_machine();

	/*************
	 *
	 * Inheritance stuff
	 *
	 *************/
protected:
	/// Returns a set of controller groups
	virtual std::vector<std::string> internal_get_controller_groups();

	/// Returns a set of controller names
	virtual std::vector<std::string> internal_get_controller_names();
	virtual std::vector<std::string> internal_get_controller_names(const std::string &group_name);

	/// Returns a controller pointer (remember to delete it...)
	virtual Controller *internal_get_controller(const std::string &name);

	/// get a hint about what this machine is (for example, "effect" or "generator")
	virtual std::string internal_get_hint();

	virtual bool detach_and_destroy();

	// Additional XML-formated descriptive data.
	// functions, Machine::*, like get_base_xml_description()
	virtual std::string get_class_name();
	virtual std::string get_descriptive_xml();

	virtual void fill_buffers();

	// reset a machine to a defined state
	virtual void reset();

	/*************
	 *
	 * Dynamic class/object stuff
	 *
	 *************/
private:
	/* stuff related to the fill_buffers() function and sub-functions */
	Pad pad;
	MidiEventBuilder _meb;
	Loop *current_loop;

	void prep_empty_loop_sequence();
	int internal_get_loop_id_at(int position);
	void internal_get_loop_ids_at(int *position_vector, int length);
	void internal_set_loop_id_at(int position, int loop_id);
	int internal_add_new_loop();
	void get_loops_xml(std::ostringstream &stream);
	void get_loop_sequence_xml(std::ostringstream &stream);

	std::set<std::string> internal_available_midi_controllers();

	/*************
	 *
	 * controller envelopes - the envelopes start at global line 0 and continue as long as they have control
	 *                        points. It's one envelope for each controller that supports MIDI. The envelopes
	 *                        are not stored in the loops, so you have one for the whole sequence
	 *
	 *****/
	void create_controller_envelopes();
	void process_controller_envelopes(int current_tick, MidiEventBuilder *_meb);
	std::map<std::string, ControllerEnvelope *> controller_envelope; // a map of controller names to their envelopes

	/* rest of it */
	bool mute; // don't start to play new tones..
	std::string sibling_name; // this MachineSequencer generates events to a specific sibling
	Loop **loop_store;
	int loop_store_size, last_free_loop_store_position;

	void double_loop_store();

	int *loop_sequence;
	int loop_sequence_length;

	void refresh_length();

	virtual ~MachineSequencer();
	MachineSequencer(int project_interface_level, const KXMLDoc &machine_xml, const std::string &name);
	MachineSequencer(const std::string &name_root);

	// get the coarse and fine midi controller id:s for a named controller
	void get_midi_controllers(const std::string &name, int &coarse, int &fine);

	// write MIDI information track to file
	static void write_MIDI_info_track(MidiExport *target);
	// returns the nr of loops exported (= midi tracks)
	int export_loops2midi_file(MidiExport *target);
	// will export entire sequence as one track, returns true if the sequence contains data, false if empty.
	bool export_sequence2midi_file(MidiExport *target);

	/****************************
	 *
	 *    Static private interface and data
	 *
	 ****************************/
private:
	// static book-keeping
	static std::vector<MachineSequencer *> machine_from_xml_cache; // this cache is used for temporary storing machine sequencer objects until all machines in the xml file has been loaded. After that we can connect machine sequencers to their siblings.
	static std::map<Machine *, MachineSequencer *> machine2sequencer;
	static int sequence_length;
	static std::vector<MachineSequencerSetChangeCallbackF_t> set_change_callback;

	// tell all listeners something was updated
	static void inform_registered_callbacks_async();

	// Set the length of the sequences of ALL MachineSequencer objects.
	// To keep everything synchronized there is no
	// public interface to set a specific machine sequencer's sequence
	// length.
	static void set_sequence_length(int new_size);

	/****************************
	 *
	 *    Static public interface
	 *
	 ****************************/
public:
	static int quantize_tick(int start_tick); // helper, quantize a tick value into the closest absolute line

	static void presetup_from_xml(int project_interface_level, const KXMLDoc &machine_xml);
	static void finalize_xml_initialization();

	static void create_sequencer_for_machine(Machine *m);
	static MachineSequencer *get_sequencer_for_machine(Machine *m);
	static std::map<std::string, MachineSequencer *, ltstr> get_sequencers();

	static std::vector<std::string> get_pad_arpeggio_patterns();

	/// register a call-back that will be called when the MachineSequencer set is changed
	static void register_change_callback(
		void (*callback_f)(void *));

	/// This will export the entire sequence to a MIDI, type 2, file
	static void export2MIDI(bool just_loops, const std::string &output_path);

};

#endif
