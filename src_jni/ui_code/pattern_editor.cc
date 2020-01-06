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
#include "popup_window.hh"

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
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/patternEditorMenu.svg"), cnvs)
{
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

	gridops_container = KammoGUI::GnuVGCanvas::ElementReference(this, "gridOperationsContainer");
	patops_container = KammoGUI::GnuVGCanvas::ElementReference(this, "patternIdOperationsContainer");
	memops_container = KammoGUI::GnuVGCanvas::ElementReference(this, "memoryOperationsContainer");

	gridops_container.set_display("none");
	patops_container.set_display("none");
	memops_container.set_display("none");

	deselect_button = KammoGUI::GnuVGCanvas::ElementReference(this, "deselectButton");
	deselect_button.set_display("none");
	deselect_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				PatternEditor::perform_operation(PatternEditor::deselect_all_notes);
			}
		}
		);
	deselect_text = KammoGUI::GnuVGCanvas::ElementReference(this, "deselectText");

	gridops_button = KammoGUI::GnuVGCanvas::ElementReference(this, "gridOperationsButton");
	patops_button = KammoGUI::GnuVGCanvas::ElementReference(this, "patternIdButton");
	memops_button = KammoGUI::GnuVGCanvas::ElementReference(this, "memoryOperationsButton");

	gridops_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("gridops_button pressed...\n");
			if(tap_detector.analyze_events(event)) {
				gridops_container.set_display("none");
				patops_container.set_display("none");
				memops_container.set_display("none");
				if(current_menu_mode != gridops_mode) {
					current_menu_mode = gridops_mode;
					gridops_container.set_display("inline");
				} else {
					current_menu_mode = no_mode;
				}
			}
		}
		);
	patops_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("patops_button pressed...\n");
			if(tap_detector.analyze_events(event)) {
				gridops_container.set_display("none");
				patops_container.set_display("none");
				memops_container.set_display("none");
				if(current_menu_mode != patops_mode) {
					current_menu_mode = patops_mode;
					patops_container.set_display("inline");
				} else {
					current_menu_mode = no_mode;
				}
			}
		}
		);
	memops_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("gridops_button pressed...\n");
			if(tap_detector.analyze_events(event)) {
				gridops_container.set_display("none");
				patops_container.set_display("none");
				memops_container.set_display("none");
				if(current_menu_mode != memops_mode) {
					current_menu_mode = memops_mode;
					memops_container.set_display("inline");
				} else {
					current_menu_mode = no_mode;
				}
			}
		}
		);

	shift_one_up_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOneUp");
	shift_octave_up_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOctaveUp");
	shift_one_left_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOneLeft");
	shift_one_down_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOneDown");
	shift_octave_down_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOctaveDown");
	shift_one_right_button = KammoGUI::GnuVGCanvas::ElementReference(this, "shiftOneRight");

	copy_button = KammoGUI::GnuVGCanvas::ElementReference(this, "copyButton");
	paste_button = KammoGUI::GnuVGCanvas::ElementReference(this, "pasteButton");
	delete_button = KammoGUI::GnuVGCanvas::ElementReference(this, "deleteButton");

	pattern_id_plus_button = KammoGUI::GnuVGCanvas::ElementReference(this, "patternIdPlusButton");
	pattern_id_minus_button = KammoGUI::GnuVGCanvas::ElementReference(this, "patternIdMinusButton");
	pattern_id_text = KammoGUI::GnuVGCanvas::ElementReference(this, "patternIdText");

	pattern_id_plus_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				PatternEditor::perform_operation(PatternEditor::pattern_id_plus);
			}
		}
		);
	pattern_id_minus_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				PatternEditor::perform_operation(PatternEditor::pattern_id_minus);
			}
		}
		);
	delete_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				PatternEditor::perform_operation(PatternEditor::delete_notes);
			}
		}
		);
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
	transform_t.translate((canvas_width_fingers - 3) * finger_width, (canvas_height_fingers - 3.5) * finger_height);
	root.set_transform(transform_t);

}

