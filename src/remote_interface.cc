/*
 * VuKNOB
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

/*
 * Parts of this file - Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
 *
 * Original code was distributed under the Boost Software License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include <string>
#include <mutex>
#include <condition_variable>

#include "remote_interface.hh"
#include "dynamic_machine.hh"
#include "machine_sequencer.hh"
#include "common.hh"
#include "serialize.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class RemoteInterface::Session
 *
 ***************************/

void RemoteInterface::Session::initialize(asio::io_service& io_service, std::function<void()> on_completion) {
	const char *my_introduction = "HELLO";
	char peer_introduction[6];

	asio::write(socket, asio::buffer(my_introduction, 5));
	asio::read(socket, asio::buffer(peer_introduction, 5));

	io_service.post(on_completion);
}

/***************************
 *
 *  Class RemoteInterface::Message
 *
 ***************************/

RemoteInterface::Message::Message() : context(), ostrm(&sbuf), data2send(0) {}

RemoteInterface::Message::Message(RemoteInterface::Context *_context) : context(_context), ostrm(&sbuf), data2send(0) {}

void RemoteInterface::Message::set_reply_handler(std::function<void(const Message *reply_msg)> __reply_received_callback) {
	reply_received_callback = __reply_received_callback;
	awaiting_reply = true;
}

void RemoteInterface::Message::reply_to(const Message *reply_msg) {
	reply_received_callback(reply_msg);
}

bool RemoteInterface::Message::is_awaiting_reply() {
	return awaiting_reply;
}

void RemoteInterface::Message::set_value(const std::string &key, const std::string &value) {
	if(key.find(';') != std::string::npos) throw IllegalChar();
	if(key.find('=') != std::string::npos) throw IllegalChar();
	if(value.find(';') != std::string::npos) throw IllegalChar();
	if(value.find('=') != std::string::npos) throw IllegalChar();

	auto enc_key = Serialize::encode_string(key);
	auto enc_val = Serialize::encode_string(value);

	if(key2val.find(enc_key) != key2val.end())
		throw SettingSameKeyTwice(key.c_str());

	key2val[enc_key] = enc_val;

	data2send += enc_key.size() + enc_val.size() + 2; // +2 for the '=' and ';' that encode will add later

	encoded = false;
}

std::string RemoteInterface::Message::get_value(const std::string &key) const {
	auto result = key2val.find(key);
	if(result == key2val.end()) {
		for(auto k2v : key2val) {
			SATAN_DEBUG("[%s] does not match [%s] -> %s.\n", key.c_str(), k2v.first.c_str(), k2v.second.c_str());
		}
		throw NoSuchKey(key.c_str());
	}
	return result->second;
}

asio::streambuf::mutable_buffers_type RemoteInterface::Message::prepare_buffer(std::size_t length) {
	clear_msg_content();
	return sbuf.prepare(length);
}

void RemoteInterface::Message::commit_data(std::size_t length) {
	sbuf.commit(length);
}

asio::streambuf::const_buffers_type RemoteInterface::Message::get_data() {
	return sbuf.data();
}

void RemoteInterface::Message::recycle() {
	sbuf.consume(data2send);
	clear_msg_content();
}

bool RemoteInterface::Message::decode_header() {
	std::istream is(&sbuf);
	std::string header;
	is >> header;
	sscanf(header.c_str(), "%08x", &body_length);

	SATAN_DEBUG("decode_header()-> [%s]\n", header.c_str());

	return true;
}

bool RemoteInterface::Message::decode_body() {
	std::istream is(&sbuf);

	std::string key, val;

	std::getline(is, key, '=');
	SATAN_DEBUG("decode_body()\n");
	while(!is.eof() && key != "") {
		SATAN_DEBUG("   key: %s\n", key.c_str());
		std::getline(is, val, ';');
		SATAN_DEBUG("     -> val: %s\n", val.c_str());
		if(key != "") {
			key2val[Serialize::decode_string(key)] = Serialize::decode_string(val);
		}
		std::getline(is, key, '=');
	}

	return true;
}

