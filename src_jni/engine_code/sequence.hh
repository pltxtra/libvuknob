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

#ifndef SEQUENCE_HH
#define SEQUENCE_HH

#ifndef __RI__SERVER_SIDE
#ifndef __RI__CLIENT_SIDE
#error __RI__SERVER_SIDE or __RI__CLIENT_SIDE must be defined.
#endif
#endif

#include <list>

#include "../common.hh"
#include "../linked_list.hh"
#include "../remote_interface.hh"
#include "../serialize.hh"

#include "machine_api.hh"

#ifdef __RI__SERVER_SIDE
#include "../midi_generation.hh"
#include "../machine_sequencer.hh"
#include "controller_envelope.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class Sequence
			: public BaseMachine
		ON_SERVER(, public Machine)
		{
		public:
			enum PadAxis_t {
				pad_x_axis = 0,
				pad_y_axis = 1,
				pad_z_axis = 2,
			};
			enum PadEvent_t {
				ms_pad_press = 0,
				ms_pad_slide = 1,
				ms_pad_release = 2,
				ms_pad_no_event = 3
			};
			enum ArpeggioDirection_t {
				arp_off = 0,
				arp_forward = 1,
				arp_reverse = 2,
				arp_pingpong = 3
			};
			enum ChordMode_t {
				chord_off = 0,
				chord_triad = 1,
				chord_quad = 2
			};

			static constexpr const char* FACTORY_NAME		= "Sequence";

			struct PatternInstance {
				uint32_t pattern_id;
				int start_at, loop_length, stop_at;

				PatternInstance *next; // to support class LinkedList<T>

				inline bool operator==(const PatternInstance& rhs) const {
					bool a = pattern_id == rhs.pattern_id;
					bool b = start_at == rhs.start_at;
					bool c = start_at == rhs.start_at;
					bool d = stop_at == rhs.stop_at;
					return a && b && c && d;
				}

				inline bool operator!=(const PatternInstance& rhs) const {
					return !(*this == rhs);
				}

				inline bool operator<(const PatternInstance& rhs) const {
					return start_at < rhs.start_at;
				}

				inline bool operator>(const PatternInstance& rhs) const {
					return rhs < *this;
				}

			};

			struct Note {
				int channel, // only values that are within (channel & 0x0f)
				    program, // only values that are within (program & 0x7f)
				    velocity,// only values that are within (velocity & 0x7f)
				    note;    // only values that are within (note & 0x7f)
				int on_at, length;

				ON_SERVER(
					int ticks2off; // playback counter
					)

				Note *next; // to support class LinkedList<T>

				static constexpr const char* serialize_identifier = "SequenceNote";
				template <class SerderClassT>
				static void serderize_single(Note *trgt, SerderClassT& iserder);
				template <class SerderClassT> void serderize(SerderClassT& iserder);
				static Note* allocate();

				inline bool operator==(const Note& rhs) const {
					bool a = channel == rhs.channel;
					bool b = program == rhs.program;
					bool c = velocity == rhs.velocity;
					bool d = note == rhs.note;
					bool e = on_at == rhs.on_at;
					bool f = length == rhs.length;
					return a && b && c && d && e && f;
				}

				inline bool operator!=(const Note& rhs) const {
					return !(*this == rhs);
				}

				inline bool operator<(const Note& rhs) const {
					if(on_at != rhs.on_at)
						return on_at < rhs.on_at;

					if(note != rhs.note)
						return note < rhs.note;

					return length < rhs.length;
				}

				inline bool operator>(const Note& rhs) const {
					return rhs < *this;
				}
			};

			ON_CLIENT(
				class SequenceListener {
				public:
					virtual void instance_added(const PatternInstance& instance) = 0;
					virtual void instance_deleted(const PatternInstance& instance) = 0;
				};

				class PatternListener {
				public:
					virtual void note_added(uint32_t pattern_id, const Note &note) = 0;
					virtual void note_deleted(uint32_t pattern_id, const Note &note) = 0;
					virtual void pattern_deleted(uint32_t pattern_id) = 0;
				};

				void insert_pattern_in_sequence(uint32_t pattern_id,
								int start_at,
								int loop_length,
								int stop_at);
				void get_sequence(std::list<PatternInstance> &storage);
				void delete_pattern_from_sequence(const PatternInstance& pattern_instance);

				void add_note(
					uint32_t pattern_id,
					int channel, int program, int velocity,
					int note, int on_at, int length
					);
				void delete_note(uint32_t pattern_id, const Note& note);

				std::string get_name();

				void add_sequence_listener(std::shared_ptr<SequenceListener> sel);
				void add_pattern_listener(uint32_t pattern_id, std::shared_ptr<PatternListener> pal);
				void drop_pattern_listener(std::shared_ptr<PatternListener> pal);

				/* Live (Pad) interfaces */
				std::set<std::string> available_midi_controllers();
				void pad_export_to_loop(int loop_id = RI_LOOP_NOT_SET);
				void pad_set_octave(int octave);
				void pad_set_scale(int scale_index);
				void pad_set_record(bool record);
				void pad_set_quantize(bool do_quantize);
				void pad_assign_midi_controller(PadAxis_t axis, const std::string &controller);
				void pad_set_chord_mode(ChordMode_t chord_mode);
				void pad_set_arpeggio_pattern(const std::string &arp_pattern);
				void pad_set_arpeggio_direction(ArpeggioDirection_t arp_direction);
				void pad_clear();
				void pad_enqueue_event(int finger, PadEvent_t event_type, float ev_x, float ev_y, float ev_z);
				void enqueue_midi_data(size_t len, const char* data);
				);

			ON_CLIENT(
				Sequence(const Factory *factory, const RemoteInterface::Message &serialized);
				);
			ON_SERVER(
				Sequence(int32_t new_obj_id, const Factory *factory);
				static void create_sequence_for_machine(std::shared_ptr<MachineAPI> m_ptr);
				virtual void serialize(std::shared_ptr<Message> &target) override;
				);

			// functions for implementing class Machine
			ON_SERVER(
				// this will detach all connections to/from this
				// machine and then it returns a boolean which
				// tells you if you should delete it or not.
				virtual bool detach_and_destroy();

				// Additional XML-formated descriptive data.
				// Should only be called from monitor protected
				// functions, Machine::*, like get_base_xml_description()
				virtual std::string get_class_name();
				virtual std::string get_descriptive_xml();

				// fill output buffers for a new frame
				virtual void fill_buffers();
				// reset a machine to a defined state
				virtual void reset();
				/// Returns a set of controller groups
				virtual std::vector<std::string> internal_get_controller_groups();
				/// Returns the set of all controller names
				virtual std::vector<std::string> internal_get_controller_names();
				/// Returns the set of all controller names in a given group
				virtual std::vector<std::string> internal_get_controller_names(
					const std::string &group_name);
				/// Returns a controller pointer (remember to delete it...)
				virtual Controller *internal_get_controller(const std::string &name);
				/// get a hint about what this machine is (for example, "effect" or "generator")
				virtual std::string internal_get_hint();
				)

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

		private:
			struct Pattern {
				ON_CLIENT(
					std::set<std::shared_ptr<PatternListener> > pattern_listeners;
					);
				ON_SERVER(
					int current_playing_position = 0;
					Note *next_note_to_play = NULL;
					Note *active_note[MAX_ACTIVE_NOTES];
					)
				uint32_t id;
				std::string name;
				LinkedList<Note> note_list;

				static constexpr const char* serialize_identifier = "SequencePattern";
				template <class SerderClassT> void serderize(SerderClassT& iserder);
				static Pattern* allocate();
			};

			std::map<std::string, std::pair<int, int> > knob2midi;
			ON_SERVER(
				Pad pad;
				MidiEventBuilder _meb;
				static std::map<std::shared_ptr<MachineAPI>, std::shared_ptr<Sequence> > machine2sequence;
				Pattern *current_pattern = NULL;
				void get_midi_controllers(const std::string &name, int &coarse, int &fine) {
					coarse = fine = -1;
					auto found = knob2midi.find(name);
					if(found != knob2midi.end()) {
						auto cnf = found->second;
						coarse = cnf.first;
						fine = cnf.second;
					}
				}
				);

			ON_CLIENT(
				std::set<std::shared_ptr<SequenceListener> > sequence_listeners;
				);

			static ObjectAllocator<Pattern> pattern_allocator;
			static ObjectAllocator<PatternInstance> pattern_instance_allocator;
			static ObjectAllocator<Note> note_allocator;

			ON_SERVER(MachineSequencer* m_seq);
			std::map<uint32_t, Pattern*> patterns;
			LinkedList<PatternInstance> instance_list;
			std::string sequence_name;
			bool mute = false;

			/* internal functions */
			ON_CLIENT(
				void get_notes(uint32_t pattern_id, std::list<Note> &storage);
				);
			ON_SERVER(

				uint32_t get_pattern_starting_at(int sequence_position);
				void start_to_play_pattern(Pattern *pattern_to_play);
				bool activate_note(Pattern *p, Note *note);
				void deactivate_note(Pattern *p, Note *note);
				void process_note_on(Pattern *p, bool mute, MidiEventBuilder *_meb);
				void process_note_off(Pattern *p, MidiEventBuilder *_meb);
				void process_current_pattern(bool mute, MidiEventBuilder *_meb);
				/*************
				 *
				 * controller envelopes - the envelopes start at global line 0 and continue as long as they have control
				 *                        points. It's one envelope for each controller that supports MIDI. The envelopes
				 *                        are not stored in the loops, so you have one for the whole sequence
				 *
				 *****/
//				void create_controller_envelopes();
//				void process_controller_envelopes(int current_tick, MidiEventBuilder *_meb);
//				std::map<std::string, ControllerEnvelope *> controller_envelope; // a map of controller names to their envelopes
				);

			// This function makes sure that a required pattern exists
			Pattern* require_pattern(uint32_t pattern_id);

			/* internal processing of commands - on both the server & client */
			bool process_add_pattern_instance(uint32_t pattern_id,
							  int start_at,
							  int loop_length,
							  int stop_at);
			void process_del_pattern_instance(const PatternInstance& pattern_instance);
			void process_add_note(uint32_t pattern_id,
					      int channel, int program, int velocity,
					      int note, int on_at, int length);
			void process_delete_note(uint32_t pattern_id, const Note& note);

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_add_pattern_instance, "req_add_ptrn_inst");
			SERVER_SIDE_HANDLER(req_del_pattern_instance, "req_del_ptrn_inst");
			SERVER_SIDE_HANDLER(req_add_note, "req_add_note");
			SERVER_SIDE_HANDLER(req_del_note, "req_del_note");
			SERVER_SIDE_HANDLER(req_pad_set_octave, "req_pad_set_octave");
			SERVER_SIDE_HANDLER(req_pad_set_scale, "req_pad_set_scale");
			SERVER_SIDE_HANDLER(req_pad_record, "req_pad_set_record");
			SERVER_SIDE_HANDLER(req_pad_quantize, "req_pad_set_quantize");
			SERVER_SIDE_HANDLER(req_pad_assign_midi_controller, "req_pad_assign_midi_controller");
			SERVER_SIDE_HANDLER(req_pad_set_chord_mode, "req_pad_set_chord_mode");
			SERVER_SIDE_HANDLER(req_pad_set_arpeggio_pattern, "req_pad_set_arpeggio_pattern");
			SERVER_SIDE_HANDLER(req_pad_set_arpeggio_direction, "req_pad_set_arpeggio_direction");
			SERVER_SIDE_HANDLER(req_pad_clear, "req_pad_clear");
			SERVER_SIDE_HANDLER(req_pad_enqueue_event, "req_pad_enqueue_event");
			SERVER_SIDE_HANDLER(req_enqueue_midi_data, "req_enqueue_midi_data");

			CLIENT_SIDE_HANDLER(cmd_add_pattern_instance, "cmd_add_ptrn_inst");
			CLIENT_SIDE_HANDLER(cmd_del_pattern_instance, "cmd_del_ptrn_inst");
			CLIENT_SIDE_HANDLER(cmd_add_note, "cmd_add_note");
			CLIENT_SIDE_HANDLER(cmd_del_note, "cmd_del_note");

			void register_handlers() {
				SERVER_REG_HANDLER(Sequence,req_add_pattern_instance);
				SERVER_REG_HANDLER(Sequence,req_del_pattern_instance);
				SERVER_REG_HANDLER(Sequence,req_add_note);
				SERVER_REG_HANDLER(Sequence,req_del_note);
				SERVER_REG_HANDLER(Sequence,req_pad_set_octave);
				SERVER_REG_HANDLER(Sequence,req_pad_set_scale);
				SERVER_REG_HANDLER(Sequence,req_pad_record);
				SERVER_REG_HANDLER(Sequence,req_pad_quantize);
				SERVER_REG_HANDLER(Sequence,req_pad_assign_midi_controller);
				SERVER_REG_HANDLER(Sequence,req_pad_set_chord_mode);
				SERVER_REG_HANDLER(Sequence,req_pad_set_arpeggio_pattern);
				SERVER_REG_HANDLER(Sequence,req_pad_set_arpeggio_direction);
				SERVER_REG_HANDLER(Sequence,req_pad_clear);
				SERVER_REG_HANDLER(Sequence,req_pad_enqueue_event);
				SERVER_REG_HANDLER(Sequence,req_enqueue_midi_data);

				CLIENT_REG_HANDLER(Sequence,cmd_add_pattern_instance);
				CLIENT_REG_HANDLER(Sequence,cmd_del_pattern_instance);
				CLIENT_REG_HANDLER(Sequence,cmd_add_note);
				CLIENT_REG_HANDLER(Sequence,cmd_del_note);
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize_sequence(SerderClassT &serder);

		public:
			class SequenceFactory
				: public FactoryTemplate<Sequence>
			{
			public:
				ON_SERVER(SequenceFactory()
					  : FactoryTemplate<Sequence>(ServerSide, FACTORY_NAME)
					  {}
					);
				ON_CLIENT(SequenceFactory()
					  : FactoryTemplate<Sequence>(ClientSide, FACTORY_NAME)
					  {}
					);
				virtual std::shared_ptr<BaseObject> create(
					const Message &serialized,
					RegisterObjectFunction register_object
					) override;
				virtual std::shared_ptr<BaseObject> create(
					int32_t new_obj_id,
					RegisterObjectFunction register_object
					) override;

			};
		};
	};
};

#endif
