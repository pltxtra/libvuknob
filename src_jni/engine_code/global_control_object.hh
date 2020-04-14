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

#ifndef GLOBAL_CONTROL_OBJECT_HH
#define GLOBAL_CONTROL_OBJECT_HH

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

#ifdef __RI__SERVER_SIDE
#include "../machine.hh"
#endif

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class GlobalControlObject
			: public RemoteInterface::SimpleBaseObject
		ON_SERVER(
			, public Machine::PlaybackStateListener
			)
		{
		public:
			static constexpr const char* FACTORY_NAME		= "GlobCon";

			ON_CLIENT(
				class GlobalControlListener {
				public:
					virtual void loop_state_changed(bool new_state) {};
					virtual void loop_start_changed(int new_start) {};
					virtual void loop_length_changed(int new_length) {};
					virtual void playback_state_changed(bool playing) {};
					virtual void record_state_changed(bool recording) {};
					virtual void bpm_changed(int bpm) {};
					virtual void lpb_changed(int lpb) {};
					virtual void row_update(int row) {};
				};

				void add_global_control_listener(std::shared_ptr<GlobalControlListener> glol);

				void set_loop_state(bool new_state);
				void set_loop_start(int new_start);
				void set_loop_length(int new_length);

				void play();
				void stop();

				void set_record_state(bool record);

				void set_bpm(int bpm);
				void set_lpb(int lpb);
				);

			GlobalControlObject(const Factory *factory, const RemoteInterface::Message &serialized);
			GlobalControlObject(int32_t new_obj_id, const Factory *factory);

			ON_SERVER(
				virtual void serialize(std::shared_ptr<Message> &target) override;

				virtual void project_loaded() override {};
				virtual void loop_state_changed(bool loop_state) override;
				virtual void loop_start_changed(int loop_start) override;
				virtual void loop_length_changed(int loop_length) override;
				virtual void playback_state_changed(bool playing) override;
				virtual void record_state_changed(bool recording) override;
				virtual void bpm_changed(int bpm) override;
				virtual void lpb_changed(int lbp) override;
				)

			virtual void on_delete(RemoteInterface::Context* context) override {
				context->unregister_this_object(this);
			}

		private:
			bool loop_state, playing, record;
			int loop_start, loop_length, bpm, lpb;

			ON_CLIENT(
				std::set<std::shared_ptr<GlobalControlListener> > gco_listeners;
				);

			ON_SERVER(
				void send_periodic_update(int row);
				);

			/* REQ means the client request the server to perform an operation */
			/* CMD means the server commands the client to perform an operation */

			SERVER_SIDE_HANDLER(req_set_loop_state, "req_set_loop_state");
			SERVER_SIDE_HANDLER(req_set_loop_start, "req_set_loop_start");
			SERVER_SIDE_HANDLER(req_set_loop_length, "req_set_loop_length");
			SERVER_SIDE_HANDLER(req_set_playback_state, "req_set_playback_state");
			SERVER_SIDE_HANDLER(req_set_record_state, "req_set_record_state");
			SERVER_SIDE_HANDLER(req_set_bpm, "req_set_bpm");
			SERVER_SIDE_HANDLER(req_set_lpb, "req_set_lpb");

			CLIENT_SIDE_HANDLER(cmd_set_loop_state, "cmd_set_loop_state");
			CLIENT_SIDE_HANDLER(cmd_set_loop_start, "cmd_set_loop_start");
			CLIENT_SIDE_HANDLER(cmd_set_loop_length, "cmd_set_loop_length");
			CLIENT_SIDE_HANDLER(cmd_set_playback_state, "cmd_set_playback_state");
			CLIENT_SIDE_HANDLER(cmd_set_record_state, "cmd_set_record_state");
			CLIENT_SIDE_HANDLER(cmd_set_bpm, "cmd_set_bpm");
			CLIENT_SIDE_HANDLER(cmd_set_lpb, "cmd_set_lpb");
			CLIENT_SIDE_HANDLER(cmd_per_upd, "cmd_per_upd");

			void register_handlers() {
				SERVER_REG_HANDLER(GlobalControlObject,req_set_loop_state);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_loop_start);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_loop_length);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_playback_state);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_record_state);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_bpm);
				SERVER_REG_HANDLER(GlobalControlObject,req_set_lpb);

				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_loop_state);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_loop_start);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_loop_length);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_playback_state);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_record_state);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_bpm);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_set_lpb);
				CLIENT_REG_HANDLER(GlobalControlObject,cmd_per_upd);
			}

			// serder is an either an ItemSerializer or ItemDeserializer object.
			template <class SerderClassT>
			void serderize(SerderClassT &serder);

		public:
			class GlobalControlObjectFactory
				: public FactoryTemplate<GlobalControlObject>
			{
			public:
				ON_SERVER(GlobalControlObjectFactory()
					  : FactoryTemplate<GlobalControlObject>(ServerSide, FACTORY_NAME, true)
					  {}
					);
				ON_CLIENT(GlobalControlObjectFactory()
					  : FactoryTemplate<GlobalControlObject>(ClientSide, FACTORY_NAME)
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
