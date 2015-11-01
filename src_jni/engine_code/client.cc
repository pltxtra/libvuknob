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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include "client.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

CLIENT_CODE(

	std::shared_ptr<Client> Client::client;
	std::mutex Client::client_mutex;

	Client::Client(const std::string &server_host,
		       int server_port,
		       std::function<void()> _disconnect_callback,
		       std::function<void(const std::string &failure_response)> _failure_response_callback)
	: MessageHandler(io_service)
	, resolver(io_service)
	, udp_resolver(io_service)
	, disconnect_callback(_disconnect_callback)
	{
		failure_response_callback = _failure_response_callback;
		auto endpoint_iterator = resolver.resolve({server_host, std::to_string(server_port) });

		asio::async_connect(my_socket, endpoint_iterator,
				    [this](std::error_code ec, asio::ip::tcp::resolver::iterator)
				    {
					    if (!ec)
					    {
						    start_receive();
					    } else {
						    SATAN_ERROR("Failed to connect to server.\n");
					    }
				    }
			);

		udp_target_endpoint = *udp_resolver.resolve(
			{asio::ip::udp::v4(), server_host, std::to_string(server_port) }
			);
	}

	void Client::flush_all_objects() {
		for(auto objid2obj : all_objects) {
			objid2obj.second->on_delete(this);
		}
		all_objects.clear();

	}

	void Client::on_message_received(const Message &msg) {
		int identifier = std::stol(msg.get_value("id"));

		SATAN_DEBUG("Client() received message: %d\n", identifier);

		switch(identifier) {
		case __MSG_PROTOCOL_VERSION:
		{
			auto protocol_version = std::stol(msg.get_value("pversion"));
			if(protocol_version > __VUKNOB_PROTOCOL_VERSION__) {
				failure_response_callback("Server is to new - you must upgrade before you can connect.");
				disconnect();
			}
		}
		break;

		case __MSG_CLIENT_ID:
		{
			client_id = std::stol(msg.get_value("clid"));
		}
		break;

		case __MSG_CREATE_OBJECT:
		{
			SATAN_DEBUG("Client - Creating object from message...\n");
			std::shared_ptr<BaseObject> new_obj = BaseObject::create_object_on_client(msg);
			new_obj->set_context(this);

			int32_t obj_id = new_obj->get_obj_id();

			if(all_objects.find(obj_id) != all_objects.end()) throw BaseObject::DuplicateObjectId();
			if(obj_id < 0) throw BaseObject::ObjIdOverflow();

			all_objects[new_obj->get_obj_id()] = new_obj;
		}
		break;

		case __MSG_FLUSH_ALL_OBJECTS:
		{
			flush_all_objects();
		}
		break;

		case __MSG_REPLY:
		{
			SATAN_DEBUG("Will get repid...\n");

			int32_t reply_identifier = std::stol(msg.get_value("repid"));
			SATAN_DEBUG("Reply received: %d\n", reply_identifier);
			auto repmsg = msg_waiting_for_reply.find(reply_identifier);
			if(repmsg != msg_waiting_for_reply.end()) {
				SATAN_DEBUG("Matching outstanding request.\n");
				repmsg->second->reply_to(&msg);
				SATAN_DEBUG("Reply processed.\n");
				msg_waiting_for_reply.erase(repmsg);
			}
		}
		break;

		case __MSG_DELETE_OBJECT:
		{
			auto obj_iterator = all_objects.find(std::stol(msg.get_value("objid")));
			if(obj_iterator != all_objects.end()) {
				obj_iterator->second->on_delete(this);
				all_objects.erase(obj_iterator);
			}
		}
		break;

		case __MSG_FAILURE_RESPONSE:
		{
			failure_response_callback(msg.get_value("response"));
		}
		break;

		default:
		{
			auto obj_iterator = all_objects.find(identifier);
			if(obj_iterator == all_objects.end()) throw BaseObject::NoSuchObject();

			obj_iterator->second->process_message_client(this, this, msg);
		}
		break;
		}
	}

	void Client::on_connection_dropped() {
		// inform all waiting messages that their action failed
		for(auto msg : msg_waiting_for_reply) {
			msg.second->reply_to(NULL);
		}
		msg_waiting_for_reply.clear();

		flush_all_objects();
		disconnect_callback();

	}

	void Client::start_client(const std::string &server_host,
				  int server_port,
				  std::function<void()> disconnect_callback,
				  std::function<void(const std::string &fresp)> failure_response_callback) {
		std::lock_guard<std::mutex> lock_guard(client_mutex);

		try {
			client = std::shared_ptr<Client>(new Client(server_host, server_port, disconnect_callback, failure_response_callback));

			client->io_thread = std::thread([]() {
					SATAN_DEBUG(" *********  CLIENT THREAD STARTED **********\n");
					client->io_service.run();
				}
				);
		} catch (std::exception& e) {
			SATAN_ERROR("exception caught: %s\n", e.what());
		}
	}

	void Client::disconnect() {
		std::lock_guard<std::mutex> lock_guard(client_mutex);
		if(client) {
			client->io_service.post(
				[]()
				{
					try {
						client->my_socket.close();
					} catch(std::exception &exp) {
					}
					client->io_service.stop();
				});

			client->io_thread.join();
			client.reset();
		}
	}

	void Client::register_ri_machine_set_listener(
		std::weak_ptr<RIMachine::RIMachineSetListener> ri_mset_listener
		) {
		std::lock_guard<std::mutex> lock_guard(client_mutex);
		if(client) {
			client->io_service.post(
				[ri_mset_listener]()
				{
					RIMachine::register_ri_machine_set_listener(ri_mset_listener);
				});
		} else {
			// if disconnected, we can go ahead and directly register...
			RIMachine::register_ri_machine_set_listener(ri_mset_listener);
		}

	}

	void Client::distribute_message(std::shared_ptr<Message> &msg, bool via_udp) {
		SATAN_DEBUG("Client::distribute_message()...\n");
		if(msg->is_awaiting_reply()) {
			if(msg_waiting_for_reply.find(next_reply_id) != msg_waiting_for_reply.end()) {
				// no available reply id - reply with empty message to signal failure
				msg->reply_to(NULL);
			} else {
				// fill in object reply id header
				msg->set_value("repid", std::to_string(next_reply_id));
				// add the msg to our set of messages waiting for a reply
				msg_waiting_for_reply[next_reply_id++] = msg;
				// deliver it
				deliver_message(msg, via_udp);
			}
		} else {
			deliver_message(msg, via_udp);
		}
	}

	auto Client::get_object(int32_t objid) -> std::shared_ptr<BaseObject> {
		if(all_objects.find(objid) == all_objects.end()) throw BaseObject::NoSuchObject();

		return all_objects[objid];
	}

	);
