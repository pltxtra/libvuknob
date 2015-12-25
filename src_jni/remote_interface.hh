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

#ifndef VUKNOB_REMOTE_INTERFACE
#define VUKNOB_REMOTE_INTERFACE

#include "machine.hh"

#include <string>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <asio.hpp>
#include <thread>
#include <utility>
#include <atomic>
#include <unordered_map>

#include "common.hh"

#define RI_LOOP_NOT_SET -1

#ifdef __RI__SERVER_SIDE
#define __RI__CURRENT_NAMESPACE ServerSpace
#define SERVER_SIDE_HANDLER(_name,_key)					\
	static constexpr const char* _name = _key;			\
	void handle_##_name(RemoteInterface::Context* context,		\
			    RemoteInterface::MessageHandler *src,	\
			    const RemoteInterface::Message& msg)
#define CLIENT_SIDE_HANDLER(_name,_key)					\
	static constexpr const char* _name = _key;
#define SERVER_REG_HANDLER(_class,_name)				\
	register_handler(_name,						\
			 std::bind(&_class::handle_##_name, this,	\
				   std::placeholders::_1,		\
				   std::placeholders::_2,		\
				   std::placeholders::_3))
#define CLIENT_REG_HANDLER(_class,_name)

#define SERVER_CODE(...)			\
	namespace RemoteInterface {		\
	namespace ServerSpace {			\
		__VA_ARGS__			\
	}; }
#define CLIENT_CODE(...)
#define SERVER_N_CLIENT_CODE(...)		\
	namespace RemoteInterface {		\
	namespace ServerSpace {			\
		__VA_ARGS__			\
	}; }

#define SERVER_CODE_START			\
	namespace RemoteInterface {		\
	namespace ServerSpace {
#define SERVER_CODE_END				\
	}; }

#define ON_SERVER(...) __VA_ARGS__
#define ON_CLIENT(...)

#endif

#ifdef __RI__CLIENT_SIDE
#define __RI__CURRENT_NAMESPACE ClientSpace
#define SERVER_SIDE_HANDLER(_name,_key)					\
	static constexpr const char* _name = _key;
#define CLIENT_SIDE_HANDLER(_name,_key)					\
	static constexpr const char* _name = _key;			\
	void handle_##_name(RemoteInterface::Context* context,		\
			    RemoteInterface::MessageHandler *src,	\
			    const RemoteInterface::Message& msg)
#define SERVER_REG_HANDLER(_class,_name)
#define CLIENT_REG_HANDLER(_class,_name)				\
	register_handler(_name,						\
			 std::bind(&_class::handle_##_name, this,	\
				   std::placeholders::_1,		\
				   std::placeholders::_2,		\
				   std::placeholders::_3))
#define SERVER_CODE(...)
#define CLIENT_CODE(...) \
	namespace RemoteInterface {		\
	namespace ClientSpace {			\
		__VA_ARGS__			\
	}; }
#define SERVER_N_CLIENT_CODE(...) \
	namespace RemoteInterface {		\
	namespace ClientSpace {			\
		__VA_ARGS__			\
	}; }

#define ON_SERVER(...)
#define ON_CLIENT(...) __VA_ARGS__

#endif

/***************************
 *
 *  RemoteInterface definitions
 *
 ***************************/

#define VUKNOB_MAX_UDP_SIZE 512

#define __MSG_CREATE_OBJECT -1
#define __MSG_FLUSH_ALL_OBJECTS -2
#define __MSG_DELETE_OBJECT -3
#define __MSG_FAILURE_RESPONSE -4
#define __MSG_PROTOCOL_VERSION -5
#define __MSG_REPLY -6
#define __MSG_CLIENT_ID -7

// Factory names
#define __FCT_HANDLELIST		"HandleList"
#define __FCT_GLOBALCONTROLOBJECT	"GloCtrlObj"
#define __FCT_RIMACHINE			"RIMachine"
#define __FCT_SAMPLEBANK	        "SampleBank"

