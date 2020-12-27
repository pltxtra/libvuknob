/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2014 by Anton Persson
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

#include "gnuvg_livepad.hh"
#include "svg_loader.hh"
#include "scales.hh"

#include "../engine_code/client.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#include "common.hh"

static void yes(void *ignored) {
	static KammoGUI::UserEvent *ue = NULL;
	static KammoGUI::UserEvent *callback_ue = NULL;
	KammoGUI::get_widget((KammoGUI::Widget **)&ue, "showMachineTypeListScroller");
	KammoGUI::get_widget((KammoGUI::Widget **)&callback_ue, "showComposeContainer");
	if(ue != NULL && callback_ue != NULL) {
		std::map<std::string, void *> args;
		args["hint_match"] = strdup("generator");
		args["callback_event"] = callback_ue;
		KammoGUI::EventHandler::trigger_user_event(ue, args);
	}
}

static void no(void *ignored) {
	// do not do anything
}

static void ask_to_create_machine() {
	SATAN_DEBUG("-------- Would ask for new machine here....\n");
//	KammoGUI::ask_yes_no("Create machine?", "No machine available - do you want to create one now?",
//			     yes, NULL,
//			     no, NULL);
}

void GnuVGLivePad::on_resize() {
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);

	int pixel_w, pixel_h;
	float canvas_w_inches, canvas_h_inches;

	auto canvas = get_canvas();
	canvas->get_size_pixels(pixel_w, pixel_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	float w_fingers = canvas_w_inches / INCHES_PER_FINGER;
	float h_fingers = canvas_h_inches / INCHES_PER_FINGER;

	// calculate pixel size of 12 by 15 fingers
	double finger_width = (12.0 * pixel_w ) / w_fingers;
	double finger_height = (15.0 * pixel_h) / h_fingers;

	// if larger than the canvas pixel size, then limit it
	finger_width = finger_width < pixel_w ? finger_width : pixel_w;
	finger_height = finger_height < pixel_h ? finger_height : pixel_h;

	// calculate scaling factor
	double scaling_w = (finger_width) / (double)document_size.width;
	double scaling_h = (finger_height) / (double)document_size.height;

	doc_scale = scaling_w < scaling_h ? scaling_w : scaling_h;

	// calculate translation
	doc_x1 = (pixel_w - document_size.width * doc_scale) / 2.0;
	doc_y1 = (pixel_h - document_size.height * doc_scale) / 2.0;
	doc_x2 = doc_x1 + document_size.width * doc_scale;
	doc_y2 = doc_y1 + (8.0 / 10.0) * document_size.height * doc_scale; // we only use 8 out of 10 because the pad is 8 fingers high while the buttons below the pad are 2 fingers high.

	// initiate transform_m
	transform_m.init_identity();
	transform_m.scale(doc_scale, doc_scale);
	transform_m.translate(doc_x1, doc_y1);
}

void GnuVGLivePad::on_render() {
	{ // Translate the document, and scale it properly to fit the defined viewbox
		KammoGUI::GnuVGCanvas::ElementReference root(this);
		root.set_transform(transform_m);
	}

	{ /* get graphArea size */
		graphArea_element.get_viewport(graphArea_viewport);
	}
}

void GnuVGLivePad::select_machine() {
	SATAN_DEBUG("      ---- ::select_machine() 1\n");
	if(msequencers.size() == 0) {
		SATAN_DEBUG("      ---- ::select_machine() 1.2\n");
		ask_to_create_machine();
		return;
	}
	SATAN_DEBUG("      ---- ::select_machine() 2\n");
	listView->clear();
	SATAN_DEBUG("      ---- ::select_machine() 3\n");

	for(auto k_weak : msequencers) {
		if(auto k = k_weak.lock()) {
			SATAN_DEBUG("Adding machine %s to selection.\n", k->get_name().c_str());
			listView->add_row(k->get_name());
		}
	}
	SATAN_DEBUG("      ---- ::select_machine() 4\n");

	listView->select_from_list("Select machine",
				   [this](bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   for(auto k_weak : msequencers) {
						   if(auto k = k_weak.lock()) {
							   if(row_text == k->get_name()) {
								   SATAN_DEBUG("Machine selected: %s\n",
									       k->get_name().c_str());
								   switch_MachineSequencer(k);
							   }
						   }
					   }

					   refresh_machine_settings();
				   }

		);
	SATAN_DEBUG("      ---- ::select_machine() 5\n");
}

