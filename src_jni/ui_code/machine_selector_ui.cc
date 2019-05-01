/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
 * Copyright (C) 2011 by Anton Persson
 *
 * About SATAN:
 *   Originally developed as a small subproject in
 *   a course about applied signal processing.
 * Original Developers:
 *   Anton Persson
 *   Johan Thim
 *
 * http://www.733kru.org/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2; see COPYING for the complete License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include <jngldrum/jinformer.hh>
#include <kamogui.hh>
#include <fstream>
#include <condition_variable>
#include <mutex>

#include "engine_code/client.hh"
#include "remote_interface.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static KammoGUI::UserEvent *callback_event = NULL;

KammoEventHandler_Declare(MachineTypeHandler,"MachineType:addNode:showMachineTypeListScroller");

virtual void on_user_event(KammoGUI::UserEvent *ev, std::map<std::string, void *> data) {
	char *hint_match = NULL;

	// reset callback event
	callback_event = NULL;

	if(data.find("hint_match") != data.end())
		hint_match = (char *)data["hint_match"];

	// set callback event
	if(data.find("callback_event") != data.end())
		callback_event = (KammoGUI::UserEvent *)data["callback_event"];

	if(ev->get_id() == "showMachineTypeListScroller") {
		KammoGUI::List *list = NULL;
		KammoGUI::get_widget((KammoGUI::Widget **)&list, "MachineType");

		list->clear(); // flush the list before use
		std::vector<std::string> row_values;

		for(auto hnh : RemoteInterface::HandleList::get_handles_and_hints()) {
			unsigned int p = 0;
			if(hint_match)
				p = hnh.second.find(hint_match);

			row_values.clear();

			if((p >= 0 && p < hnh.second.size())) {
				// hint does contain the string hint_match
				// _OR_ hint_match was not set to begin with
				row_values.push_back(hnh.first);
				list->add_row(row_values);
				row_values.clear();
			}
		}
	}

	// if hint_match was set we assume it was strdup() there fore we must free() here
	if(hint_match)
		free(hint_match);
}

void auto_connect(std::shared_ptr<RemoteInterface::RIMachine> m) {
	if(!m) return; // silently fail if no m

	// we will try and attach m to the sink
	std::shared_ptr<RemoteInterface::RIMachine> sink;
	try {
		sink = RemoteInterface::RIMachine::get_sink();
	} catch(RemoteInterface::RIMachine::NoSuchMachine &e) {
		SATAN_DEBUG("auto_connect: no liveoutsink found.\n");
		goto fail;
	}

	{
		std::vector<std::string> inputs = sink->get_input_names();
		std::vector<std::string> outputs = m->get_output_names();

		// currently we are not smart enough to find matching sockets unless there are only one output and one input...
		if(inputs.size() != 1 || outputs.size() != 1) {
			SATAN_DEBUG("auto_connect: input.size() = %d, output.size() = %d.\n", inputs.size(), outputs.size());
			goto fail;
		}

		// OK, so there is only one possible connection to make.. let's try!
		try {
			sink->attach_input(m, outputs[0], inputs[0]);

			return; // we've suceeded
		} catch(jException e) {
			SATAN_ERROR("auto_connect: error attaching - %s\n", e.message.c_str());
		} catch(...) {
			// ignore the error, next stop is fail...
		}
	}

fail:
	{
		// automatic attempt failed, inform the user
		std::string  error_msg =
			"Automatic connection of machine '" +
			m->get_name() +
			"' failed. You must connect it manually if you want to use it.";
		jInformer::inform(error_msg);
	}
}

class MachineWaiter
	: public RemoteInterface::Context::ObjectSetListener<RemoteInterface::RIMachine>
	, public std::enable_shared_from_this<MachineWaiter> {
private:
	std::string wait_for_name;
	std::mutex mutex;
	std::condition_variable cv;
	std::shared_ptr<RemoteInterface::RIMachine> retval;

public:
	virtual void object_registered(std::shared_ptr<RemoteInterface::RIMachine> ri_machine) override {
		std::lock_guard<std::mutex> lk(mutex);
		bool was_found = false;

		SATAN_DEBUG("MachineWaiter::ri_machine_registered() %s\n", ri_machine->get_name().c_str());

		if(ri_machine->get_name() == wait_for_name) {
			SATAN_DEBUG("MachineWaiter::ri_machine_registered() found a match for %s\n",
				    ri_machine->get_name().c_str());
			retval = ri_machine;
			was_found = true;
		}

		if(was_found) {
			SATAN_DEBUG("MachineWaiter::ri_machine_registered() will notify...\n");
			cv.notify_all();
		}
	}

	virtual void object_unregistered(std::shared_ptr<RemoteInterface::RIMachine> ri_machine) override {
	}

	std::shared_ptr<RemoteInterface::RIMachine> wait_for(const std::string &name) {
		wait_for_name = name;

		SATAN_DEBUG("MachineWaiter::wait_for() will wait for %s...\n", wait_for_name.c_str());
		std::unique_lock<std::mutex> lk(mutex);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<RemoteInterface::RIMachine>(shared_from_this());
		cv.wait(lk);
		SATAN_DEBUG("MachineWaiter::wait_for() was notified!\n");

		return retval;
	}
};

virtual void on_select_row(KammoGUI::Widget *wid, KammoGUI::List::iterator iter) {
	KammoGUI::List *types = (KammoGUI::List *)wid;

	if(wid->get_id() == "MachineType") {
		std::string new_machine_name = RemoteInterface::HandleList::create_instance(types->get_value(iter, 0), 0.0, 0.0);

		SATAN_DEBUG("machine_selector_ui.cc: Created new machine %s\n", new_machine_name.c_str());

		auto mwaiter = std::make_shared<MachineWaiter>();
		auto new_machine = mwaiter->wait_for(new_machine_name);

		SATAN_DEBUG("New machine?: %s -> %p\n", new_machine_name.c_str(), new_machine.get());
		if(new_machine) {
			try {
				auto mseq = new_machine->get_sequencer();
				SATAN_DEBUG("New machine sequencer?: %p\n", mseq.get());
				SATAN_DEBUG("Setting loop...\n");
				mseq->set_loop_id_at(0, 0); // default to having a loop at position 0
			} catch(RemoteInterface::RIMachine::NoSuchMachine &e) { /* ignore */ }

			// if the callback_event is set we will try to autoconnect the node since the user
			// will not be presented with the machine connector interface
			if(callback_event) {
				auto_connect(new_machine);
			}
		}

		if(callback_event) {
			// call the callback
			trigger_user_event(callback_event);
		} else {
			static KammoGUI::UserEvent *ue = NULL;
			KammoGUI::get_widget((KammoGUI::Widget **)&ue, "showConnector_container");
			if(ue != NULL)
				trigger_user_event(ue);
		}
	}
}

KammoEventHandler_Instance(MachineTypeHandler);