void RemoteInterface::Message::encode() const {
	if(encoded) return;
	encoded = true;

	// add header
	char header[9];
	snprintf(header, 9, "%08x", data2send);
	ostrm << header;

	int checksum = 0;
	SATAN_DEBUG("encode() header -> [%s]\n", header);
	// add body
	for(auto k2v : key2val) {
		auto enc_key = k2v.first;
		auto enc_val = k2v.second;

		ostrm << enc_key << "=" << enc_val << ";";

		SATAN_DEBUG("   encode: [%s] -> [%s]\n", enc_key.c_str(), enc_val.c_str());
		checksum += 2 + enc_key.size() + enc_val.size();
	}
	SATAN_DEBUG("data2send[%d] =? checksum[%d] ?\n", data2send, checksum);

	data2send += header_length; // add header length to the total data size
}

void RemoteInterface::Message::clear_msg_content() {
	data2send = 0;
	awaiting_reply = false;
	reply_received_callback = [](const Message *reply_msg){};
	key2val.clear();
}

/***************************
 *
 *  Class RemoteInterface::Context
 *
 ***************************/

RemoteInterface::Context::~Context() {
	for(auto msg : available_messages)
		delete msg;
}

void RemoteInterface::Context::invalidate_object(
	std::shared_ptr<RemoteInterface::BaseObject> obj) {
	obj->__object_is_valid = false;
}

void RemoteInterface::Context::post_action(std::function<void()> f, bool do_synch) {
	auto ttid = std::this_thread::get_id();
	if(ttid == __context_thread_id) {
		// run directly - already on context thread
		f();
	} else if(do_synch) {
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<bool> ready(false);

		static int32_t sync_id = 0;
		int32_t this_sync_id = sync_id;

		SATAN_DEBUG("Context::post_action() doing sync for %d (%p)\n", this_sync_id, &io_service);
		io_service.dispatch(
			[&ready, &mtx, &cv, &f, this_sync_id]() {
				SATAN_DEBUG("Context::post_action() calling operation for %d\n", this_sync_id);
				f();
				SATAN_DEBUG("Context::post_action() operation for %d finished\n", this_sync_id);
				std::unique_lock<std::mutex> lck(mtx);
				ready = true;
				cv.notify_all();
				SATAN_DEBUG("Context::post_action() waiter %d notified\n", this_sync_id);
			}
			);

		SATAN_DEBUG("Context::post_action() waiting for lock %d\n", this_sync_id);
		std::unique_lock<std::mutex> lck(mtx);
		while (!ready) cv.wait(lck);
		SATAN_DEBUG("Context::post_action() woke up for lock %d\n", this_sync_id);
		sync_id++;
	} else {
		io_service.dispatch(f);
	}
}

std::shared_ptr<RemoteInterface::Message> RemoteInterface::Context::acquire_message() {
	Message *m_ptr = NULL;

	if(available_messages.size() == 0) {
		m_ptr = new Message(this);
	} else {
		m_ptr = available_messages.front();
		available_messages.pop_front();
	}

	return std::shared_ptr<Message>(
		m_ptr,
		[this](RemoteInterface::Message* msg) {
			msg->recycle();
			available_messages.push_back(msg);
		}
		);
}

std::shared_ptr<RemoteInterface::Message> RemoteInterface::Context::acquire_reply(const Message &originator) {
	std::shared_ptr<Message> reply = acquire_message();
	reply->set_value("id", std::to_string(__MSG_REPLY));
	reply->set_value("repid", originator.get_value("repid"));
	return reply;
}

/***************************
 *
 *  Class RemoteInterface::BasicMessageHandler
 *
 ***************************/

void RemoteInterface::BasicMessageHandler::do_read_header() {
	auto self(shared_from_this());
	asio::async_read(
		my_socket,
		read_msg.prepare_buffer(Message::header_length),
		[this, self](std::error_code ec, std::size_t length) {
			if(!ec) read_msg.commit_data(length);

			if(!ec && read_msg.decode_header()) {
				do_read_body();
			} else {
				on_connection_dropped();
			}
		}
		);
}

