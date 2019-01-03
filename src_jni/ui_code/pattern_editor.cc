/*
 * vu|KNOB
 * (C) 2018 by Anton Persson
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
#include "fling_animation.hh"

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

		finger_position = (int)floor(event_current_y / finger_height);
		new_tone = ((int)floor(pianoroll_offset)) + canvas_height_fingers - finger_position - 1;

		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		SATAN_DEBUG("finger_position: %f\n", event_current_x);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		SATAN_DEBUG("new_tone: %d (%f -> %f)\n", new_tone, event_left_x, event_right_x);

		if(start_at_sequence_position != stop_at_sequence_position) {
			ri_seq->add_note(
				pattern_id,
				0, 0, 0xff,
				new_tone,
				start_at_sequence_position,
				stop_at_sequence_position - start_at_sequence_position
				);
		}

		display_action = false;
		break;
	}

	if(event_current_x < event_start_x) {
		event_left_x = event_current_x;
		event_right_x = event_start_x;
	} else {
		event_left_x = event_start_x;
		event_right_x = event_current_x;
	}

	start_at_sequence_position = timelines->get_sequence_minor_position_at(event_left_x);
	stop_at_sequence_position = timelines->get_sequence_minor_position_at(event_right_x);

	if(start_at_sequence_position < 0)
		start_at_sequence_position = 0;
	if(stop_at_sequence_position < 0)
		stop_at_sequence_position = 0;

	start_at_sequence_position = (start_at_sequence_position >> 4) << 4;
	stop_at_sequence_position = 16 + ((stop_at_sequence_position >> 4) << 4);

	SATAN_DEBUG("Forced - Start: %d - Stop: %d\n",
		    start_at_sequence_position,
		    stop_at_sequence_position);

	auto root_element = backdrop_reference->get_root();
	auto new_piece_indicator = root_element.find_child_by_class("newPieceIndicator");

	new_piece_indicator.set_display(display_action ? "inline" : "none");
	if(display_action) {
		auto stt = start_at_sequence_position;
		auto stp = stop_at_sequence_position;
		auto timelines_offset = timelines->get_graphics_horizontal_offset();
		auto timelines_minor_spacing = timelines->get_horizontal_pixels_per_minor();
		double lft = timelines_minor_spacing * stt + timelines_offset;
		double rgt = timelines_minor_spacing * stp + timelines_offset;

		SATAN_DEBUG("timeline_offset: %f\n", timelines_offset);
		SATAN_DEBUG("b_x: %f, e_x: %f\n", lft, rgt);
		double top = finger_position * finger_height;
		new_piece_indicator.set_rect_coords(lft, top, rgt - lft, finger_height);
	}

}

void PatternEditor::pianoroll_scrolled_vertical(float pixels_changed) {
	pianoroll_offset += ((double)pixels_changed) / finger_height;
	if(pianoroll_offset < 0.0) pianoroll_offset = 0.0;
	if(pianoroll_offset > 127.0) pianoroll_offset = 127.0;
	SATAN_DEBUG("pianoroll offset: %f\n", pianoroll_offset);
	refresh_note_graphics();
}

void PatternEditor::on_pianorollscroll_event(const KammoGUI::MotionEvent &event) {
	if(fling_detector.on_touch_event(event)) {
		SATAN_DEBUG("fling detected.\n");
		float speed_x, speed_y;
		float abs_speed_x, abs_speed_y;

		fling_detector.get_speed(speed_x, speed_y);
		fling_detector.get_absolute_speed(abs_speed_x, abs_speed_y);

		bool do_horizontal_fling = abs_speed_x > abs_speed_y ? true : false;

		float speed = 0.0f, abs_speed;
		std::function<void(float pixels_changed)> scrolled;

		if(!do_horizontal_fling) {
			abs_speed = abs_speed_y;
			speed = speed_y;
			scrolled = [this](float pixels_changed){
				pianoroll_scrolled_vertical(pixels_changed);
				SATAN_DEBUG("Scrolled vertical: %f\n", pixels_changed);
			};

			float fling_duration = abs_speed / FLING_DEACCELERATION;

			FlingAnimation *flinganim =
				new FlingAnimation(speed, fling_duration, scrolled);
			start_animation(flinganim);
		}
	}

	event_current_x = event.get_x();
	event_current_y = event.get_y();

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		event_start_x = event_current_x;
		event_start_y = event_current_y;

		SATAN_DEBUG("event_start_x: %f\n", event_start_x);
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		{
			auto diff = event_current_y - event_start_y;
			pianoroll_scrolled_vertical(diff);
		}
		event_start_x = event_current_x;
		event_start_y = event_current_y;
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		break;
	}
}

void PatternEditor::on_timelines_scroll(double minor_spacing, double horizontal_offset,
					int min_visible_offset,
					int max_visible_offset) {
	refresh_note_graphics();
}

void PatternEditor::create_note_graphic(const RINote &new_note) {
	SATAN_DEBUG("create_note_graphic()\n");

	if(note_graphics.find(new_note) != note_graphics.end()) {
		SATAN_DEBUG("Cannot insert graphic, element already exists.");
		return;
	}

	auto new_id = note_graphics_id_allocator.get_id();

	std::stringstream ss_new_id;
	ss_new_id << "note_instance_" << new_id;

	std::stringstream ss;

	ss << "<rect "
	   << "style=\"fill:#ff00ff;fill-opacity:1\" "
	   << "id=\"" << ss_new_id.str() << "\" "
	   << "width=\"500\" "
	   << "height=\"100\" "
	   << "x=\"0.0\" "
	   << "y=\"400.0\" />"
/*
			   << "width=\"" << 5000 << "\" "
	   << "height=\"" << 5000 << "\" "
	   << "x=\"" << (0) << "\" "
	   << "y=\"0.0\" />"
*/
		;
	SATAN_DEBUG("New note: %d (%d -> %d)\n", new_note.note, new_note.on_at, new_note.length);
	SATAN_DEBUG("Inserting graphic: %s", ss.str().c_str());
	auto note_container = KammoGUI::GnuVGCanvas::ElementReference(this, "noteContainer");
	note_container.add_svg_child(ss.str());
	SATAN_DEBUG("Graphic inserted: %s", ss.str().c_str());
	KammoGUI::GnuVGCanvas::ElementReference elref(&note_container, ss_new_id.str());

	note_graphics[new_note] = NoteGraphic {
		.id = new_id,
		.note = new_note,
		.graphic_reference = elref
	};

	refresh_note_graphics();
}