void GnuVGLivePad::select_arp_pattern() {
	listView->clear();

	if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
		for(auto arpid :  gco->get_pad_arpeggio_patterns()) {
			listView->add_row(arpid);
		}
	}

	listView->select_from_list("Select Arp Pattern",
				   [this](bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   arp_pattern = row_text;

					   refresh_machine_settings();
				   }
		);
}

void GnuVGLivePad::refresh_scale_key_names() {
	SATAN_DEBUG("GnuVGLivePad::refresh_scale_key_names() - A\n");
	auto scalo = Scales::get_scales_object();
	SATAN_DEBUG("GnuVGLivePad::refresh_scale_key_names() - B\n");
	if(!scalo) return;
	SATAN_DEBUG("GnuVGLivePad::refresh_scale_key_names() - C\n");

	if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
		SATAN_DEBUG("GnuVGLivePad::refresh_scale_key_names() - D\n");
		std::vector<int> keys = scalo->get_scale_keys(scale_name);
		SATAN_DEBUG("keys[%s].size() = %d\n", scale_name.c_str(), keys.size());
		for(int n = 0; n < 8; n++) {
			std::stringstream key_name_id;
			key_name_id << "key_name_" << n;
			SATAN_DEBUG("Will try to get id: %s\n", key_name_id.str().c_str());
			KammoGUI::GnuVGCanvas::ElementReference key_name(this, key_name_id.str());

			int key_number = keys[n % keys.size()];

			std::stringstream key_name_text;
			key_name_text << scalo->get_key_text(key_number) << (octave + (key_number / 12) + (n / keys.size()));
			SATAN_DEBUG("   [%d] -> %s\n", key_number, key_name_text.str().c_str());
			key_name.set_text_content(key_name_text.str());
		}
	}
}

void GnuVGLivePad::select_scale() {
	listView->clear();

	if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
		auto scalo = Scales::get_scales_object();
		if(!scalo) return;

		for(auto scale : scalo->get_scale_names()) {
			listView->add_row(scale);
		}
	}

	listView->select_from_list("Select scale",
				   [this](bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   scale_name = row_text;

					   if(auto gco = RemoteInterface::GlobalControlObject::get_global_control_object()) {
						   auto scalo = Scales::get_scales_object();
						   if(!scalo) return;

						   int n = 0;

						   for(auto scale : scalo->get_scale_names()) {
							   if(scale_name == scale) {
								   scale_index = n;
							   }
							   n++;
						   }
					   }

					   refresh_machine_settings();
				   }
		);
}

void GnuVGLivePad::select_controller() {
	listView->clear();

	listView->add_row("Velocity");

	if(mseq) {
		for(auto ctrl : mseq->available_midi_controllers()) {
			listView->add_row(ctrl);
		}
	}

	listView->select_from_list("Select controller",
				   [this](bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   std::string new_ctrl = "Velocity"; // default to Velocity

					   if(mseq) {
						   for(auto ctrl : mseq->available_midi_controllers()) {
							   if(ctrl == row_text) {
								   new_ctrl = ctrl; // we found a match, keep this name
							   }
						   }
					   }

					   controller = new_ctrl;

					   refresh_machine_settings();
				   }
		);
}

void GnuVGLivePad::select_menu() {
	SATAN_DEBUG("      ---- ::select_menu() 1\n");
	if(!mseq) {
		ask_to_create_machine();
		return;
	}
	SATAN_DEBUG("      ---- ::select_menu() 2\n");

	listView->clear();

	auto rn_copy_to_loop = "Copy to loop editor";
	auto rn_custom_scale_editor = "Custom scale editor";

	listView->add_row(rn_copy_to_loop);
	listView->add_row(rn_custom_scale_editor);

	listView->select_from_list("Menu",
				   [this
				    , rn_copy_to_loop
				    , rn_custom_scale_editor]
				   (bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   if(row_text == rn_copy_to_loop) {
						   copy_to_loop();
					   } else if(row_text == rn_custom_scale_editor) {
						   scale_editor->show(mseq);
					   }
				   }
		);
}

