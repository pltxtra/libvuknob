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

#include <cxxabi.h>

#include "server.hh"
#include "base_machine.hh"
#include "../vuknob/server.h"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

SERVER_CODE(

	Server::ClientAgent::ClientAgent(Session *session, Server *_server)
	: BasicMessageHandler(std::move(session->get_socket()))
	, server(_server) {
	}

	Server::ClientAgent::~ClientAgent() {
	}

	void Server::ClientAgent::start() {
		start_receive();
	}

	void Server::ClientAgent::disconnect() {
		my_socket.close();
	}

	void Server::ClientAgent::on_message_received(const Message &msg) {
		std::string resp_msg;

		try {
			server->route_incomming_message(this, msg);
		} catch(Context::FailureResponse &fresp) {
			resp_msg = fresp.response_message;
		} catch(std::exception& e) {
			SATAN_ERROR("Server::ClientAgent::on_message_received() failed: %s\n", e.what());
			throw;
		}

		if(resp_msg.size() != 0) {
			std::shared_ptr<Message> response = server->acquire_message();
			response->set_value("id", std::to_string(__MSG_FAILURE_RESPONSE));
			response->set_value("response", resp_msg);
			deliver_message(response);
		}
	}

	void Server::ClientAgent::on_connection_dropped() {
		server->drop_client(std::static_pointer_cast<ClientAgent>(shared_from_this()));
	}

/***************************
 *
 *  Class Server
 *
 ***************************/

	std::shared_ptr<Server> Server::server;
	std::mutex Server::server_mutex;

	int32_t Server::reserve_new_obj_id() {
		int32_t new_obj_id = ++last_obj_id;

		if(new_obj_id < 0) throw BaseObject::ObjIdOverflow();

		return new_obj_id;
	}

	void Server::delete_object(std::shared_ptr<BaseObject> obj2delete) {
		for(auto obj  = all_objects.begin();
		    obj != all_objects.end();
		    obj++) {
			if((*obj).second == obj2delete) {

				std::shared_ptr<Message> destroy_object_message = acquire_message();
				add_destroy_object_header(destroy_object_message, obj2delete);
				distribute_message(destroy_object_message);
				invalidate_object(obj2delete); /* unlink from this context */
				all_objects.erase(obj);
				return;
			}
		}
	}

	void Server::project_loaded() {
	}

	void Server::machine_registered(std::shared_ptr<Machine> m_ptr) {
		auto seqm = std::dynamic_pointer_cast<Sequence>(m_ptr);
		if(seqm) {
			return; /* don't need any further action - we already know about this type of object */
		}

		io_service.post(

			[this, m_ptr]()
			{
				SATAN_DEBUG("RemoteInterface::ServerSpace::Server::machine_registered(%s) was registered\n", m_ptr->get_name().c_str());
				std::string resp_msg;

				try {
					auto new_machine_api = create_object_from_factory<RemoteInterface::ServerSpace::MachineAPI>(
						[this, m_ptr]
						(std::shared_ptr<RemoteInterface::ServerSpace::MachineAPI> mchapi) {
							SATAN_DEBUG("Serverside will initiate MachineAPI object from machine ptr...\n");
							mchapi->init_from_machine_ptr(mchapi, m_ptr);
							SATAN_DEBUG("Serverside MachineAPI object created\n");
						}
						);

					Sequence::create_sequence_for_machine(new_machine_api);
				} catch(jException &e) {
					SATAN_ERROR("Server::machine_registered() - "
						    "caught an jException exception: %s\n", e.message.c_str());
				} catch(...) {
					int status = 0;
					char * buff = __cxxabiv1::__cxa_demangle(
						__cxxabiv1::__cxa_current_exception_type()->name(),
						NULL, NULL, &status);
					SATAN_ERROR("Server::machine_registered() - "
						    "caught an unknown exception: %s\n", buff);
					SATAN_ERROR("Server::machine_registered() - "
						    "failed to create RIMachine or sequencer for %s\n",
						    m_ptr->get_name().c_str());
					Machine::disconnect_and_destroy(m_ptr.get());
					resp_msg =
						"Internal server error - "
						"failed to create RIMachine or Sequence for new Machine."
						"Please restart session.";
				}

				if(resp_msg != "") {
					std::shared_ptr<Message> response = server->acquire_message();
					response->set_value("id", std::to_string(__MSG_FAILURE_RESPONSE));
					response->set_value("response", resp_msg);
					distribute_message(response);
				}
			}

			);
	}

	void Server::machine_unregistered(std::shared_ptr<Machine> m_ptr) {
		io_service.post(

			[this, m_ptr]()
			{
				BaseMachine::machine_unregistered(m_ptr);
			}

			);
	}

	void Server::machine_input_attached(std::shared_ptr<Machine> source,
					    std::shared_ptr<Machine> destination,
					    const std::string &output_name,
					    const std::string &input_name) {
		io_service.post(
			[this, source, destination, output_name, input_name]()
			{
				BaseMachine::machine_input_attached(source, destination, output_name, input_name);
				SATAN_DEBUG("Server::machine_inpute_attached() [%s:%s] -> [%s:%s]\n",
					    source->get_name().c_str(), output_name.c_str(),
					    destination->get_name().c_str(), input_name.c_str());
			}

			);
	}

	void Server::machine_input_detached(std::shared_ptr<Machine> source,
					    std::shared_ptr<Machine> destination,
					    const std::string &output_name,
					    const std::string &input_name) {
		io_service.post(
			[this, source, destination, output_name, input_name]()
			{
				BaseMachine::machine_input_detached(source, destination, output_name, input_name);
				SATAN_DEBUG("!!! Signal detached [%s:%s] -> [%s:%s]\n",
					    source->get_name().c_str(), output_name.c_str(),
					    destination->get_name().c_str(), input_name.c_str());
			}

			);
	}

	Server::Server(const asio::ip::tcp::endpoint& endpoint)
	: last_obj_id(-1)
	, acceptor(io_service, endpoint)
	{
		acceptor.listen();

		current_port = acceptor.local_endpoint().port();

		SATAN_DEBUG("Server::Server() current ip: %s, port: %d\n",
			    acceptor.local_endpoint().address().to_string().c_str(),
			    current_port);

		io_service.post(
			[this]()
			{
				try {
					this->create_service_objects();
				} catch(std::exception &exp) {
					throw;
				}
			});

		do_accept();
	}

	int Server::get_port() {
		return current_port;
	}

	void Server::do_accept() {
		auto new_session = new Session(io_service);
		acceptor.async_accept(
			new_session->get_socket(),
			[this, new_session](std::error_code ec) {
				if (!ec) {
					new_session->initialize(
						io_service,
						[this, new_session]() {
							std::shared_ptr<ClientAgent> new_client_agent =
								std::make_shared<ClientAgent>(new_session, this);

							client_agents.insert(new_client_agent);

							send_protocol_version_to_new_client(new_client_agent);
							send_all_objects_to_new_client(new_client_agent);

							new_client_agent->start();
						}
						);
				}
				do_accept();
			}
			);
	}

	void Server::drop_client(std::shared_ptr<ClientAgent> client_agent) {
		auto client_iterator = client_agents.find(client_agent);
		if(client_iterator != client_agents.end()) {
			client_agents.erase(client_iterator);
		}
	}

	void Server::add_create_object_header(std::shared_ptr<Message> &target, std::shared_ptr<BaseObject> obj) {
		SATAN_DEBUG("Server::add_create_object_header() -- id: %d\n", __MSG_CREATE_OBJECT);
		target->set_value("id", std::to_string(__MSG_CREATE_OBJECT));
		target->set_value("new_objid", std::to_string(obj->get_obj_id()));
		target->set_value("factory", obj->get_type().type_name);
	}

	void Server::add_destroy_object_header(std::shared_ptr<Message> &target, std::shared_ptr<BaseObject> obj) {
		target->set_value("id", std::to_string(__MSG_DELETE_OBJECT));
		target->set_value("objid", std::to_string(obj->get_obj_id()));
	}

	void Server::send_protocol_version_to_new_client(std::shared_ptr<MessageHandler> client_agent) {
		std::shared_ptr<Message> pv_message = acquire_message();
		pv_message->set_value("id", std::to_string(__MSG_PROTOCOL_VERSION));
		pv_message->set_value("pversion", std::to_string(__VUKNOB_PROTOCOL_VERSION__));
		client_agent->deliver_message(pv_message);
	}

	void Server::send_all_objects_to_new_client(std::shared_ptr<MessageHandler> client_agent) {
		for(auto obj : all_objects) {
			std::shared_ptr<Message> create_object_message = acquire_message();
			add_create_object_header(create_object_message, obj.second);
			obj.second->serialize(create_object_message);
			client_agent->deliver_message(create_object_message);
		}
	}

	void Server::route_incomming_message(ClientAgent *src, const Message &msg) {
		int identifier = std::stol(msg.get_value("id"));

		if(identifier == __MSG_DELETE_OBJECT) {
			int identifier = std::stol(msg.get_value("objid"));

			auto obj_iterator = all_objects.find(identifier);
			if(obj_iterator == all_objects.end()) throw BaseObject::NoSuchObject();

			delete_object(obj_iterator->second);
		} else {
			auto obj_iterator = all_objects.find(identifier);
			if(obj_iterator == all_objects.end()) throw BaseObject::NoSuchObject();

			obj_iterator->second->process_message_server(this, src, msg);
		}
	}

	void Server::disconnect_clients() {
		for(auto client_agent : client_agents) {
			client_agent->disconnect();
		}
	}

	void Server::create_service_objects() {
		{ // create handle list object
			create_object_from_factory<RemoteInterface::ServerSpace::HandleList>(
				[](std::shared_ptr<RemoteInterface::ServerSpace::HandleList> new_obj){}
				);
		}
		{ // create global control object
			create_object_from_factory<RemoteInterface::ServerSpace::GlobalControlObject>(
				[](std::shared_ptr<RemoteInterface::ServerSpace::GlobalControlObject> new_obj){}
				);
		}
		{
			BaseObject::create_static_single_objects_on_server(
				this,
				[this](void) -> int {
					return reserve_new_obj_id();
				},
				[this](std::shared_ptr<BaseObject> new_obj) {
					remember_object<BaseObject>(
						new_obj,
						[](std::shared_ptr<BaseObject> /*nuobj*/)
						{/* empty callback */}
						);
				}
				);
		}
		{ // register us as a machine set listener
			Machine::register_machine_set_listener(shared_from_this());
		}
	}

	void Server::on_remove_object(int32_t objid) {
		io_service.post(
			[this, objid]() {
				auto obj_iterator = all_objects.find(objid);
				if(obj_iterator == all_objects.end()) throw BaseObject::NoSuchObject();

				delete_object(obj_iterator->second);
			}
			);
	}

	int Server::start_server() {
		std::lock_guard<std::mutex> lock_guard(server_mutex);

		if(server) {
			return server->get_port();
		}

		int portval = -1;

		try {
			asio::ip::tcp::endpoint endpoint;//(asio::ip::tcp::v4(), 0); // 0 => select a random available port
			server = std::shared_ptr<Server>(new Server(endpoint));

			server->io_thread = std::thread([]() {
					SATAN_DEBUG(" *********  SERVER THREAD STARTED (%p) **********\n",
						    &(server->io_service));
					try {
						server->run_context();
					} catch(std::exception const& e) {
						SATAN_ERROR("RemoteInterface::Server::start_server() - std::exception caught %s\n", e.what());
						abort();
					} catch(jException const& e) {
						SATAN_ERROR("RemoteInterface::Server::start_server() - jException caught %s\n", e.message.c_str());
						abort();
					}
				}
				);

			portval = server->get_port();
		} catch (std::exception& e) {
			SATAN_ERROR("Exception caught: %s\n", e.what());
		}

		return portval;
	}

	bool Server::is_running() {
		std::lock_guard<std::mutex> lock_guard(server_mutex);

		if(server) {
			return true;
		}
		return false;
	}

	void Server::stop_server() {
		std::lock_guard<std::mutex> lock_guard(server_mutex);

		if(server) {
			server->io_service.post(
				[]()
				{
					try {
						SATAN_DEBUG("will disconnect clients...\n");
						server->disconnect_clients();
						SATAN_DEBUG("clients disconnected!");
					} catch(std::exception &exp) {
						throw;
					}
					SATAN_DEBUG("stop IO service...!");
					server->io_service.stop();
				});

			SATAN_DEBUG("waiting for thread...!");
			server->io_thread.join();
			SATAN_DEBUG("reseting server object...!");
			server.reset();
			SATAN_DEBUG("done!");
		}
	}

	void Server::distribute_message(std::shared_ptr<Message> &msg) {
		for(auto client_agent : client_agents) {
			SATAN_DEBUG("Server::distribute_message() - deliver_message() called.\n");
			client_agent->deliver_message(msg);
		}
	}

	auto Server::get_object(int32_t objid) -> std::shared_ptr<BaseObject> {
		if(all_objects.find(objid) == all_objects.end()) throw BaseObject::NoSuchObject();

		return all_objects[objid];
	}

);

extern "C" {
	int __RI_server_is_running() {
		return RemoteInterface::ServerSpace::Server::is_running() ? 1 : 0;
	}

	int __RI_start_server() {
		return RemoteInterface::ServerSpace::Server::start_server();
	}

	void __RI_stop_server() {
		return RemoteInterface::ServerSpace::Server::stop_server();
	}

	int vuknob_bootstrap_server() {
		Machine::prepare_baseline();
		SatanProjectEntry::clear_satan_project();
		return __RI_start_server();
	}
};
