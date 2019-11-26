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

#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

PatternEditor* PatternEditor::singleton = nullptr;
static TapDetector tap_detector;
static PatternEditorMenu* pattern_editor_menu = nullptr;

/*******************************************************************************
 *
 *      PatternEditorMenu
 *
 *******************************************************************************/

PatternEditorMenu::PatternEditorMenu(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/testTextABC.svg"), cnvs)
{
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
}

PatternEditorMenu::~PatternEditorMenu() {
}

void PatternEditorMenu::on_render() {
}

void PatternEditorMenu::on_resize() {
	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	canvas_height_fingers = (int)tmp;

	tmp = canvas_w / ((double)canvas_width_fingers);
	finger_width = tmp;
	tmp = canvas_h / ((double)canvas_height_fingers);
	finger_height = tmp;

	// get data
	root.get_viewport(document_size);
	scaling = (3.0 * finger_height) / document_size.height;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate((canvas_width_fingers - 3) * finger_width, (canvas_height_fingers - 6) * finger_height);
	root.set_transform(transform_t);

}

void PatternEditorMenu::prepare_menu(KammoGUI::GnuVGCanvas* cnvs) {
	if(pattern_editor_menu) return;

	pattern_editor_menu = new PatternEditorMenu(cnvs);
}

/*******************************************************************************
 *
 *      PatternEditor
 *
 *******************************************************************************/

void PatternEditor::note_on(int index) {
	char midi_data[] = {
		0x90, (char)(index), 0x7f
	};
	if(ri_seq) ri_seq->enqueue_midi_data(3, midi_data);
};

void PatternEditor::note_off(int index) {
	char midi_data[] = {
		0x80, (char)(index), 0x7f
	};
	if(ri_seq) ri_seq->enqueue_midi_data(3, midi_data);
};

void PatternEditor::on_backdrop_event(const KammoGUI::MotionEvent &event) {
	event_current_x = event.get_x();
	event_current_y = event.get_y() - finger_height / 2.0;

	finger_position = (int)floor(event_current_y / finger_height);
	auto current_tone = ((int)floor(pianoroll_offset)) + canvas_height_fingers - finger_position - 1;

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
		new_tone = current_tone;
		note_on(new_tone);
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		SATAN_DEBUG("finger_position: %f\n", event_current_x);
		if(new_tone != current_tone) {
			note_off(new_tone);
			new_tone = current_tone;
			note_on(new_tone);
		}
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		note_off(new_tone);
		SATAN_DEBUG("new_tone: %d (%f -> %f)\n", new_tone, event_left_x, event_right_x);

		if(start_at_sequence_position != stop_at_sequence_position) {
			ri_seq->add_note(
				pattern_id,
				0, 0, 0x7f,
				new_tone,
				start_at_sequence_position,
				stop_at_sequence_position - start_at_sequence_position - (MACHINE_TICKS_PER_LINE >> 1)
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

	start_at_sequence_position = timelines->get_sequence_line_position_at(event_left_x) * MACHINE_TICKS_PER_LINE;
	stop_at_sequence_position = timelines->get_sequence_line_position_at(event_right_x) * MACHINE_TICKS_PER_LINE;

	if(start_at_sequence_position < 0)
		start_at_sequence_position = 0;
	if(stop_at_sequence_position < 0)
		stop_at_sequence_position = 0;
	if(start_at_sequence_position > stop_at_sequence_position) {
		auto t = start_at_sequence_position;
		start_at_sequence_position = stop_at_sequence_position;
		stop_at_sequence_position = t;
	}
	if(stop_at_sequence_position - start_at_sequence_position == 0)
		stop_at_sequence_position +=  MACHINE_TICKS_PER_LINE;

	SATAN_DEBUG("Forced - Start: %d - Stop: %d\n",
		    start_at_sequence_position,
		    stop_at_sequence_position);

	auto root_element = backdrop_reference.get_root();
	auto new_piece_indicator = root_element.find_child_by_class("newPieceIndicator");

	new_piece_indicator.set_display(display_action ? "inline" : "none");
	if(display_action) {
		double offset = pianoroll_offset - floor(pianoroll_offset);
		auto stt = start_at_sequence_position;
		auto stp = stop_at_sequence_position;
		auto timelines_offset = timelines->get_graphics_horizontal_offset();
		auto timelines_minor_spacing = timelines->get_horizontal_pixels_per_line() / (double)MACHINE_TICKS_PER_LINE;
		double lft = timelines_minor_spacing * stt + timelines_offset;
		double rgt = timelines_minor_spacing * stp + timelines_offset;

		SATAN_DEBUG("timeline_offset: %f\n", timelines_offset);
		SATAN_DEBUG("b_x: %f, e_x: %f\n", lft, rgt);
		double top = (finger_position + offset) * finger_height;
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

	auto finger_coord = (int)floor(event_current_y / finger_height);
	auto new_tone_index = ((int)floor(pianoroll_offset)) + canvas_height_fingers - finger_coord - 1;

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		event_start_x = event_current_x;
		event_start_y = event_current_y;

		pianoroll_tone = new_tone_index;
		note_on(pianoroll_tone);

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
		note_off(pianoroll_tone);
	case KammoGUI::MotionEvent::ACTION_UP:
		note_off(pianoroll_tone);
		break;
	}
}

void PatternEditor::on_timelines_scroll(double minor_spacing, double horizontal_offset,
					int min_visible_offset,
					int max_visible_offset) {
	refresh_note_graphics();
}

void PatternEditor::on_single_note_event(RINote selected_note,
					 KammoGUI::GnuVGCanvas::ElementReference *e_ref,
					 const KammoGUI::MotionEvent &event) {
	bool not_tapped = true;
	if(tap_detector.analyze_events(event)) {
		SATAN_DEBUG("   TAP TAP TAP...\n");
		note_graphics[selected_note].selected = !(note_graphics[selected_note].selected);
		e_ref->set_style(note_graphics[selected_note].selected ? "fill:#ffff00" : "fill:#ff00ff");
		not_tapped = false;
	}
	event_current_x = event.get_x();
	event_current_y = event.get_y() - finger_height / 2.0;

	SATAN_DEBUG("on_single_note_event().\n");

	finger_position = (int)floor(event_current_y / finger_height);
	auto current_tone = ((int)floor(pianoroll_offset)) + canvas_height_fingers - finger_position - 1;

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
		new_tone = current_tone;
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		SATAN_DEBUG("finger_position: %f\n", event_current_x);
		if(new_tone != current_tone) {
			note_off(new_tone);
			new_tone = current_tone;
			note_on(new_tone);
		}
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		note_off(new_tone);
		SATAN_DEBUG("new_tone: %d (%f -> %f)\n", new_tone, event_left_x, event_right_x);

		if(not_tapped && (start_at_sequence_position != stop_at_sequence_position)) {
			ri_seq->delete_note(
				pattern_id,
				selected_note
				);
			ri_seq->add_note(
				pattern_id,
				0, 0, 0x7f,
				new_tone,
				start_at_sequence_position,
				stop_at_sequence_position - start_at_sequence_position
				);
		}

		display_action = false;
		break;
	}

	// Calculate the note drag horizontal offset
	start_at_sequence_position = timelines->get_sequence_line_position_at(event_start_x) * MACHINE_TICKS_PER_LINE;
	stop_at_sequence_position = timelines->get_sequence_line_position_at(event_current_x) * MACHINE_TICKS_PER_LINE;
	auto note_on_offset = stop_at_sequence_position - start_at_sequence_position;

	// set start/stop positions using selected_note, plus offset
	start_at_sequence_position = selected_note.on_at + note_on_offset;
	if(start_at_sequence_position < 0)
		start_at_sequence_position = 0;
	stop_at_sequence_position = start_at_sequence_position + selected_note.length;

	SATAN_DEBUG("Forced - Start: %d - Stop: %d\n",
		    start_at_sequence_position,
		    stop_at_sequence_position);

	auto root_element = backdrop_reference.get_root();
	auto new_piece_indicator = root_element.find_child_by_class("newPieceIndicator");

	new_piece_indicator.set_display(display_action ? "inline" : "none");
	e_ref->set_display(display_action ? "none" : "inline");
	if(display_action) {
		double offset = pianoroll_offset - floor(pianoroll_offset);
		auto stt = start_at_sequence_position;
		auto stp = stop_at_sequence_position;
		auto timelines_offset = timelines->get_graphics_horizontal_offset();
		auto timelines_minor_spacing = timelines->get_horizontal_pixels_per_line() / (double)MACHINE_TICKS_PER_LINE;
		double lft = timelines_minor_spacing * stt + timelines_offset;
		double rgt = timelines_minor_spacing * stp + timelines_offset;

		SATAN_DEBUG("timeline_offset: %f\n", timelines_offset);
		SATAN_DEBUG("b_x: %f, e_x: %f\n", lft, rgt);
		double top = (finger_position + offset) * finger_height;
		new_piece_indicator.set_rect_coords(lft, top, rgt - lft, finger_height);
	}
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
		.selected = false,
		.graphic_reference = elref
	};

	note_graphics[new_note].graphic_reference.set_event_handler(
		[this, new_note](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *e_ref,
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Note graphic event...\n");
			on_single_note_event(new_note, e_ref, event);
		}
		);

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

	backdrop_reference = KammoGUI::GnuVGCanvas::ElementReference(this, "backdrop");
	backdrop_reference.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_backdrop_event(event);
			SATAN_DEBUG("End backdrop event...\n");
		}
		);

	pianorollscroll_reference = KammoGUI::GnuVGCanvas::ElementReference(this, "pianorollscroll");
	pianorollscroll_reference.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_pianorollscroll_event(event);
			SATAN_DEBUG("End backdrop event...\n");
		}
		);
	pianoroll_reference = KammoGUI::GnuVGCanvas::ElementReference(this, "pianoroll");
}