void GnuVGLivePad::copy_to_loop() {
	listView->clear();

	if(mseq) {
		listView->add_row("New loop");
		std::string failure_message = "";

		try {
			int max_loop = 32; // XXX mseq->get_nr_of_loops();
			SATAN_DEBUG("max_loop: %d\n", max_loop);
			for(int k = 0; k < max_loop; k++) {
				std::ostringstream loop_id;
				loop_id << "Loop #" << k;
				listView->add_row(loop_id.str());
			}
		} catch(std::exception &e) {
			failure_message = e.what();
		}
		if(failure_message.size() != 0) jInformer::inform(failure_message);
	}

	listView->select_from_list("Copy to loop",
				   [this](bool row_selected, int row_index, const std::string &row_text) {
					   if(!row_selected) return;

					   if(mseq) {
						   if(row_index == 0) {
							   mseq->pad_export_to_loop();
						   } else {
							   mseq->pad_export_to_loop(row_index - 1);
						   }
					   }

					   refresh_machine_settings();
				   }
		);
}

void GnuVGLivePad::refresh_record_indicator() {
	KammoGUI::GnuVGCanvas::ElementReference recordIndicator_element(this, "recInd");

	recordIndicator_element.set_style(record ? "fill-opacity:1.0" : "fill-opacity:0.2");
}

void GnuVGLivePad::refresh_quantize_indicator() {
	KammoGUI::GnuVGCanvas::ElementReference quantizeOn_element(this, "quantizeOn");
	KammoGUI::GnuVGCanvas::ElementReference quantizeOff_element(this, "quantizeOff");

	quantizeOn_element.set_display(quantize ? "inline" : "none");
	quantizeOff_element.set_display(quantize ? "none" : "inline");
}

void GnuVGLivePad::refresh_scale_indicator() {
	int n = 0;

	if(auto scalo = Scales::get_scales_object()) {
		for(auto scale : scalo->get_scale_names()) {
			if(scale_index == n) {
				selectScale_element.find_child_by_class("selectedText").set_text_content(scale);
			}
			n++;
		}
	}
}

void GnuVGLivePad::refresh_controller_indicator() {
	std::string ctrl_name = "Velocity"; // default to Velocity

	if(mseq) {
		for(auto ctrl : mseq->available_midi_controllers()) {
			if(ctrl == controller) {
				ctrl_name = ctrl; // we found a match, keep this name
			}
		}
	}

	selectController_element.find_child_by_class("selectedText").set_text_content(ctrl_name);
}

void GnuVGLivePad::refresh_machine_settings() {
	SATAN_DEBUG("GnuVGLivePad::refresh_machine_settings() - A\n");
	refresh_scale_key_names();
	SATAN_DEBUG("GnuVGLivePad::refresh_machine_settings() - B\n");

	if(!is_playing && arp_direction != RISequence::arp_off) {
		jInformer::inform("Press the play button before using arpeggio.");
		arp_direction = RISequence::arp_off;
	}
	SATAN_DEBUG("GnuVGLivePad::refresh_machine_settings() - C\n");

	selectArpPattern_element.set_display(arp_direction == RISequence::arp_off ? "none" : "inline");
	SATAN_DEBUG("GnuVGLivePad::refresh_machine_settings() - D\n");

	std::string name = "mchn";
	SATAN_DEBUG("Refresh machine settings, mseq\n");
	if(mseq) {
		mseq->pad_set_octave(octave);
		mseq->pad_set_scale(scale_index);
		mseq->pad_set_record(record);
		mseq->pad_set_quantize(quantize);
		mseq->pad_assign_midi_controller(RISequence::pad_y_axis,
						 controller);
		mseq->pad_assign_midi_controller(RISequence::pad_z_axis,
						 do_pitch_bend ? pitch_bend_controller : "[none]");
		mseq->pad_set_chord_mode(chord_mode);
		mseq->pad_set_arpeggio_pattern(arp_pattern);
		mseq->pad_set_arpeggio_direction(arp_direction);

		name = mseq->get_name();
	}

	refresh_record_indicator();
	refresh_quantize_indicator();
	refresh_scale_indicator();
	refresh_controller_indicator();

	{
		selectArpPattern_element.find_child_by_class("selectedText").set_text_content(arp_pattern);
	}
	{
		auto no_chord = toggleChord_element.find_child_by_class("noChord");
		auto triad_chord = toggleChord_element.find_child_by_class("triadChord");
		auto quad_chord = toggleChord_element.find_child_by_class("quadChord");

		no_chord.set_display("none");
		triad_chord.set_display("none");
		quad_chord.set_display("none");

		switch(chord_mode) {
		case RISequence::chord_off:
			no_chord.set_display("inline");
			break;
		case RISequence::chord_triad:
			triad_chord.set_display("inline");
			break;
		case RISequence::chord_quad:
			quad_chord.set_display("inline");
			break;
		}
	}
	{
		auto arrow_up = toggleArpDirection_element.find_child_by_class("arrowUp");
		auto arrow_down = toggleArpDirection_element.find_child_by_class("arrowDown");

		arrow_up.set_display("none");
		arrow_down.set_display("none");

		switch(arp_direction) {
		default:
		case RISequence::arp_off:
			/* nothing */
			break;
		case RISequence::arp_forward:
			arrow_up.set_display("inline");
			break;
		case RISequence::arp_reverse:
			arrow_down.set_display("inline");
			break;
		case RISequence::arp_pingpong:
			arrow_up.set_display("inline");
			arrow_down.set_display("inline");
			break;
		}
	}
	{
		if(do_pitch_bend && pitch_bend_controller == "[none]") {
			jInformer::inform("Machine does not support pitch bend.");
			do_pitch_bend = false;
		}
		auto no_pitchbend = togglePitchBend_element.find_child_by_class("disable");
		no_pitchbend.set_display(do_pitch_bend ? "none" : "inline");
	}
	{
		selectMachine_element.find_child_by_class("selectedText").set_text_content(name);
	}
}