void PatternEditor::delete_note_graphic(const RINote &note) {
	SATAN_DEBUG("delete_note_graphic()\n");

	auto found_element = note_graphics.find(note);
	if(found_element != note_graphics.end()) {
		auto v = found_element->second;
		SATAN_DEBUG("Will delete note graphic for: %d (%d -> %d)\n",
			    v.note.note,
			    v.note.on_at,
			    v.note.length);
		v.graphic_reference.drop_element();
		note_graphics.erase(found_element);
	}
}

PatternEditor::PatternEditor(KammoGUI::GnuVGCanvas* cnvs,
			     std::shared_ptr<TimeLines> _timelines)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/VerticalKeyboard.svg"), cnvs)
	, timelines(_timelines)
{
	timelines->add_scroll_callback([this](double minor_spacing, double horizontal_offset,
					      int min_visible_offset,
					      int max_visible_offset) {
					       on_timelines_scroll(minor_spacing,
								   horizontal_offset,
								   min_visible_offset,
								   max_visible_offset);
				       });
	singleton = this;

	backdrop_reference = new KammoGUI::GnuVGCanvas::ElementReference(this, "backdrop");
	backdrop_reference->set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_backdrop_event(event);
			SATAN_DEBUG("End backdrop event...\n");
		}
		);

	pianorollscroll_reference = new KammoGUI::GnuVGCanvas::ElementReference(this, "pianorollscroll");
	pianorollscroll_reference->set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_pianorollscroll_event(event);
			SATAN_DEBUG("End backdrop event...\n");
		}
		);
	pianoroll_reference = new KammoGUI::GnuVGCanvas::ElementReference(this, "pianoroll");
}

PatternEditor::~PatternEditor() {
	singleton = nullptr;
}

void PatternEditor::refresh_note_graphics() {
	int lowest_note = (int)pianoroll_offset;
	int highest_note = lowest_note + canvas_height_fingers + 1;
	auto timelines_offset = timelines->get_graphics_horizontal_offset();
	auto timelines_minor_spacing = timelines->get_horizontal_pixels_per_minor();
	double temp = -timelines_offset / timelines_minor_spacing;
	int earliest_visible_note = (int)temp;
	int last_visible_note = earliest_visible_note + canvas_width_fingers * 16;
	double offset = pianoroll_offset - floor(pianoroll_offset);

	for(auto n_grph : note_graphics) {
		auto n = n_grph.first;
		auto g = n_grph.second.graphic_reference;
		if(lowest_note <= n.note && n.note < highest_note &&
		   earliest_visible_note < (n.on_at + n.length) && n.on_at < last_visible_note) {
			g.set_display("inline");

			auto stt = n.on_at;
			auto stp = n.on_at + n.length;
			double lft = timelines_minor_spacing * stt + timelines_offset;
			double rgt = timelines_minor_spacing * stp + timelines_offset;

			double top = (canvas_height_fingers - (1 + n.note - lowest_note) + offset) * finger_height;
			g.set_rect_coords(lft, top, rgt - lft, finger_height);
		} else {
			g.set_display("none");
		}
	}
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
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.get_viewport(document_size);

	SATAN_DEBUG("::on_resize(%f / %f => %f, %f / %f => %f\n",
		    finger_width, document_size.width, finger_width / document_size.width,
		    finger_height, document_size.height, finger_height / document_size.height
		);

	// resize the backdrop
	backdrop_reference->set_rect_coords(0, finger_height, canvas_w, canvas_h - finger_height);

	// resize the pianoroll scroll
	pianorollscroll_reference->set_rect_coords(0, 0, finger_width, canvas_h);
}

void PatternEditor::on_render() {
	// transform the pianoroll
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.scale(
		finger_width / document_size.width,

		// we assume that there are 108 keys in the keyboard graphic....
		108.0 * finger_height / document_size.height
		);
	transform_t.translate(0.0, -pianoroll_offset * finger_height);
	pianoroll_reference->set_transform(transform_t);
}

void PatternEditor::hide() {
	if(singleton) {
		if(singleton->ri_seq) {
			singleton->ri_seq->drop_pattern_listener(singleton->shared_from_this());
			singleton->ri_seq.reset();
		}
		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("none");

		if(singleton->on_exit_pattern_editor)
			singleton->on_exit_pattern_editor();
	}
}

void PatternEditor::show(std::function<void()> _on_exit_pattern_editor,
			 std::shared_ptr<RISequence> ri_seq,
			 uint32_t pattern_id) {
	if(singleton && ri_seq) {
		singleton->on_exit_pattern_editor = _on_exit_pattern_editor;
		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("inline");

		singleton->ri_seq = ri_seq;
		singleton->pattern_id = pattern_id;
		ri_seq->add_pattern_listener(pattern_id, singleton->shared_from_this());
	}
}

void PatternEditor::note_added(uint32_t pattern_id, const RINote &note) {
	KammoGUI::run_on_GUI_thread(
		[this, note]() {
			delete_note_graphic(note);
			create_note_graphic(note);
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