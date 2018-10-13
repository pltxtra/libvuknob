/*
 * vu|KNOB
 * (C) 2015 by Anton Persson
 *
 * http://www.vuknob.com/
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

#include <iostream>

#include "pattern_editor.hh"
#include "svg_loader.hh"
#include "common.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

PatternEditor* PatternEditor::singleton = nullptr;

void PatternEditor::on_backdrop_event(const KammoGUI::MotionEvent &event) {
	event_current_x = event.get_x();
	event_current_y = event.get_y();

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		display_action = true;
		event_start_x = event_current_x;
		event_start_y = event_current_y;
		new_tone_relative = (int)floor(event_current_y / finger_height);

		SATAN_DEBUG("event_start_x: %f\n", event_start_x);
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		start_at_sequence_position = timelines->get_sequence_minor_position_at(event_left_x);
		stop_at_sequence_position = timelines->get_sequence_minor_position_at(event_right_x);

		if(start_at_sequence_position < 0)
			start_at_sequence_position = 0;
		if(stop_at_sequence_position < 0)
			start_at_sequence_position = 0;

		start_at_sequence_position = (start_at_sequence_position >> 4) << 4;
		stop_at_sequence_position = (stop_at_sequence_position >> 4) << 4;

		SATAN_DEBUG("Forced - Start: %d - Stop: %d\n",
			    start_at_sequence_position,
			    stop_at_sequence_position);

		if(start_at_sequence_position != stop_at_sequence_position) {
			ri_seq->add_note(
				pattern_id,
				0, 0, 0xff,
				new_tone_relative + tone_at_top,
				start_at_seququence_position - pattern_start_position,
				stop_at_seququence_position - start_at_sequence_position
				);
		}

		display_action = false;
		break;
	}

	if(event_current_x < event_start_x) {
		event_left_x = event_current_x;
		event_right_x = event_start_x - event_current_x;
	} else {
		event_left_x = event_start_x;
		event_right_x = event_current_x - event_start_x;
	}

	auto newPieceIndicator = find_child_by_class("newPieceIndicator");

	newPieceIndicator.set_display(display_action ? "inline" : "none");
	if(display_action) {
		SATAN_DEBUG("b_x: %f, e_x: %f\n", b_x, e_x);
		double top = new_tone_relative * finger_height;
		double bottom = top + finger_height;
		newPieceIndicator.set_rect_coords(event_left_x, top, event_right_x, bottom);
	}

}

void PatternEditor::new_note_graphic(const RINote &note) {
}

void PatternEditor::delete_note_graphic(const RINote &note) {
}

PatternEditor::PatternEditor(KammoGUI::GnuVGCanvas* cnvs,
			     std::shared_ptr<TimeLines> _timelines)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/VerticalKeyboard.svg"), cnvs)
	, timelines(_timelines)
{
	singleton = this;

	backdrop_reference = new KammoGUI::GnuVGCanvas::ElementReference(this, "background");
	backdrop_reference->set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_backdrop_event(event);
			SATAN_DEBUG("End backdrop event...\n");
		}
		);
}

PatternEditor::~PatternEditor() {
	singleton = nullptr;
}

void PatternEditor::on_resize() {
	get_canvas_size(canvas_w, canvas_h);
	get_canvas_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	canvas_height_fingers = (int)tmp;

	// calculate the "width" of a finger tip in pixels
	tmp = canvas_w / ((double)canvas_width_fingers);
	finger_width = tmp;

	// calculate the "height" of a finger tip in pixels
	tmp = canvas_h / ((double)canvas_height_fingers);
	finger_height = tmp;

	SATAN_ERROR("PatternEditor finger_height: %d\n", (int)finger_height);

	// get document parameters
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.get_viewport(document_size);

	SATAN_DEBUG("::on_resize(%f / %f => %f, %f / %f => %f\n",
		    finger_width, document_size.width, finger_width / document_size.width,
		    finger_height, document_size.height, finger_height / document_size.height
		);

	// resize the backdrop
	newPieceIndicator.set_rect_coords(0, 0, canvas_w, canvas_h);

	// transform the keyboard
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.scale(
		finger_width / document_size.width,

		// we assume that there are 108 keys in the keyboard graphic....
		108.0 * finger_height / document_size.height
		);

	auto keyboard = KammoGUI::GnuVGCanvas::ElementReference(this, "keyboard");
	keyboard.set_transform(transform_t);
}

void PatternEditor::on_render() {
}

void PatternEditor::hide() {
	if(singleton) {
		singleton->ri_seq->drop_pattern_listener(singleton->shared_from_this());
		singleton->ri_seq.reset();

		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("none");

		singleton->on_exit_pattern_editor();
	}
}

void PatternEditor::show(std::function<void()> _on_exit_pattern_editor,
			 std::shared_ptr<RISequence> ri_seq,
			 int pattern_start_position,
			 uint32_t pattern_id) {
	if(singleton) {
		singleton->on_exit_pattern_editor = _on_exit_pattern_editor;
		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("inline");

		singleton->ri_seq = ri_seq;
		singleton->pattern_id = pattern_id;
		singleton->pattern_start_position = pattern_start_position;
		ri_seq->add_pattern_listener(pattern_id, singleton->shared_from_this());
	}
}

void PatternEditor::note_added(uint32_t pattern_id, const RINote &note) {
	KammoGUI::run_on_GUI_thread(
		[this, note]() {
			delete_note_graphic(note);
			new_note_graphic(note);
		}
		);
}

void PatternEditor::note_deleted(uint32_t pattern_id, const RINote &note) {
	KammoGUI::run_on_GUI_thread(
		[this, note]() {
			delete_note_graphic(note);
		}
		);
}

void PatternEditor::pattern_deleted(uint32_t pattern_id) {
	KammoGUI::run_on_GUI_thread(
		[this]() {
			PatternEditor::hide();
		}
		);
}
