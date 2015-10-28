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
#include "../remote_interface.hh"

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class Sequence : public RemoteInterface::SimpleBaseObject {
		public:
			struct PatternInstance {
				uint32_t pattern_id;
				int start_at, loop_length, stop_at;
			};

			struct Note {
				int note, velocity;
				int start_at, length;
				Note *next_note;
			};

			void add_pattern(const std::string& name);
			void get_pattern_ids(std::list<uint32_t> &storage);
			void delete_pattern(uint32_t pattern_id);

			void insert_pattern_in_sequence(uint32_t pattern_id,
							int start_at,
							int loop_length,
							int stop_at);
			void get_sequence(std::list<PatternInstance> &storage);
			void delete_pattern_from_sequence(const PatternInstance& pattern_instance);

			void insert_note(uint32_t pattern_id,
					 int note, int velocity,
					 int start_at, int length);
			void get_notes(uint32_t pattern_id, std::list<Note> &storage);
			void delete_note(uint32_t pattern_id, const Note& note);

		private:
			struct Pattern {
				uint32_t id;
				std::string name;
				Note *first_note;
			};

			static ObjectAllocator<Pattern> pattern_allocator;
			static ObjectAllocator<Note> note_allocator;

			std::map<uint32_t, Pattern*> patterns;

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

		};
	};
};

#endif