void RemoteInterface::BasicMessageHandler::do_read_body() {
	SATAN_DEBUG("::do_read_body() - body length: %d\n", read_msg.get_body_length());
	auto self(shared_from_this());
	asio::async_read(
		my_socket,
		read_msg.prepare_buffer(read_msg.get_body_length()),
		[this, self](std::error_code ec, std::size_t length) {
			SATAN_DEBUG("::do_read_body()->async(%d)\n", length);
			if(!ec) read_msg.commit_data(length);

			if(!ec && read_msg.decode_body()) {

				try {
					SATAN_DEBUG("MessageHandler::do_read_body() - message received..\n");
					on_message_received(read_msg);
				} catch (Message::NoSuchKey& e) {
					SATAN_ERROR("do_read_body() caught an NoSuchKey exception: %s\n",
						    e.keyname);
					on_connection_dropped();
				} catch (std::exception& e) {
					SATAN_ERROR("do_read_body() caught an exception: %s\n", e.what());
					on_connection_dropped();
				}

				do_read_header();
			} else {
				on_connection_dropped();
			}
		}
		);
}

void RemoteInterface::BasicMessageHandler::do_write() {
	SATAN_DEBUG("MessageHandler::do_write()... queing async write..\n");
	auto self(shared_from_this());
	asio::async_write(
		my_socket,
		write_msgs.front()->get_data(),
		[this, self](std::error_code ec, std::size_t length) {
			SATAN_DEBUG("MessageHandler::do_write()... async write completed!\n");
			if (!ec) {
				write_msgs.pop_front();
				if (!write_msgs.empty()) {
					do_write();
				}
			} else {
				on_connection_dropped();
			}
		}
		);
	SATAN_DEBUG("MessageHandler::do_write()... async write queued..\n");
}

RemoteInterface::BasicMessageHandler::BasicMessageHandler(asio::io_service &io_service) : my_socket(io_service) {
}

RemoteInterface::BasicMessageHandler::BasicMessageHandler(asio::ip::tcp::socket _socket) : my_socket(std::move(_socket)) {
}

void RemoteInterface::BasicMessageHandler::start_receive() {
	do_read_header();
}

void RemoteInterface::BasicMessageHandler::deliver_message(std::shared_ptr<Message> &msg) {
	msg->encode();

	SATAN_DEBUG("RemoteInterface::BasicMessageHandler::deliver_message() - msg encoded..\n");

	bool write_in_progress = !write_msgs.empty();
	write_msgs.push_back(msg);
	if (!write_in_progress){
		do_write();
	}
}

/***************************
 *
 *  Class RemoteInterface::BaseObject::Factory
 *
 ***************************/

std::mutex RemoteInterface::BaseObject::Factory::__factory_registration_mutex;

RemoteInterface::BaseObject::Factory::Factory(const char* _type, bool _static_single_object)
	: type(_type)
	, static_single_object(_static_single_object)
	, what_sides(ServerSide | ClientSide)
{}

RemoteInterface::BaseObject::Factory::Factory(WhatSide _what_sides, const char* _type, bool _static_single_object)
	: type(_type)
	, static_single_object(_static_single_object)
	, what_sides(_what_sides)
{}

RemoteInterface::BaseObject::Factory::~Factory() {
	if(what_sides & ServerSide) {
		auto factory_iterator = server_factories_by_name.find(type);
		if(factory_iterator != server_factories_by_name.end()) {
			server_factories_by_name.erase(factory_iterator);
		}
	}
	if(what_sides & ClientSide) {
		auto factory_iterator = client_factories_by_name.find(type);
		if(factory_iterator != client_factories_by_name.end()) {
			client_factories_by_name.erase(factory_iterator);
		}
	}
}

const char* RemoteInterface::BaseObject::Factory::get_type() const {
	return type;
}

auto RemoteInterface::BaseObject::Factory::create_static_single_object(
	Context* context,
	int32_t new_obj_id) -> std::shared_ptr<BaseObject> {
	std::shared_ptr<BaseObject> retval;

	if(static_single_object)
		retval = create_and_register(context, new_obj_id);

	return retval;
}

/***************************
 *
 *  Class RemoteInterface::BaseObject
 *
 ***************************/