void GnuVGLivePad::octave_up() {
	octave = octave + 1;
	if(octave > 6) octave = 6;
	refresh_machine_settings();
}

void GnuVGLivePad::octave_down() {
	octave = octave - 1;
	if(octave < 1) octave = 1;
	refresh_machine_settings();
}

void GnuVGLivePad::toggle_chord() {
	switch(chord_mode) {
	case RISequence::chord_off:
		chord_mode = RISequence::chord_triad;
		break;
	case RISequence::chord_triad:
		chord_mode = RISequence::chord_quad;
		break;
	case RISequence::chord_quad:
		chord_mode = RISequence::chord_off;
		break;
	}
	refresh_machine_settings();
}

void GnuVGLivePad::toggle_arp_direction() {
	switch(arp_direction) {
	case RISequence::arp_off:
		arp_direction = RISequence::arp_forward;
		break;
	case RISequence::arp_forward:
		arp_direction = RISequence::arp_reverse;
		break;
	case RISequence::arp_reverse:
		arp_direction = RISequence::arp_pingpong;
		break;
	case RISequence::arp_pingpong:
		arp_direction = RISequence::arp_off;
		break;
	}

	refresh_machine_settings();
}

void GnuVGLivePad::toggle_pitch_bend() {
	do_pitch_bend = !do_pitch_bend;

	refresh_machine_settings();
}

void GnuVGLivePad::toggle_record() {
	record = !record;
	refresh_machine_settings();
}

void GnuVGLivePad::toggle_quantize() {
	quantize = !quantize;
	refresh_machine_settings();
}

void GnuVGLivePad::yes(void *void_ctx) {
	GnuVGLivePad *ctx = (GnuVGLivePad *)void_ctx;

	if(ctx != NULL && ctx->mseq)
		ctx->mseq->pad_clear();
}

void GnuVGLivePad::no(void *ctx) {
	// do not do anything
}

void GnuVGLivePad::ask_clear_pad() {
	SATAN_DEBUG("      ---- ::ask_clear_pad() 1\n");
	if(!mseq) {
		ask_to_create_machine();
		return;
	}
	std::ostringstream question;

	question << "Do you want to clear: " << mseq->get_name();

	KammoGUI::ask_yes_no("Clear jammin?",
			     question.str(),
			     yes, this,
			     no, this);
}

