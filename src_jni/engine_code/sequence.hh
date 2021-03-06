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

#include <list>

#include "../common.hh"
#include "../linked_list.hh"
#include "../remote_interface.hh"
#include "../serialize.hh"

#ifdef __RI__SERVER_SIDE
#include "../machine_sequencer.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class Sequence
			: public RemoteInterface::SimpleBaseObject
		ON_SERVER(, public Machine)
		{
		public:
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
					return on_at < rhs.on_at;
				}

				inline bool operator>(const Note& rhs) const {
					return rhs < *this;
				}
			};

			ON_CLIENT(
				void add_pattern(const std::string& name);
				void get_pattern_ids(std::list<uint32_t> &storage);
				void delete_pattern(uint32_t pattern_id);

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
				void get_notes(uint32_t pattern_id, std::list<Note> &storage);
				void delete_note(uint32_t pattern_id, const Note& note);

				std::string get_name();
				);

			Sequence(const Factory *factory, const RemoteInterface::Message &serialized);
			Sequence(int32_t new_obj_id, const Factory *factory);

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
				);
			ON_SERVER(
				static void create_sequence_for_machine(std::shared_ptr<Machine> m_ptr);
				virtual void serialize(std::shared_ptr<Message> &target) override;
				);

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

		private:
			ON_SERVER(
				static std::map<std::shared_ptr<Machine>,
				std::shared_ptr<Sequence> > machine2sequence;
				);

			struct Pattern {
				uint32_t id;
				std::string name;
				LinkedList<Note> note_list;

				static constexpr const char* serialize_identifier = "SequencePattern";
				template <class SerderClassT> void serderize(SerderClassT& iserder);
				static Pattern* allocate();
			};

			static ObjectAllocator<Pattern> pattern_allocator;
			static ObjectAllocator<PatternInstance> pattern_instance_allocator;
			static ObjectAllocator<Note> note_allocator;

			ON_SERVER(MachineSequencer* m_seq);
			std::map<uint32_t, Pattern*> patterns;
			LinkedList<PatternInstance> instance_list;
			std::string sequence_name;

			/* internal processing of commands - on both the server & client */
			void process_add_pattern(const std::string& new_name, uint32_t new_id);
			bool process_del_pattern(uint32_t pattern_id);
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

			SERVER_SIDE_HANDLER(req_add_pattern, "req_add_ptrn");
			SERVER_SIDE_HANDLER(req_del_pattern, "req_del_ptrn");
			SERVER_SIDE_HANDLER(req_add_pattern_instance, "req_add_ptrn_inst");
			SERVER_SIDE_HANDLER(req_del_pattern_instance, "req_del_ptrn_inst");
			SERVER_SIDE_HANDLER(req_add_note, "req_add_note");
			SERVER_SIDE_HANDLER(req_del_note, "req_del_note");

			CLIENT_SIDE_HANDLER(cmd_add_pattern, "cmd_add_ptrn");
			CLIENT_SIDE_HANDLER(cmd_del_pattern, "cmd_del_ptrn");
			CLIENT_SIDE_HANDLER(cmd_add_pattern_instance, "cmd_add_ptrn_inst");
			CLIENT_SIDE_HANDLER(cmd_del_pattern_instance, "cmd_del_ptrn_inst");
			CLIENT_SIDE_HANDLER(cmd_add_note, "cmd_add_note");
			CLIENT_SIDE_HANDLER(cmd_del_note, "cmd_del_note");

			void register_handlers() {
				SERVER_REG_HANDLER(Sequence,req_add_pattern);
				SERVER_REG_HANDLER(Sequence,req_del_pattern);
				SERVER_REG_HANDLER(Sequence,req_add_pattern_instance);
				SERVER_REG_HANDLER(Sequence,req_del_pattern_instance);
				SERVER_REG_HANDLER(Sequence,req_add_note);
				SERVER_REG_HANDLER(Sequence,req_del_note);

				CLIENT_REG_HANDLER(Sequence,cmd_add_pattern);
				CLIENT_REG_HANDLER(Sequence,cmd_del_pattern);
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
				virtual std::shared_ptr<BaseObject> create(const Message &serialized) override;
				virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) override;

			};
		};
	};
};

#endif