RemoteInterface::BaseObject::BaseObject(const Factory *_factory, const Message &serialized) : __is_server_side(false), my_factory(_factory) {
	// this constructor should only be used client side
	obj_id = std::stol(serialized.get_value("new_objid"));
}

RemoteInterface::BaseObject::BaseObject(int32_t new_obj_id, const Factory *_factory) : __is_server_side(true), obj_id(new_obj_id), my_factory(_factory) {
	// this constructor should only be used server side
}

void RemoteInterface::BaseObject::request_delete_me() {
	if(!check_object_is_valid()) throw ObjectWasDeleted();

	context->post_action(
		[this]() {
			context->on_remove_object(obj_id);
		}
		);
}

void RemoteInterface::BaseObject::send_object_message(std::function<void(std::shared_ptr<Message> &msg_to_send)> complete_message) {
	SATAN_DEBUG("::send_object_message() - A\n");
	if(!check_object_is_valid()) throw ObjectWasDeleted();

	SATAN_DEBUG("::send_object_message() - B\n");
	context->post_action(
		[this, complete_message]() {
			SATAN_DEBUG("::send_object_message() - C1\n");
			std::shared_ptr<Message> msg2send = context->acquire_message();

			{
				// fill in object id header
				msg2send->set_value("id", std::to_string(obj_id));
			}
			SATAN_DEBUG("::send_object_message() - C2\n");
			{
				// call complete message
				complete_message(msg2send);
			}
			SATAN_DEBUG("::send_object_message() - C3\n");

			try {
				context->distribute_message(msg2send);
			} catch(std::exception& e) {
				SATAN_ERROR("RemoteInterface::BaseObject::send_object_message() caught an exception: %s\n", e.what());
				throw;
			} catch(...) {
				SATAN_ERROR("RemoteInterface::BaseObject::send_object_message() caught an unknown exception.\n");
				throw;
			}
		}, true
		);
	SATAN_DEBUG("::send_object_message() - D\n");
}

void RemoteInterface::BaseObject::send_object_message(std::function<void(std::shared_ptr<Message> &msg_to_send)> complete_message,
						      std::function<void(const Message *reply_msg)> reply_received_callback) {
	SATAN_DEBUG("::send_object_message(v2) - A\n");
	std::mutex mtx;
	std::condition_variable cv;
	bool ready = false;

	SATAN_DEBUG("::send_object_message(v2) - B\n");
	if(!check_object_is_valid()) throw ObjectWasDeleted();
	SATAN_DEBUG("::send_object_message(v2) - C\n");

	context->post_action(
		[this, complete_message, reply_received_callback, &ready, &mtx, &cv]() {
			SATAN_DEBUG("::send_object_message(v2) - 1\n");
			std::shared_ptr<Message> msg2send = context->acquire_message();
			SATAN_DEBUG("::send_object_message(v2) - 2\n");

			{
				// fill in object id header
			SATAN_DEBUG("::send_object_message(v2) - 3\n");
				msg2send->set_value("id", std::to_string(obj_id));
			SATAN_DEBUG("::send_object_message(v2) - 4\n");
				msg2send->set_reply_handler(
					[reply_received_callback, &ready, &mtx, &cv](const Message *reply_msg) {
						try {
							reply_received_callback(reply_msg);
						} catch(const std::exception &e) {
							SATAN_ERROR("RemoteInterface::BaseObject::send_object_message() - "
								    "failed to process server reply: %s\n",
								    e.what());
							throw;
						} catch(...) {
							SATAN_ERROR("RemoteInterface::BaseObject::send_object_message() - "
								    "failed to process server reply, unknown exception.\n");
							throw;
						}

						std::unique_lock<std::mutex> lck(mtx);
						ready = true;
						cv.notify_all();
					}
					);
			SATAN_DEBUG("::send_object_message(v2) - 5\n");
			}
			{
				// call complete message
			SATAN_DEBUG("::send_object_message(v2) - 6\n");
				complete_message(msg2send);
			SATAN_DEBUG("::send_object_message(v2) - 7\n");
			}

			SATAN_DEBUG("::send_object_message(v2) - 8\n");
			context->distribute_message(msg2send);
			SATAN_DEBUG("::send_object_message(v2) - 9\n");
		}
		);
	SATAN_DEBUG("::send_object_message(v2) - D\n");

	std::unique_lock<std::mutex> lck(mtx);
	while (!ready) cv.wait(lck);
	SATAN_DEBUG("::send_object_message(v2) - E\n");
}