void GnuVGLivePad::button_on_event(KammoGUI::GnuVGCanvas::SVGDocument *source, KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				   const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG("event trigger for active element - %s\n", e_ref->get_id().c_str());

	GnuVGLivePad *ctx = (GnuVGLivePad *)source;

	{
		float x = event.get_x();
		float y = event.get_y();

		static float start_x, start_y;
		switch(event.get_action()) {
		case KammoGUI::MotionEvent::ACTION_CANCEL:
		case KammoGUI::MotionEvent::ACTION_OUTSIDE:
		case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
		case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		case KammoGUI::MotionEvent::ACTION_MOVE:
			break;
		case KammoGUI::MotionEvent::ACTION_DOWN:
			start_x = x;
			start_y = y;
			break;
		case KammoGUI::MotionEvent::ACTION_UP:
			x = start_x - x;
			y = start_y - y;
			x = x < 0 ? -x : x;
			y = y < 0 ? -y : y;
			if(x < 10 && y < 10) {
				if(e_ref->get_id() == "recordGroup") {
					ctx->toggle_record();
				} else if(e_ref->get_id() == "quantize") {
					ctx->toggle_quantize();
				} else if(e_ref->get_id() == "clearPad") {
					ctx->ask_clear_pad();
				} else if(e_ref->get_id() == "octaveUp") {
					ctx->octave_up();
				} else if(e_ref->get_id() == "octaveDown") {
					ctx->octave_down();
				} else if(e_ref->get_id() == "selectMachine") {
					ctx->select_machine();
				} else if(e_ref->get_id() == "selectArpPattern") {
					ctx->select_arp_pattern();
				} else if(e_ref->get_id() == "toggleChord") {
					ctx->toggle_chord();
				} else if(e_ref->get_id() == "arpDirection") {
					ctx->toggle_arp_direction();
				} else if(e_ref->get_id() == "pitchBend") {
					ctx->toggle_pitch_bend();
				} else if(e_ref->get_id() == "selectScale") {
					ctx->select_scale();
				} else if(e_ref->get_id() == "selectController") {
					ctx->select_controller();
				} else if(e_ref->get_id() == "selectMenu") {
					ctx->select_menu();
				}
			}
			break;
		}
	}
}

void GnuVGLivePad::graphArea_on_event(KammoGUI::GnuVGCanvas::SVGDocument *source, KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				      const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG(" ----------- ::graphArea_on_event() 1\n");
	GnuVGLivePad *ctx = (GnuVGLivePad *)source;

	if(!(ctx->mseq)) {
	SATAN_DEBUG(" ----------- ::graphArea_on_event() 1.2\n");
		ask_to_create_machine();
		return;
	}
	SATAN_DEBUG(" ----------- ::graphArea_on_event() 2\n");

	float x, y;
	/* scale by screen resolution, and complete svg offset */
	x = event.get_x(); y = event.get_y();
	if(x < ctx->doc_x1) x = ctx->doc_x1;
	if(x > ctx->doc_x2) x = ctx->doc_x2;
	if(y < ctx->doc_y1) y = ctx->doc_y1;
	if(y > ctx->doc_y2) y = ctx->doc_y2;

	x = (x - ctx->doc_x1);
	y = (y - ctx->doc_y1);

	int finger = event.get_pointer_id(event.get_action_index());
	RISequence::PadEvent_t pevt = RISequence::ms_pad_no_event;

	float width  = ctx->doc_x2 - ctx->doc_x1 + 1;
	float height = ctx->doc_y2 - ctx->doc_y1 + 1; // +1 to disable overflow

	for(auto k = 0; k < 10; k++) {
		ctx->f_active[k] = false;
	}

	auto z_value = ctx->do_pitch_bend ? ctx->l_ev_z : 0.5f;

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
		break;
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
		pevt = RISequence::ms_pad_press;
		break;
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		pevt = RISequence::ms_pad_release;
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		pevt = RISequence::ms_pad_press;
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		if(ctx->mseq) {
			for(int k = 0; k < event.get_pointer_count(); k++) {
				int f = event.get_pointer_id(k);
				if(f >= 10) continue; // skip if too many fingers

				/* scale by screen resolution, and complete svg offset */
				float ev_x = event.get_x(k); float ev_y = event.get_y(k);
				if(ev_x < ctx->doc_x1) ev_x = ctx->doc_x1;
				if(ev_x > ctx->doc_x2) ev_x = ctx->doc_x2;
				if(ev_y < ctx->doc_y1) ev_y = ctx->doc_y1;
				if(ev_y > ctx->doc_y2) ev_y = ctx->doc_y2;

				ev_x = (ev_x - ctx->doc_x1);
				ev_y = (ev_y - ctx->doc_y1);

				ctx->l_ev_x[f] = ev_x / width;
				ctx->l_ev_y[f] = ev_y / height;
				ctx->f_active[f] = true;
				ctx->mseq->pad_enqueue_event(f, RISequence::ms_pad_slide,
							     ctx->l_ev_x[f], ctx->l_ev_y[f],
							     z_value);
			}
		}
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		pevt = RISequence::ms_pad_release;
		break;
	}

	if(finger < 10 && pevt != RISequence::ms_pad_no_event && ctx->mseq) {
		ctx->mseq->pad_enqueue_event(finger, pevt, x / width, y / height, z_value);
	}
}