void PatternEditorMenu::prepare_menu(KammoGUI::GnuVGCanvas* cnvs) {
	if(pattern_editor_menu) return;

	pattern_editor_menu = new PatternEditorMenu(cnvs);
}

void PatternEditorMenu::show() {
	pattern_editor_menu->root.set_display("inline");

	pattern_editor_menu->gridops_container.set_display("none");
	pattern_editor_menu->patops_container.set_display("none");
	pattern_editor_menu->memops_container.set_display("none");
	pattern_editor_menu->current_menu_mode = no_mode;
}

void PatternEditorMenu::hide() {
	pattern_editor_menu->root.set_display("none");
}

void PatternEditorMenu::set_deselectable_count(int selected_notes_counter) {
	if(selected_notes_counter > 0) {
		std::ostringstream stream;
		stream << "Deselect " << selected_notes_counter;
		pattern_editor_menu->deselect_button.set_display("inline");
		pattern_editor_menu->deselect_text.set_text_content(stream.str());
	} else {
		pattern_editor_menu->deselect_button.set_display("none");
	}
}

void PatternEditorMenu::set_pattern_id(uint32_t pattern_id) {
	char bfr[32];
	snprintf(bfr, 24, "%02d", pattern_id);
	pattern_editor_menu->pattern_id_text.set_text_content(bfr);
}

/*******************************************************************************
 *
 *      PatternEditor
 *
 *******************************************************************************/

void PatternEditor::update_selected_notes_counter() {
	int selected_notes_counter = 0;
	for(auto ngph : note_graphics) {
		if(ngph.second->selected)
			++selected_notes_counter;
	}
	PatternEditorMenu::set_deselectable_count(selected_notes_counter);
}

void PatternEditor::deselect_all() {
	for(auto ngph : note_graphics) {
		if(ngph.second->selected)
			deselect(ngph.second);
	}
	update_selected_notes_counter();
}

void PatternEditor::delete_selected_notes() {
	auto selection = get_selection(true);
	if(selection.size() == 0) {
		SATAN_DEBUG("PatternEditor::delete_selected_notes() on empty selection.\n");
		return;
	}

	auto delete_on_yes = [this, selection](PopupWindow::UserResponse response) {
		if(response == PopupWindow::yes) {
			for(auto selected_note : selection) {
				ri_seq->delete_note(
					pattern_id,
					selected_note
					);
			}
		}
	};

	if(selection.size() == note_graphics.size()) {
		PopupWindow::ask_yes_or_no("Do you want", "to delete", "all notes?", delete_on_yes);
	} else {
		delete_on_yes(PopupWindow::yes);
	}
}

std::set<RINote> PatternEditor::get_selection(bool all_if_selection_is_empty = false) {
	std::set<RINote> selection;
	for(auto ngph : note_graphics) {
		if(ngph.second->selected)
			selection.insert(ngph.second->note);
	}
	if(all_if_selection_is_empty && selection.size() == 0) {
		for(auto ngph : note_graphics) {
			selection.insert(ngph.second->note);
		}
	}
	return selection;
}

void PatternEditor::select(NoteGraphic &ngph) {
	SATAN_DEBUG("select() on %p\n", &ngph);
	ngph->selected = true;
	ngph->graphic_reference.set_style("fill:#ffff00");
}

void PatternEditor::deselect(NoteGraphic &ngph) {
	SATAN_DEBUG("deselect() on %p\n", &ngph);
	ngph->selected = false;
	ngph->graphic_reference.set_style("fill:#ff00ff");
}

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
		if(note_graphics[selected_note]->selected)
			deselect(note_graphics[selected_note]);
		else
			select(note_graphics[selected_note]);
		not_tapped = false;

		update_selected_notes_counter();
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

	auto ngph = std::make_shared<NoteGraphic_s>();
	ngph->id = new_id;
	ngph->note = new_note;
	ngph->selected = false;
	ngph->graphic_reference = std::move(elref);

	ngph->graphic_reference.set_event_handler(
		[this, new_note](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *e_ref,
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Note graphic event...\n");
			on_single_note_event(new_note, e_ref, event);
		}
		);

	note_graphics[new_note] = ngph;

	refresh_note_graphics();
}