#define __VUKNOB_PROTOCOL_VERSION__ 9

//#define VUKNOB_UDP_SUPPORT
//#define VUKNOB_UDP_USE

namespace RemoteInterface {
	class MessageHandler;
	class Context;
	class BaseObject;

	enum WhatSide {
		ServerSide = 0x0010,
		ClientSide = 0x0100
	};

	static inline std::string encode_byte_array(size_t len, const char *data) {
		std::string result;
		size_t offset = 0;
		while(len - offset > 0) {
			char frb[3];

			frb[0] = ((data[offset] & 0xf0) >> 4) + 'a';
			frb[1] = ((data[offset] & 0x0f) >> 0) + 'a';
			frb[2] = '\0';

			offset++;

			result +=  frb;
		}
		return result;
	}

	static inline void decode_byte_array(const std::string &encoded,
					     size_t &len, char **data) {
		*data = (char*)malloc((sizeof(char) * encoded.size()) >> 1);
		if(*data == 0) return;

		len = encoded.size() / 2;
		size_t offset = 0;

		while(len - offset > 0) {
			(*data)[offset] =
				( (encoded[(offset << 1) + 0] - 'a') << 4)
				|
				( (encoded[(offset << 1) + 1] - 'a') << 0);
			offset++;
		}
	}

	class Message {
	public:
		enum { header_length = 8 };

		class NoSuchKey : public std::runtime_error {
		public:
			const char *keyname;

			NoSuchKey(const char *_keyname) : runtime_error("No such key in message."), keyname(_keyname) {}
			virtual ~NoSuchKey() {}
		};

		class IllegalChar : public std::runtime_error {
		public:
			IllegalChar() : runtime_error("Key or value contains illegal character.") {}
			virtual ~IllegalChar() {}
		};

		class MessageTooLarge : public std::runtime_error {
		public:
			MessageTooLarge() : runtime_error("Encoded message too large.") {}
			virtual ~MessageTooLarge() {}
		};

		class CannotReceiveReply : public std::runtime_error {
		public:
			CannotReceiveReply() : runtime_error("Server reply interrupted.") {}
			virtual ~CannotReceiveReply() {}
		};

		Message();
		Message(Context* context);

		void set_reply_handler(std::function<void(const Message *reply_msg)> reply_received_callback);
		void reply_to(const Message *reply_msg);
		bool is_awaiting_reply();

		void set_value(const std::string &key, const std::string &value);
		std::string get_value(const std::string &key) const;

		asio::streambuf::mutable_buffers_type prepare_buffer(std::size_t length);
		void commit_data(std::size_t length);
		asio::streambuf::const_buffers_type get_data();

	private:
		Context* context;

		mutable bool encoded;
		mutable uint32_t body_length;
		mutable int32_t client_id;
		mutable asio::streambuf sbuf;
		mutable std::ostream ostrm;
		mutable int data2send;

		std::function<void(const Message *reply_msg)> reply_received_callback;
		bool awaiting_reply = false;

		std::map<std::string, std::string> key2val;

		void clear_msg_content();

	public:
		void recycle();

		inline uint32_t get_body_length() { return body_length; }
		inline int32_t get_client_id() { return client_id; }

		bool decode_client_id(); // only for messages arriving via UDP
		bool decode_header();
		bool decode_body();

		void encode() const;

	};

	class Context {
	public:
		template<class T>
		class ObjectSetListener {
		public:
			virtual void object_registered(std::shared_ptr<T> obj) = 0;
			virtual void object_unregistered(std::shared_ptr<T> obj) = 0;
		};

	private:
		std::deque<Message *> available_messages;

		typedef std::function<void(std::shared_ptr<BaseObject>)> ListenerCB;
		std::unordered_multimap<int, ListenerCB> register_obj_cbs;
		std::unordered_multimap<int, ListenerCB> unregister_obj_cbs;

		std::thread::id __context_thread_id;