void GnuVGLivePad::playback_state_changed(bool _is_playing) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, _is_playing]() {
			is_playing = _is_playing;
			refresh_machine_settings();
		}
		);
}

void GnuVGLivePad::recording_state_changed(bool _is_recording) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, _is_recording]() {
			is_recording = _is_recording;
		}
		);
}

#define SENSOR_LIMIT 10.0f
#define MAX_ANGLE (M_PI / 4.0f)
void GnuVGLivePad::on_sensor_event(std::shared_ptr<KammoGUI::SensorEvent> event) {
	if(mseq) {
		static float y_cmp = 5.608387f, z_cmp = 8.043009f;

		float ev_y = event->values[1];
		float ev_z = event->values[2];

		float dot_p = z_cmp * ev_z + y_cmp * ev_y;
		float cross_p = z_cmp * ev_y - y_cmp * ev_z;
		float angle = atan2f(cross_p, dot_p);

		if(angle > MAX_ANGLE) angle = MAX_ANGLE;
		if(angle < (-MAX_ANGLE)) angle = -MAX_ANGLE;

		float alpha = 0.999;

		y_cmp = alpha * y_cmp + (1.0 - alpha) * ev_y;
		z_cmp = alpha * z_cmp + (1.0 - alpha) * ev_z;

		l_ev_z = (angle / (2.0f * MAX_ANGLE)) + 0.5f;

		if(do_pitch_bend) {
			for(auto f = 0; f < 10; f++) {
				// we only need to send the ev z for one finger, since it will be converted
				// into a midi controller value, and that value is global for the instrument
				// and not a per-finger value.
				if(f_active[f]) {
					mseq->pad_enqueue_event(
						f,
						RISequence::ms_pad_slide,
						l_ev_x[f], l_ev_y[f], l_ev_z);
					break;
				}
			}
		}
	}
}

GnuVGLivePad::GnuVGLivePad(KammoGUI::GnuVGCanvas *cnv)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/livePad2.svg"), cnv)
	, octave(3), scale_index(0), scale_name("C- "), record(false), quantize(false)
	, do_pitch_bend(false)
	, arp_pattern("built-in #0"), controller("velocity")
	, chord_mode(RISequence::chord_off)
	, arp_direction(RISequence::arp_off)
	, listView(NULL), scale_editor(NULL)
{
	for(int k = 0; k < 10; k++) {
		f_active[k] = false;
	}

	background_element = KammoGUI::GnuVGCanvas::ElementReference(this, "background");
	background_element.set_event_handler(
		[](SVGDocument *source,
		   KammoGUI::GnuVGCanvas::ElementReference *e,
		   const KammoGUI::MotionEvent &event) {}
		);

	{ // get the graph area and attach the event listener
		graphArea_element = KammoGUI::GnuVGCanvas::ElementReference(this, "graphArea");
		graphArea_element.set_event_handler(graphArea_on_event);
	}
	{ // get the controller selector and attach the event listener
		selectMachine_element = KammoGUI::GnuVGCanvas::ElementReference(this, "selectMachine");
		selectMachine_element.set_event_handler(button_on_event);
	}
	{ // get the arp_pattern selector and attach the event listener
		selectArpPattern_element = KammoGUI::GnuVGCanvas::ElementReference(this, "selectArpPattern");
		selectArpPattern_element.set_event_handler(button_on_event);
	}
	{ // get the chord toggle button and attach the event listener
		toggleChord_element = KammoGUI::GnuVGCanvas::ElementReference(this, "toggleChord");
		toggleChord_element.set_event_handler(button_on_event);
	}
	{ // get the pitchBend toggle button and attach the event listener
		togglePitchBend_element = KammoGUI::GnuVGCanvas::ElementReference(this, "pitchBend");
		togglePitchBend_element.set_event_handler(button_on_event);
	}
	{ // get the arpDirection toggle button and attach the event listener
		toggleArpDirection_element = KammoGUI::GnuVGCanvas::ElementReference(this, "arpDirection");
		toggleArpDirection_element.set_event_handler(button_on_event);
	}
	{ // get the scale selector and attach the event listener
		selectScale_element = KammoGUI::GnuVGCanvas::ElementReference(this, "selectScale");
		selectScale_element.set_event_handler(button_on_event);
	}
	{ // get the controller selector and attach the event listener
		selectController_element = KammoGUI::GnuVGCanvas::ElementReference(this, "selectController");
		selectController_element.set_event_handler(button_on_event);
	}
	{ // get the quantize element and attach the event listener
		quantize_element = KammoGUI::GnuVGCanvas::ElementReference(this, "quantize");
		quantize_element.set_event_handler(button_on_event);
	}
	{ // get the menu element and attach the event listener
		selectMenu_element = KammoGUI::GnuVGCanvas::ElementReference(this, "selectMenu");
		selectMenu_element.set_event_handler(button_on_event);
	}

	{ // get the octave up button and attach the event listener
		octaveUp_element = KammoGUI::GnuVGCanvas::ElementReference(this, "octaveUp");
		octaveUp_element.set_event_handler(button_on_event);
	}
	{ // get the octave down button and attach the event listener
		octaveDown_element = KammoGUI::GnuVGCanvas::ElementReference(this, "octaveDown");
		octaveDown_element.set_event_handler(button_on_event);
	}
	{ // get the clear button and attach the event listener
		clear_element = KammoGUI::GnuVGCanvas::ElementReference(this, "clearPad");
		clear_element.set_event_handler(button_on_event);
	}
	{ // get the record group and attach the event listener
		recordGroup_element = KammoGUI::GnuVGCanvas::ElementReference(this, "recordGroup");
		recordGroup_element.set_event_handler(button_on_event);
	}

	listView = new GnuVGListView(cnv);
	scale_editor = new GnuVGScaleEditor(cnv);

	refresh_machine_settings();

}

