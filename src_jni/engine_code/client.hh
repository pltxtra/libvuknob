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

#ifndef CLIENT_HH
#define CLIENT_HH

#include <list>

#include "../common.hh"
#include "../remote_interface.hh"

namespace RemoteInterface {
	namespace __RI__CURRENT_NAMESPACE {

		class Client : public Context, public MessageHandler {
		private:
			std::map<int32_t, std::shared_ptr<BaseObject> > all_objects;

			int32_t client_id = -1;

			// code for handling messages waiting for a reply
			int32_t next_reply_id = 0;
			std::map<int32_t, std::shared_ptr<Message> > msg_waiting_for_reply;

			std::map<std::string, std::string> handle2hint;

			asio::ip::tcp::resolver resolver;
			asio::ip::udp::resolver udp_resolver;
			std::function<void()> disconnect_callback;
			std::function<void(const std::string &fresp)> failure_response_callback;

			Client(const std::string &server_host,
			       int server_port,
			       std::function<void()> disconnect_callback,
			       std::function<void(const std::string &failure_response)> failure_response_callback);

			void flush_all_objects();

			static std::shared_ptr<Client> client;
			static std::mutex client_mutex;

		protected:
			virtual void on_remove_object(int32_t objid) override;

		public: // public singleton interface
			static void start_client(const std::string &server_host, int server_port,
						 std::function<void()> disconnect_callback,
						 std::function<void(const std::string &failure_response)> failure_response_callback);
			static void disconnect();

			static void register_ri_machine_set_listener(std::weak_ptr<RIMachine::RIMachineSetListener> ri_mset_listener);

		public:
			virtual void on_message_received(const Message &msg) override;
			virtual void on_connection_dropped() override;

			virtual void distribute_message(std::shared_ptr<Message> &msg, bool via_udp) override;
			virtual std::shared_ptr<BaseObject> get_object(int32_t objid) override;
		};
	};
};

#endif
