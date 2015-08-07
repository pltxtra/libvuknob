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

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"
#include "information_catcher.hh"

#include "machine_sequencer.hh"
#include "remote_interface.hh"

#ifdef ANDROID
#include "android_java_interface.hh"
#endif


#include <iostream>
#include "machine.hh"


#include "machine_sequencer.hh"
#include "dynamic_machine.hh"

#include <kamogui.hh>
#include <fstream>

KammoEventHandler_Declare(MainWindowHandler, "MainWindow:quitnow");

virtual void on_click(KammoGUI::Widget *wid) {
#ifdef ANDROID
	SATAN_ERROR("taking down service..\n");
	AndroidJavaInterface::takedown_service();
#endif

	SATAN_ERROR("stopping server..\n");
	RemoteInterface::Server::stop_server();
	SATAN_ERROR("disconnecting client..\n");
	RemoteInterface::Client::disconnect();
	SATAN_ERROR("exit application..\n");
	Machine::exit_application();
}

virtual bool on_delete(KammoGUI::Widget *wid) {
	exit(0);
}

virtual void on_init(KammoGUI::Widget *wid) {
	if(wid->get_id() == "MainWindow") {
		mkdir(DEFAULT_SATAN_ROOT, 0755);
		mkdir(DEFAULT_PROJECT_SAVE_PATH, 0755);
		mkdir(DEFAULT_EXPORT_PATH, 0755);

		InformationCatcher::init();
	}
}

KammoEventHandler_Instance(MainWindowHandler);

KammoEventHandler_Declare(LoopHandler,
			  "refreshProject:"
			  "decreaseBPM:scaleBPM:increaseBPM:"
			  "decreaseLPB:scaleLPB:increaseLPB:"
			  "decreaseShuffle:scaleShuffle:increaseShuffle"
	);

void refresh_bpm_label(int _new_bpm) {
	static KammoGUI::Label *bpmLabel;
	KammoGUI::get_widget((KammoGUI::Widget **)&bpmLabel, "scaleBPMlabel");
	std::ostringstream stream;
	stream << "BPM: " << _new_bpm;
	bpmLabel->set_title(stream.str());
}

void refresh_lpb_label(int _new_bpm) {
	static KammoGUI::Label *lpbLabel;
	KammoGUI::get_widget((KammoGUI::Widget **)&lpbLabel, "scaleLPBlabel");
	std::ostringstream stream;
	stream << "LPB: " << _new_bpm;
	lpbLabel->set_title(stream.str());
}

void refresh_shuffle_label(int _new_shuffle) {
	static KammoGUI::Label *shuffleLabel;
	KammoGUI::get_widget((KammoGUI::Widget **)&shuffleLabel, "scaleShufflelabel");
	std::ostringstream stream;
	stream << "Shuffle: " << _new_shuffle;
	shuffleLabel->set_title(stream.str());
}

void refresh_all() {
	{
		int bpm = Machine::get_bpm();

		static KammoGUI::Scale *bpmScale;
		KammoGUI::get_widget((KammoGUI::Widget **)&bpmScale, "scaleBPM");
		bpmScale->set_value(bpm);

		refresh_bpm_label(bpm);
	}
	{
		int lpb = Machine::get_lpb();

		static KammoGUI::Scale *lpbScale;
		KammoGUI::get_widget((KammoGUI::Widget **)&lpbScale, "scaleLPB");
		lpbScale->set_value(lpb);

		refresh_lpb_label(lpb);
	}
	{
		int shuffle = (int)(Machine::get_shuffle_factor());

		static KammoGUI::Scale *shuffleScale;
		KammoGUI::get_widget((KammoGUI::Widget **)&shuffleScale, "scaleShuffle");
		shuffleScale->set_value(shuffle);

		refresh_shuffle_label(shuffle);
	}
}

virtual void on_user_event(KammoGUI::UserEvent *ue, std::map<std::string, void *> args) {
	refresh_all();
}