		static std::atomic_int __obj_type_id_counter;
		template <typename T>
		static int get_objtype_id() {
			static int id = ++__obj_type_id_counter;
			return id;
		}

	protected:
		std::thread io_thread;
		asio::io_service io_service;

		friend class BaseObject;
		virtual void on_remove_object(int32_t objid) = 0;
		void invalidate_object(std::shared_ptr<BaseObject> obj);

		template<typename T>
		void __register_ObjectSetListener(std::weak_ptr<ObjectSetListener<T> > osl) {
			auto objid = get_objtype_id<T>();

			register_obj_cbs[objid] =
				[osl](std::shared_ptr<BaseObject> obj) {
				if(auto locked = osl.lock()) {
					auto T_obj = std::dynamic_pointer_cast<T>(obj);
					if(T_obj)
						locked->object_registered(T_obj);
				}
			};
			unregister_obj_cbs[objid] =
				[osl](std::shared_ptr<BaseObject> obj) {
				if(auto locked = osl.lock()) {
					auto T_obj = std::dynamic_pointer_cast<T>(obj);
					if(T_obj)
						locked->object_unregistered(T_obj);
				}
			};
		}

	public:
		template <class T>
		void unregister_object(std::shared_ptr<T> obj) {
			auto objid = get_objtype_id<T>();
			auto range = unregister_obj_cbs.equal_range(objid);
			for(auto cb : range) {
				cb.second(obj);
			}
		}

		template <class T>
		void register_object(std::shared_ptr<T> obj) {
			auto objid = get_objtype_id<T>();
			auto range = register_obj_cbs.equal_range(objid);
			for(auto cb : range) {
				cb.second(obj);
			}
		}

		class ContextNotConnected : public std::runtime_error {
		public:
			ContextNotConnected()
				: runtime_error("RemoteInterface::Context not connected to client/server.")
				{}
			virtual ~ContextNotConnected() {}
		};

		class ContextAlreadyConnected : public std::runtime_error {
		public:
			ContextAlreadyConnected()
				: runtime_error("RemoteInterface::Context already connected to client/server.")
				{}
			virtual ~ContextAlreadyConnected() {}
		};

		class FailureResponse : public std::runtime_error {
		public:
			std::string response_message;
			FailureResponse(const std::string &_response_message) :
				runtime_error("Remote interface command execution failed - response to sender generated."),
				response_message(_response_message) {}
			virtual ~FailureResponse() {}
		};

		~Context();
		virtual void distribute_message(std::shared_ptr<Message> &msg, bool via_udp) = 0;
		virtual std::shared_ptr<BaseObject> get_object(int32_t objid) = 0;

		virtual void post_action(std::function<void()> f, bool do_synch = false);
		std::shared_ptr<Message> acquire_message();
		std::shared_ptr<Message> acquire_reply(const Message &originator);

		void run_context() {
			__context_thread_id = std::this_thread::get_id();
			io_service.run();
		}
	};

	class MessageHandler : public std::enable_shared_from_this<MessageHandler> {
	private:
		Message read_msg;
		std::deque<std::shared_ptr<Message> > write_msgs;

		void do_read_header();
		void do_read_body();
		void do_write();
		void do_write_udp(std::shared_ptr<Message> &msg);

	protected:
		asio::ip::tcp::socket my_socket;

		std::shared_ptr<asio::ip::udp::socket> my_udp_socket;
		asio::ip::udp::endpoint udp_target_endpoint;

	public:
		class OnlyForDelivery : public std::runtime_error {
		public:
			OnlyForDelivery() : runtime_error("MessageHandler object only intended for use with deliver_message().") {}
			virtual ~OnlyForDelivery() {}
		};

		MessageHandler(asio::io_service &io_service);
		MessageHandler(asio::ip::tcp::socket _socket);

		void start_receive();

		virtual void deliver_message(std::shared_ptr<Message> &msg, bool via_udp = false);

		virtual void on_message_received(const Message &msg) = 0;
		virtual void on_connection_dropped() = 0;
	};