PatternEditor::~PatternEditor() {
	singleton = nullptr;
}

std::shared_ptr<PatternEditor> PatternEditor::get_pattern_editor(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> timelines) {
	PatternEditorMenu::prepare_menu(cnvs);
	if(singleton) return singleton->shared_from_this();
	return std::shared_ptr<PatternEditor>(new PatternEditor(cnvs, timelines));
}

void PatternEditor::refresh_note_graphics() {
	int lowest_note = (int)pianoroll_offset;
	int highest_note = lowest_note + canvas_height_fingers + 1;
	auto timelines_offset = timelines->get_graphics_horizontal_offset();
	auto timelines_minor_spacing = timelines->get_horizontal_pixels_per_line() / (double)MACHINE_TICKS_PER_LINE;
	double temp = -timelines_offset / timelines_minor_spacing;
	int earliest_visible_note = (int)temp;
	int last_visible_note = earliest_visible_note + canvas_w / timelines_minor_spacing;
	double offset = pianoroll_offset - floor(pianoroll_offset);

	for(auto n_grph : note_graphics) {
		auto n = n_grph.first;
		auto g = n_grph.second.graphic_reference;
		if(lowest_note <= n.note && n.note < highest_note &&
		   earliest_visible_note < (n.on_at + n.length) && n.on_at < last_visible_note) {
			g.set_display("inline");

			double stt = n.on_at;
			double stp = n.on_at + n.length;
			double lft = timelines_minor_spacing * stt + timelines_offset;
			double rgt = timelines_minor_spacing * stp + timelines_offset;

			double top = (canvas_height_fingers - (1 + n.note - lowest_note) + offset) * finger_height;
			g.set_rect_coords(lft, top, rgt - lft - 2, finger_height);
		} else {
			g.set_display("none");
		}
	}
}

void PatternEditor::clear_note_graphics() {
	for(auto n_grph : note_graphics) {
		n_grph.second.graphic_reference.drop_element();
	}
	note_graphics.clear();
}

void PatternEditor::on_resize() {
	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

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
	backdrop_reference.set_rect_coords(0, finger_height, canvas_w, canvas_h - finger_height);

	// resize the pianoroll scroll
	pianorollscroll_reference.set_rect_coords(0, 0, finger_width, canvas_h);
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
	transform_t.translate(pianoroll_horizontal_offset, -pianoroll_offset * finger_height);
	pianoroll_reference.set_transform(transform_t);
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
		singleton->clear_note_graphics();

		ri_seq->add_pattern_listener(pattern_id, singleton->shared_from_this());

		auto pianoroll_transition = new KammoGUI::SimpleAnimation(
			TRANSITION_TIME,
			[](float progress) mutable {
				singleton->pianoroll_horizontal_offset = ((double)progress - 1.0) * singleton->finger_width;
			}
		);
		singleton->start_animation(pianoroll_transition);
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