virtual void on_init(KammoGUI::Widget *wid) {
	SATAN_DEBUG("on refreshProject - calling refresh_all()\n");
	refresh_all();
	SATAN_DEBUG("on refreshProject - returned from refresh_all()\n");
}

virtual void on_click(KammoGUI::Widget *wid) {
	int changeBPM = 0;
	int changeLPB = 0;
	int changeShuffle = 0;
	if(wid->get_id() == "decreaseBPM") {
		changeBPM--;
	}
	if(wid->get_id() == "increaseBPM") {
		changeBPM++;
	}
	if(wid->get_id() == "decreaseLPB") {
		changeLPB--;
	}
	if(wid->get_id() == "increaseLPB") {
		changeLPB++;
	}
	if(wid->get_id() == "decreaseShuffle") {
		changeShuffle--;
	}
	if(wid->get_id() == "increaseShuffle") {
		changeShuffle++;
	}
	if(changeBPM) {
		if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
			int bpm = gco->get_bpm();

			bpm += changeBPM;

			// we just try and set the new value, in case of an error we just ignore
			gco->set_bpm(bpm);
			// if there was an error we must make sure we have the correct value, so we read it back
			bpm = gco->get_bpm();

			static KammoGUI::Scale *bpmScale;
			KammoGUI::get_widget((KammoGUI::Widget **)&bpmScale, "scaleBPM");
			bpmScale->set_value(bpm);

			refresh_bpm_label(bpm);
		}
	}
	if(changeLPB) {
		if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
			int lpb = gco->get_lpb();

			lpb += changeLPB;

			// we just try and set the new value, in case of an error we just ignore
			gco->set_lpb(lpb);
			// if there was an error we must make sure we have the correct value, so we read it back
			lpb = gco->get_lpb();

			static KammoGUI::Scale *lpbScale;
			KammoGUI::get_widget((KammoGUI::Widget **)&lpbScale, "scaleLPB");
			lpbScale->set_value(lpb);

			refresh_lpb_label(lpb);
		}
	}
	if(changeShuffle) {
		int shuffle = (int)(Machine::get_shuffle_factor());

		shuffle += changeShuffle;

		// we just try and set the new value, in case of an error we just ignore
		try {Machine::set_shuffle_factor(shuffle);} catch(...) { /* ignore */ }
		// if there was an error we must make sure we have the correct value, so we read it back
		shuffle = (int)(Machine::get_shuffle_factor());

		static KammoGUI::Scale *shuffleScale;
		KammoGUI::get_widget((KammoGUI::Widget **)&shuffleScale, "scaleShuffle");
		shuffleScale->set_value(shuffle);

		refresh_shuffle_label(shuffle);
	}
}

virtual void on_value_changed(KammoGUI::Widget *wid) {
	int value = (int)((KammoGUI::Scale *)wid)->get_value();
	if(wid->get_id() == "scaleBPM") {
		if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
			// we just try and set the new value, in case of an error we just ignore
			gco->set_bpm(value);
			// if there was an error we must make sure we have the correct value, so we read it back
			value = gco->get_bpm();
		}

		refresh_bpm_label(value);
	}
	if(wid->get_id() == "scaleLPB") {
		if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
			// we just try and set the new value, in case of an error we just ignore
			gco->set_lpb(value);
			// if there was an error we must make sure we have the correct value, so we read it back
			value = gco->get_lpb();
		}

		refresh_lpb_label(value);
	}
	if(wid->get_id() == "scaleShuffle") {
		// we just try and set the new value, in case of an error we just ignore
		try {Machine::set_shuffle_factor(value);} catch(...) { /* ignore */ }
		// if there was an error we must make sure we have the correct value, so we read it back
		value = (int)(Machine::get_shuffle_factor());

		refresh_shuffle_label(value);
	}
}

KammoEventHandler_Instance(LoopHandler);