	class BaseObject : public std::enable_shared_from_this<BaseObject> {
	private:
		friend class Context;

		bool __is_server_side = false;
		volatile bool __object_is_valid = true;

	protected:
		inline bool check_object_is_valid() {
			return __object_is_valid;
		}

		class Factory {
		private:
			void register_factory();

			const char* type;
			bool static_single_object;
			int what_sides; // if this factory is ServerSide, ClientSide or both.

		public:
			class FactoryAlreadyCreated : public std::runtime_error {
			public:
				FactoryAlreadyCreated() : runtime_error("Tried to create multiple RemoteInterface::BaseObject::Factory for the same type.") {}
				virtual ~FactoryAlreadyCreated() {}
			};

			class StaticSingleObjectAlreadyCreated : public std::runtime_error {
			public:
				StaticSingleObjectAlreadyCreated()
					: runtime_error("Tried to create multiple RemoteInterface::BaseObject "
							"for factory marked as static single.") {}
				virtual ~StaticSingleObjectAlreadyCreated() {}
			};

			Factory(const char* type, bool static_single_object = false);
			Factory(WhatSide what_side, const char* type, bool static_single_object = false);
			~Factory();

			const char* get_type() const;

			virtual std::shared_ptr<BaseObject> create(const Message &serialized) = 0;
			virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) = 0;

			std::shared_ptr<BaseObject> create_static_single_object(int32_t new_obj_id);
		};

		BaseObject(const Factory *factory, const Message &serialized);
		BaseObject(int32_t new_obj_id, const Factory *factory);

		Context* context;
		std::mutex base_object_mutex;

		void request_delete_me();

		void send_object_message(std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback, bool via_udp = false);
		void send_object_message(std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback,
					 std::function<void(const Message *reply_message)> reply_received_callback);

		inline bool is_server_side() { return __is_server_side; }
		inline bool is_client_side() { return !__is_server_side; }

	public:
		class ObjectType {
		public:
			const char *type_name;
		};

		class ObjectWasDeleted : public std::runtime_error {
		public:
			ObjectWasDeleted() : runtime_error("Tried to use deleted RemoteInterface object.") {}
			virtual ~ObjectWasDeleted() {}
		};

		class NoSuchFactory : public std::runtime_error {
		public:
			NoSuchFactory() : runtime_error("Tried to create object in unknown RemoteInterface::BaseObject::Factory.") {}
			virtual ~NoSuchFactory() {}
		};
		class DuplicateObjectId : public std::runtime_error {
		public:
			DuplicateObjectId() : runtime_error("Tried to create an RemoteInterface::BaseObject with an already used id.") {}
			virtual ~DuplicateObjectId() {}
		};
		class NoSuchObject : public std::runtime_error {
		public:
			NoSuchObject() : runtime_error("Tried to route message to an unknown RemoteInterface::BaseObject.") {}
			virtual ~NoSuchObject() {}
		};
		class ObjIdOverflow : public std::runtime_error {
		public:
			ObjIdOverflow()
				: runtime_error("This session ran out of possible values RemoteInterface::BaseObject::obj_id.") {}
			virtual ~ObjIdOverflow() {}
		};

		int32_t get_obj_id();
		auto get_type() -> ObjectType;

		void set_context(Context* context);

		virtual void post_constructor_client() = 0; // called after the constructor has been called
		virtual void process_message_server(Context* context, MessageHandler* src, const Message &msg) = 0; // server side processing
		virtual void process_message_client(Context* context, MessageHandler* src, const Message &msg) = 0; // client side processing
		virtual void serialize(std::shared_ptr<Message> &target) = 0;
		virtual void on_delete(Context* context) = 0; // called on client side when it's about to be deleted

		static std::shared_ptr<BaseObject> create_object_on_client(const Message &msg);
		static std::shared_ptr<BaseObject> create_object_on_server(int32_t new_obj_id, const std::string &type);