void GnuVGLivePad::switch_MachineSequencer(std::shared_ptr<RISequence> m) {
	mseq = m;

	if(mseq) {
		pitch_bend_controller = "[none]";
		SATAN_DEBUG("GnuVGLivePad::switch_MachineSequencer() A\n");
		for(auto ctrl : mseq->available_midi_controllers()) {
			if(ctrl.find("pitch_bend") != std::string::npos) {
				pitch_bend_controller = ctrl;
			}
		}
		SATAN_DEBUG("GnuVGLivePad::switch_MachineSequencer() B\n");
		refresh_machine_settings();
		SATAN_DEBUG("GnuVGLivePad::switch_MachineSequencer() C\n");
	}
}

GnuVGLivePad::~GnuVGLivePad() {
	QUALIFIED_DELETE(listView);
}

void GnuVGLivePad::show() {
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.set_display("inline");
}

void GnuVGLivePad::hide() {
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.set_display("none");
}

/***************************
 *
 * Public Statics
 *
 ***************************/

void GnuVGLivePad::object_registered(std::shared_ptr<RISequence> ri_sequence) {
	SATAN_DEBUG("GnuVGLivePad::ri_sequence_registered\n");

	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, ri_sequence]() {
			SATAN_DEBUG("GnuVGLivePad::ri_sequence_registered A\n");
			msequencers.insert(ri_sequence);
			SATAN_DEBUG("GnuVGLivePad::ri_sequence_registered B\n");
			if(!mseq) switch_MachineSequencer(ri_sequence);
			SATAN_DEBUG("GnuVGLivePad::ri_sequence_registered C\n");
		}
		);
}

void GnuVGLivePad::object_unregistered(std::shared_ptr<RISequence> ri_sequence) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, ri_sequence]() {
			if(mseq == ri_sequence)
				mseq.reset();

			for(auto found_weak = msequencers.begin(); found_weak != msequencers.end(); found_weak++) {
				if(auto found = found_weak->lock()) {
					if(found == ri_sequence) {
						found_weak = msequencers.erase(found_weak);
						break;
					} else if(!mseq) {
						mseq = found;
					}
					found_weak++;
				} else {
					found_weak = msequencers.erase(found_weak);
				}
			}
			refresh_machine_settings();
		}
		);
}

std::shared_ptr<GnuVGLivePad> GnuVGLivePad::create(KammoGUI::GnuVGCanvas *cnvs) {
	auto lpad = std::make_shared<GnuVGLivePad>(cnvs);
	RemoteInterface::ClientSpace::Client::register_object_set_listener<RISequence>(lpad);
	RemoteInterface::GlobalControlObject::register_playback_state_listener(lpad);
	KammoGUI::SensorEvent::register_listener(lpad);
	lpad->hide();
	return lpad;
}