int32_t RemoteInterface::BaseObject::get_obj_id() {
	return obj_id;
}

auto RemoteInterface::BaseObject::get_type() -> ObjectType {
	return {my_factory->get_type()};
}

void RemoteInterface::BaseObject::set_context(Context* _context) {
	context = _context;
}

std::shared_ptr<RemoteInterface::BaseObject> RemoteInterface::BaseObject::create_object_on_client(
	Context* _ctxt,
	const Message &msg)
{
	std::string factory_type = msg.get_value("factory");
	auto factory_iterator = client_factories_by_name.find(factory_type);
	if(factory_iterator == client_factories_by_name.end()) {
		throw NoSuchFactory();
	}

	auto o = factory_iterator->second->create_and_register(_ctxt, msg);

	o->post_constructor_client();

	return o;
}

void RemoteInterface::BaseObject::create_static_single_objects_on_server(
	Context* context,
	std::function<int()> get_new_id_callback,
	std::function<void(std::shared_ptr<BaseObject>)> new_obj_created) {

	for(auto factory : server_factories_by_name) {
		auto obj = factory.second->create_static_single_object(context, get_new_id_callback());
		if(obj) new_obj_created(obj);
	}
}

std::map<std::string, RemoteInterface::BaseObject::Factory *>
RemoteInterface::BaseObject::server_factories_by_name;
std::map<std::string, RemoteInterface::BaseObject::Factory *>
RemoteInterface::BaseObject::client_factories_by_name;
std::map<std::type_index, RemoteInterface::BaseObject::Factory *>
RemoteInterface::BaseObject::server_factories_by_type_index;
std::map<std::type_index, RemoteInterface::BaseObject::Factory *>
RemoteInterface::BaseObject::client_factories_by_type_index;

/***************************
 *
 *  Class RemoteInterface::SimpleBaseObject
 *
 ***************************/

namespace RemoteInterface {

	SimpleBaseObject::SimpleBaseObject(const Factory *factory, const Message &serialized)
		: BaseObject(factory, serialized)
	{
	}

	SimpleBaseObject::SimpleBaseObject(int32_t new_obj_id, const Factory* factory)
		: BaseObject(new_obj_id, factory)
	{
	}

	void SimpleBaseObject::register_handler(const std::string& command_id,
						std::function<void(Context *context, MessageHandler *src, const Message& msg)> handler) {
		if(command2function.find(command_id) != command2function.end())
			throw HandlerAlreadyRegistered();

		command2function[command_id] = handler;
	}

	class ServerSideOnlyMessageHandler : public MessageHandler{
	private:
		std::function<void(const Message *reply_message)> callback;
	public:
		ServerSideOnlyMessageHandler(std::function<void(const Message *reply_message)> _cb)
			: callback(_cb)
			{}
		ServerSideOnlyMessageHandler()
			: callback([](const Message *reply_message) {
					   SATAN_ERROR("Calling ServerSideOnlyMessageHandler::deliver_message() without a defined callback.");
				   })
			{}

		virtual void deliver_message(std::shared_ptr<Message> &msg) override {
			callback(msg.get());
		}

		virtual void on_message_received(const Message &msg) override {}
		virtual void on_connection_dropped() override {}
	};

	void SimpleBaseObject::send_message(
		const std::string &command_id,
		std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback,
		std::function<void(const Message *reply_message)> reply_received_callback) {

		send_object_message(
			[command_id, create_msg_callback](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("commandid", command_id);
				create_msg_callback(msg_to_send);
			},
			reply_received_callback);
	}