		static void create_static_single_objects_on_server(
			std::function<int()> get_new_id_callback,
			std::function<void(std::shared_ptr<BaseObject>)> new_obj_created);

	private:
		static std::map<std::string, Factory *> server_factories;
		static std::map<std::string, Factory *> client_factories;

		int32_t obj_id;

		const Factory *my_factory;
	};

	class SimpleBaseObject : public BaseObject {
	private:
		std::map<std::string,
			 std::function<void(Context* context, MessageHandler *src, const Message& msg)> > command2function;

	public:
		class HandlerAlreadyRegistered : public std::runtime_error {
		public:
			HandlerAlreadyRegistered() : runtime_error("Tried to register the same command id twice.") {}
			virtual ~HandlerAlreadyRegistered() {}
		};

		SimpleBaseObject(const Factory *factory, const Message &serialized);
		SimpleBaseObject(int32_t new_obj_id, const Factory* factory);

		void register_handler(const std::string& command_id,
				      std::function<void(Context* context, MessageHandler *src, const Message& msg)> handler);
		void send_message(const std::string &command_id,
				  std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback,
				  std::function<void(const Message *reply_message)> reply_received_callback);
		void send_message_to_server(
			const std::string &command_id,
			std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback,
			std::function<void(const Message *reply_message)> reply_received_callback);
		void send_message(const std::string &command_id,
				  std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback);
		void send_message_to_server(
			const std::string &command_id,
			std::function<void(std::shared_ptr<Message> &msg_to_send)> create_msg_callback);

		virtual void post_constructor_client() override; // called after the constructor has been called
		virtual void process_message_server(Context* context,
						    MessageHandler *src,
						    const Message &msg) override; // server side processing
		virtual void process_message_client(Context* context,
						    MessageHandler *src,
						    const Message &msg) override; // client side processing
		virtual void serialize(std::shared_ptr<Message> &target) override;
		virtual void on_delete(Context* context) override; // called on client side when it's about to be deleted
	};

	class HandleList : public BaseObject {
	private:
		std::map<std::string, std::string> handle2hint;
		static std::weak_ptr<HandleList> clientside_handle_list;

		class HandleListFactory : public Factory {
		public:
			HandleListFactory();

			virtual std::shared_ptr<BaseObject> create(const Message &serialized) override;
			virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) override;
		};

		static HandleListFactory handlelist_factory;

		virtual void post_constructor_client(); // called after the constructor has been called
		virtual void process_message_server(Context* context,
						    MessageHandler *src,
						    const Message &msg); // server side processing
		virtual void process_message_client(Context* context,
						    MessageHandler *src,
						    const Message &msg); // client side processing
		virtual void serialize(std::shared_ptr<Message> &target);
		virtual void on_delete(Context* context); // called on client side when it's about to be deleted

	public:
		class FailedToCreateMachine : public std::runtime_error {
		public:
			FailedToCreateMachine() : runtime_error("Server failed to create the machine.") {}
			virtual ~FailedToCreateMachine() {}
		};

		HandleList(const Factory *factory, const Message &serialized); // create client side HandleList
		HandleList(int32_t new_obj_id, const Factory *factory); // create server side HandleList

		static std::map<std::string, std::string> get_handles_and_hints();
		static std::string create_instance(const std::string &handle, double xpos, double ypos);
	};

	class GlobalControlObject : public BaseObject {
	private:
		class GlobalControlObjectFactory : public Factory {
		public:
			GlobalControlObjectFactory();

			virtual std::shared_ptr<BaseObject> create(const Message &serialized) override;
			virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) override;
		};

		virtual void post_constructor_client() override; // called after the constructor has been called
		virtual void process_message_server(Context* context,
						    MessageHandler *src,
						    const Message &msg) override; // server side processing
		virtual void process_message_client(Context* context,
						    MessageHandler *src,
						    const Message &msg) override; // client side processing
		virtual void serialize(std::shared_ptr<Message> &target) override;
		virtual void on_delete(Context* context) override; // called on client side when it's about to be deleted

		void parse_serialized_arp_patterns(std::vector<std::string> &retval, const std::string &serialized_arp_patterns);
		void serialize_arp_patterns(std::shared_ptr<Message> &target);
	public:
		GlobalControlObject(const Factory *factory, const Message &serialized); // create client side HandleList
		GlobalControlObject(int32_t new_obj_id, const Factory *factory); // create server side HandleList

	public: // client side interface
		class PlaybackStateListener {
		public:
			virtual void playback_state_changed(bool is_playing) = 0;
			virtual void recording_state_changed(bool is_recording) = 0;
			virtual void periodic_playback_update(int current_row) = 0;
		};

		static std::shared_ptr<GlobalControlObject> get_global_control_object(); // get a shared ptr to the current GCO, shared_ptr will be empty if the client is not connected

		std::vector<std::string> get_pad_arpeggio_patterns();

		bool is_it_playing();
		void play();
		void stop();
		void rewind();
		void set_record_state(bool do_record);
		bool get_record_state();
		std::string get_record_file_name();

		int get_lpb();
		int get_bpm();
		void set_lpb(int lpb);
		void set_bpm(int lpb);

		static void register_playback_state_listener(std::shared_ptr<PlaybackStateListener> listener_object);