void PatternEditor::delete_note_graphic(const RINote &note) {
	SATAN_DEBUG("delete_note_graphic()\n");

	auto found_element = note_graphics.find(note);
	if(found_element != note_graphics.end()) {
		auto v = found_element->second;
		SATAN_DEBUG("Will delete note graphic for: %d (%d -> %d)\n",
			    v->note.note,
			    v->note.on_at,
			    v->note.length);
		v->graphic_reference.drop_element();
		note_graphics.erase(found_element);
	}
	update_selected_notes_counter();
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

void PatternEditor::internal_perform_operation(PatternEditor::PatternEditorOperation p_operation) {
	switch(p_operation) {
	case deselect_all_notes:
		deselect_all();
		break;
	case shift_one_up:
		break;
	case shift_octave_up:
		break;
	case shift_one_left:
		break;
	case shift_one_down:
		break;
	case shift_octave_down:
		break;
	case shift_one_right:
		break;
	case copy_notes:
		break;
	case paste_notes:
		break;
	case delete_notes:
		delete_selected_notes();
		break;
	case pattern_id_plus:
		if(pattern_id < (IDAllocator::NO_ID_AVAILABLE - 1))
			use_sequence_and_pattern(ri_seq, pattern_id + 1);
		break;
	case pattern_id_minus:
		if(pattern_id > 0)
			use_sequence_and_pattern(ri_seq, pattern_id - 1);
		break;
	}
}

void PatternEditor::perform_operation(PatternEditorOperation p_operation) {
	if(singleton)
		singleton->internal_perform_operation(p_operation);
}

std::shared_ptr<PatternEditor> PatternEditor::get_pattern_editor(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> timelines) {
	if(singleton) return singleton->shared_from_this();
	auto  response = std::shared_ptr<PatternEditor>(new PatternEditor(cnvs, timelines));
	PatternEditorMenu::prepare_menu(cnvs);
	return response;
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
		auto g = n_grph.second->graphic_reference;
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
		n_grph.second->graphic_reference.drop_element();
	}
	note_graphics.clear();
	PatternEditorMenu::set_deselectable_count(0);
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
		PatternEditorMenu::hide();
		singleton->cleanup_pattern_listening();
		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("none");

		if(singleton->on_exit_pattern_editor)
			singleton->on_exit_pattern_editor();
	}
}

void PatternEditor::cleanup_pattern_listening() {
	if(ri_seq) {
		ri_seq->drop_pattern_listener(shared_from_this());
		ri_seq.reset();
	}
}

void PatternEditor::use_sequence_and_pattern(std::shared_ptr<RISequence> _ri_seq, uint32_t _pattern_id) {
	cleanup_pattern_listening();
	clear_note_graphics();
	ri_seq = _ri_seq;
	pattern_id = _pattern_id;
	ri_seq->add_pattern_listener(pattern_id, shared_from_this());
	PatternEditorMenu::set_pattern_id(pattern_id);
}

void PatternEditor::show(std::function<void()> _on_exit_pattern_editor,
			 std::shared_ptr<RISequence> ri_seq,
			 uint32_t pattern_id) {
	if(singleton && ri_seq) {
		PatternEditorMenu::show();
		singleton->on_exit_pattern_editor = _on_exit_pattern_editor;
		auto layer1 = KammoGUI::GnuVGCanvas::ElementReference(singleton, "layer1");
		layer1.set_display("inline");

		singleton->use_sequence_and_pattern(ri_seq, pattern_id);

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
