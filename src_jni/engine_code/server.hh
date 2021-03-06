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

#ifndef SERVER_HH
#define SERVER_HH

#include <list>

#include "../common.hh"
#include "../remote_interface.hh"
#include "sequence.hh"


SERVER_CODE(

	class Server
	: public Context
	, public Machine::MachineSetListener
	, public std::enable_shared_from_this<Server>
	{
	private:
		/**** begin service objects data and logic ****/

		std::map<int32_t, std::shared_ptr<BaseObject> > all_objects;

		int32_t last_obj_id; // I am making an assumption here that last_obj_id will not be counted up more than 1/sec. This gives that time until overflow for a session will be more than 20000 days. If this assumption does not hold, an error state will be communicated to the user.

		std::map<std::shared_ptr<Machine>, std::shared_ptr<RIMachine> > machine2rimachine;

		int32_t reserve_new_obj_id();

		template <typename T>
		void remember_object(
			std::shared_ptr<T> new_obj,
			std::function<void(std::shared_ptr<T> nuobj)> new_object_init_callback
			) {
			all_objects[new_obj->get_obj_id()] = new_obj;

			new_object_init_callback(new_obj);

			std::shared_ptr<Message> create_object_message = acquire_message();
			add_create_object_header(create_object_message, new_obj);
			new_obj->serialize(create_object_message);
			distribute_message(create_object_message, false);
		}

		template <typename T>
		void create_object_from_factory(
			std::function<void(std::shared_ptr<T> nuobj)> new_object_init_callback) {
			int32_t new_obj_id = reserve_new_obj_id();

			auto new_obj =
				BaseObject::create_object_on_server<T>(this, new_obj_id);

			remember_object<T>(new_obj, new_object_init_callback);
		}

		void delete_object(std::shared_ptr<BaseObject> obj2delete);

		/* these virtual functions implement the MachineSetListener interface */
		virtual void project_loaded() override;
		virtual void machine_registered(std::shared_ptr<Machine> m_ptr) override;
		virtual void machine_unregistered(std::shared_ptr<Machine> m_ptr) override;
		virtual void machine_input_attached(std::shared_ptr<Machine> source,
						    std::shared_ptr<Machine> destination,
						    const std::string &output_name,
						    const std::string &input_name) override;
		virtual void machine_input_detached(std::shared_ptr<Machine> source,
						    std::shared_ptr<Machine> destination,
						    const std::string &output_name,
						    const std::string &input_name) override;

		std::shared_ptr<HandleList> handle_list;

		/**** end service objects data and logic ****/

		class ClientAgent : public MessageHandler {
		private:
			int32_t id;
			Server *server;

			void send_handler_message();
		public:
			ClientAgent(int32_t id, asio::ip::tcp::socket _socket, Server *server);
			void start();

			void disconnect();

			int32_t get_id() { return id; }

			virtual void on_message_received(const Message &msg) override;
			virtual void on_connection_dropped() override;
		};
		friend class ClientAgent;

		typedef std::shared_ptr<ClientAgent> ClientAgent_ptr;
		std::map<int32_t, ClientAgent_ptr> client_agents;
		int32_t next_client_agent_id = 0;

		asio::ip::tcp::acceptor acceptor;
		asio::ip::tcp::socket acceptor_socket;
		int current_port;

		std::shared_ptr<asio::ip::udp::socket> udp_socket;
		Message udp_read_msg;
		asio::ip::udp::endpoint udp_endpoint;

		void do_accept();
		void drop_client(std::shared_ptr<ClientAgent> client_agent);
		void do_udp_receive();

		void disconnect_clients();
		void create_service_objects();
		void add_create_object_header(std::shared_ptr<Message> &target, std::shared_ptr<BaseObject> obj);
		void add_destroy_object_header(std::shared_ptr<Message> &target, std::shared_ptr<BaseObject> obj);
		void send_protocol_version_to_new_client(std::shared_ptr<MessageHandler> client_agent);
		void send_client_id_to_new_client(std::shared_ptr<ClientAgent> client_agent);
		void send_all_objects_to_new_client(std::shared_ptr<MessageHandler> client_agent);

		int get_port();

		Server(const asio::ip::tcp::endpoint& endpoint);

		void route_incomming_message(ClientAgent *src, const Message &msg);

		static std::shared_ptr<Server> server;
		static std::mutex server_mutex;

	protected:
		virtual void on_remove_object(int32_t objid) override;

	public:
		template <typename T>
		static void create_object(
			std::function<void(std::shared_ptr<T> nuobj)> new_object_init_callback) {
			std::lock_guard<std::mutex> lock_guard(server_mutex);

			if(server) {
				server->post_action(
					[new_object_init_callback]() {
						server->create_object_from_factory<T>(new_object_init_callback);
					}
					, true
					);
				return;
			}

			throw ContextNotConnected();
		}

		static int start_server(); // will start a server and return the port number. If the server is already started, it will just return the port number.
		static bool is_running();
		static void stop_server();

		virtual void distribute_message(std::shared_ptr<Message> &msg, bool via_udp = false) override;
		virtual std::shared_ptr<BaseObject> get_object(int32_t objid) override;
	};

	);

extern "C" {
	extern int __RI_server_is_running();
	extern int __RI_start_server();
	extern void __RI_stop_server();
};

#endif