//		void register_periodic(std::function<void(int line)>, int nr_lines_per_period);

	private:
		static std::weak_ptr<GlobalControlObject> clientside_gco;
		static GlobalControlObjectFactory globalcontrolobject_factory;
		static std::mutex gco_mutex;
		static std::vector<std::weak_ptr<PlaybackStateListener> > playback_state_listeners;

		static void insert_new_playback_state_listener_object(std::shared_ptr<PlaybackStateListener> listener_object);

		int bpm, lpb;
		bool is_playing = false, is_recording = false;
	};

	class SampleBank : public BaseObject {
	private:
		class SampleBankFactory : public Factory {
		public:
			SampleBankFactory();

			virtual std::shared_ptr<BaseObject> create(const Message &serialized) override;
			virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id) override;
		};
		static SampleBankFactory samplebank_factory;

		std::map<int, std::string> sample_names;
		std::string bank_name;

		static std::mutex clientside_samplebanks_mutex;
		static std::map<std::string, std::weak_ptr<SampleBank> > clientside_samplebanks;

		template <class SerderClassT>
		void serderize_samplebank(SerderClassT &serder); // serder is an either an ItemSerializer or ItemDeserializer object.

	public:
		class NoSampleLoaded : public std::runtime_error {
		public:
			NoSampleLoaded() : runtime_error("No sample loaded at that position in the bank.") {}
			virtual ~NoSampleLoaded() {}
		};

		class NoSuchSampleBank : public std::runtime_error {
		public:
			NoSuchSampleBank() : runtime_error("No sample bank that is matching the queried name exists.") {}
			virtual ~NoSuchSampleBank() {}
		};

		SampleBank(const Factory *factory, const Message &serialized); // create client side HandleList
		SampleBank(int32_t new_obj_id, const Factory *factory); // create server side HandleList

		std::string get_name();
		std::string get_sample_name(int bank_index);

		void load_sample(int bank_index, const std::string &serverside_file_path);

		virtual void post_constructor_client() override;
		virtual void process_message_server(Context* context,
						    MessageHandler *src,
						    const Message &msg) override;
		virtual void process_message_client(Context* context,
						    MessageHandler *src,
						    const Message &msg) override;
		virtual void serialize(std::shared_ptr<Message> &target) override;
		virtual void on_delete(Context* context) override;

		static std::shared_ptr<SampleBank> get_bank(const std::string name); // empty string or "<global>" => global SampleBank
	};

	class RIMachine : public BaseObject {
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

		/* server side API */
		RIMachine(const Factory *factory, const Message &serialized);
		RIMachine(int32_t new_obj_id, const Factory *factory);

		void serverside_init_from_machine_ptr(Machine *m_ptr);

	public:
		/* mixed server/client side API */
		void attach_input(std::shared_ptr<RIMachine> source_machine,
				  const std::string &source_output_name,
				  const std::string &destination_input_name);
		void detach_input(std::shared_ptr<RIMachine> source_machine,
				  const std::string &source_output_name,
				  const std::string &destination_input_name);

	public: 	/* client side base API */
		class RIMachineSetListener {
		public:
			virtual void ri_machine_registered(std::shared_ptr<RIMachine> ri_machine) = 0;
			virtual void ri_machine_unregistered(std::shared_ptr<RIMachine> ri_machine) = 0;
		};

		class RIMachineStateListener {
		public:
			virtual void on_move() = 0;
			virtual void on_attach(std::shared_ptr<RIMachine> src_machine,
					       const std::string src_output,
					       const std::string dst_input) = 0;
			virtual void on_detach(std::shared_ptr<RIMachine> src_machine,
					       const std::string src_output,
					       const std::string dst_input) = 0;
		};

		class RIController {
		public:
			class NoSuchEnumValue : public std::runtime_error {
			public:
				NoSuchEnumValue() : runtime_error("No such enum value in RIController.") {}
				virtual ~NoSuchEnumValue() {}
			};

			enum Type {
				ric_int = 0,
				ric_float = 1,
				ric_bool = 2,
				ric_string = 3,
				ric_enum = 4, // integer values map to a name
				ric_sigid = 5, // integer values map to sample bank index
				ric_double = 6
			};

			RIController(int ctrl_id, Machine::Controller *ctrl);
			RIController(std::function<
					     void(std::function<void(std::shared_ptr<Message> &msg_to_send)> )
					     >  _send_obj_message,
				     const std::string &serialized);

			std::string get_name(); // name of the control
			std::string get_title(); // user displayable title

			Type get_type();

			void get_min(float &val);
			void get_max(float &val);
			void get_step(float &val);
			void get_min(double &val);
			void get_max(double &val);
			void get_step(double &val);
			void get_min(int &val);
			void get_max(int &val);
			void get_step(int &val);

			void get_value(int &val);
			void get_value(float &val);
			void get_value(double &val);
			void get_value(bool &val);
			void get_value(std::string &val);

			std::string get_value_name(int val);

			void set_value(int val);
			void set_value(float val);
			void set_value(double val);
			void set_value(bool val);
			void set_value(const std::string &val);

			bool has_midi_controller(int &coarse_controller, int &fine_controller);

			std::string get_serialized_controller();
		private:
			std::function<
			void(
				std::function<void(std::shared_ptr<Message> &msg_to_send)>
				)
			>  send_obj_message;

			template <class SerderClassT>
			void serderize_controller(SerderClassT &serder); // serder is an either an ItemSerializer or ItemDeserializer object.

			int ctrl_id = -1;

			int ct_type = ric_int; //using enum Type values, but need to be explicitly stored as an integer
			std::string name, title;

			struct data_f {
				float min, max, step, value;
			};
			struct data_d {
				double min, max, step, value;
			};
			struct data_i {
				int min, max, step, value;
			};
			union {
				data_f f;
				data_d d;
				data_i i;
			} data;

			std::string str_data = "";
			bool bl_data = false;

			std::map<int, std::string> enum_names;

			int coarse_controller = -1, fine_controller = -1;
		};

		/// get a hint about what this machine is (for example, "effect" or "generator")
		std::string get_hint();

		/// Returns a set of controller groups
		std::vector<std::string> get_controller_groups();
		/// Returns the set of all controller names in a given group
		std::vector<std::string> get_controller_names(const std::string &group_name);
		/// Return a RIController object
		std::shared_ptr<RIController> get_controller(const std::string &controller_name);

		std::string get_name();
		std::string get_sibling_name();
		std::string get_machine_type();
		std::shared_ptr<RIMachine> get_sequencer();

		double get_x_position();
		double get_y_position();
		void set_position(double xpos, double ypos);

		void delete_machine(); // called on client - asks server to delete the machine.

		std::vector<std::string> get_input_names();
		std::vector<std::string> get_output_names();

		// if the client side app wants to be notified when things in this machine changes
		void set_state_change_listener(std::weak_ptr<RIMachineStateListener> state_listener);

	public:        /* client side Machine Sequencer/Pad API */
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

		int get_nr_of_loops();
		void set_loop_id_at(int position, int loop_id);

	public:
		virtual void post_constructor_client(); // called after the constructor has been called
		virtual void process_message_server(Context* context,
						    MessageHandler *src,
						    const Message &msg); // server side processing
		virtual void process_message_client(Context* context,
						    MessageHandler *src,
						    const Message &msg); // client side processing
		virtual void serialize(std::shared_ptr<Message> &target);
		virtual void on_delete(Context* context); // called on client side when it's about to be deleted

	private:
		class ServerSideControllerContainer : public IDAllocator {
		public:
			std::map<int32_t, Machine::Controller *> id2ctrl; // map Controller id to Machine::Controller

			~ServerSideControllerContainer() {
				for(auto i2ct : id2ctrl) {
					delete i2ct.second;
				}
			}

		};

		class RIMachineFactory : public Factory {
		public:
			RIMachineFactory();

			virtual std::shared_ptr<BaseObject> create(const Message &serialized);
			virtual std::shared_ptr<BaseObject> create(int32_t new_obj_id);

		};

		static RIMachineFactory rimachine_factory;

		// clientside statics
		static std::set<std::weak_ptr<RIMachineSetListener>,
				std::owner_less<std::weak_ptr<RIMachineSetListener> > > set_listeners;
		static std::weak_ptr<RIMachine> sink;
		static std::map<std::string, std::weak_ptr<RIMachine> > name2machine;
		static std::mutex ri_machine_lock;

		std::multimap<int32_t, std::pair<std::string, std::string> > connection_data; // [source machine objid]->(output name, input name)
		std::vector<std::string> inputs;
		std::vector<std::string> outputs;
		std::vector<std::string> controller_groups;

		// map a weak ptr representing the ClientAgent to a ServerSideControllerContainer
		std::map<std::weak_ptr<MessageHandler>,
			 std::shared_ptr<ServerSideControllerContainer>,
			 std::owner_less<std::weak_ptr<MessageHandler> >
			 > client2ctrl_container;

		std::string name, sibling;
		std::string type;
		bool is_sink = false;
		Machine *real_machine_ptr = nullptr;
		double xpos, ypos;
		std::set<std::string> midi_controllers;

		// clean up Controller objects for disconnected clients
		void cleanup_stray_controllers();

		// serialize a Controller into a std::string
		std::string serialize_controller(Machine::Controller *ctrl);

		// returns a serialized controller representation
 		std::string process_get_ctrl_message(const std::string &ctrl_name, MessageHandler *src);

		void process_setctrl_val_message(MessageHandler *src, const Message &msg);

		void process_attach_message(Context* context, const Message &msg);
		void process_detach_message(Context* context, const Message &msg);

		void parse_serialized_midi_ctrl_list(std::string serialized);
		void parse_serialized_connections_data(std::string serialized);
		void call_state_listeners(std::function<void(std::shared_ptr<RIMachineStateListener> listener)> callback);

		// used on client side when somethings changed server side
		std::set<std::weak_ptr<RIMachineStateListener>,
			 std::owner_less<std::weak_ptr<RIMachineStateListener> > >state_listeners;

	public:
		class NoSuchMachine : public std::runtime_error {
		public:
			NoSuchMachine() : runtime_error("No such machine registered.") {}
			virtual ~NoSuchMachine() {}
		};

		static void register_ri_machine_set_listener(std::weak_ptr<RIMachineSetListener> set_listener);
		static std::shared_ptr<RIMachine> get_by_name(const std::string &name);
		static std::shared_ptr<RIMachine> get_sink();
	};
};

#endif