	void SimpleBaseObject::send_message_to_server(
		const std::string &command_id,
		std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback,
		std::function<void(const Message *reply_message)> reply_received_callback) {

		SATAN_DEBUG("::send_message_to_server() - A\n");
		if(!check_object_is_valid()) throw ObjectWasDeleted();
		SATAN_DEBUG("::send_message_to_server() - B\n");
		if(is_server_side()) {
			SATAN_DEBUG("::send_message_to_server() - C\n");
			// if we already are server side - shortcut the process
			context->post_action(
				[this, command_id, create_msg_callback, reply_received_callback]() {
					SATAN_DEBUG("::send_message_to_server() - D\n");
					std::shared_ptr<Message> msg2send = context->acquire_message();
					SATAN_DEBUG("::send_message_to_server() - E\n");

					msg2send->set_value("id", std::to_string(get_obj_id()));
					msg2send->set_value("commandid", command_id);
					create_msg_callback(msg2send);
					SATAN_DEBUG("::send_message_to_server() - F\n");

					ServerSideOnlyMessageHandler sh(reply_received_callback);
					auto fnc = command2function.find(command_id);
					const Message& m2s = *(msg2send.get());
					if(fnc != command2function.end())
						fnc->second(context, &sh, m2s);
				}, true
				);
		} else {
			SATAN_DEBUG("::send_message_to_server() - C1\n");
			send_object_message(
				[command_id, create_msg_callback](std::shared_ptr<Message> &msg_to_send) {
					SATAN_DEBUG("::send_message_to_server() - C1A\n");
					msg_to_send->set_value("commandid", command_id);
					create_msg_callback(msg_to_send);
					SATAN_DEBUG("::send_message_to_server() - C1B\n");
				},
				reply_received_callback);
			SATAN_DEBUG("::send_message_to_server() - C3\n");
		}
	}

	void SimpleBaseObject::send_message(
		const std::string &command_id,
		std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback) {

		send_object_message(
			[command_id, create_msg_callback](std::shared_ptr<Message> &msg_to_send) {
				msg_to_send->set_value("commandid", command_id);
				create_msg_callback(msg_to_send);
			}
			);
	}

	void SimpleBaseObject::send_message_to_server(
		const std::string &command_id,
		std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback) {

		if(!check_object_is_valid()) throw ObjectWasDeleted();

		if(is_server_side()) {
			SATAN_DEBUG("::send_message_to_server() - already on server...\n");
			// if we already are server side - shortcut the process
			context->post_action(
				[this, command_id, create_msg_callback]() {
					std::shared_ptr<Message> msg2send = context->acquire_message();

					msg2send->set_value("id", std::to_string(get_obj_id()));
					msg2send->set_value("commandid", command_id);
					SATAN_DEBUG("::send_message_to_server(): Calling create_msg_callback()...\n");
					create_msg_callback(msg2send);

					SATAN_DEBUG("::send_message_to_server(): Creating handler...\n");
					ServerSideOnlyMessageHandler sh;
					SATAN_DEBUG("::send_message_to_server(): Finding function...\n");
					auto fnc = command2function.find(command_id);
					const Message& m2s = *(msg2send.get());
					if(fnc != command2function.end()) {
						SATAN_DEBUG("::send_message_to_server(): Calling function...\n");
						fnc->second(context, &sh, m2s);
					}
				}, true
				);
		} else {
			SATAN_DEBUG("::send_message_to_server() - not on server...\n");
			send_object_message(
				[command_id, create_msg_callback](std::shared_ptr<Message> &msg_to_send) {
					msg_to_send->set_value("commandid", command_id);
					create_msg_callback(msg_to_send);
				}
				);
		}
	}

	void SimpleBaseObject::post_constructor_client() {
	}

	void SimpleBaseObject::process_message_server(Context* context,
						      MessageHandler *src,
						      const Message &msg) {
		std::string command_id = msg.get_value("commandid");
		auto fnc = command2function.find(command_id);
		if(fnc != command2function.end())
			fnc->second(context, src, msg);
	}

	void SimpleBaseObject::process_message_client(Context* context,
						      MessageHandler *src,
						      const Message &msg) {
		std::string command_id = msg.get_value("commandid");
		auto fnc = command2function.find(command_id);
		if(fnc != command2function.end())
			fnc->second(context, src, msg);
	}

	void SimpleBaseObject::serialize(std::shared_ptr<Message> &target) {
	}
};
