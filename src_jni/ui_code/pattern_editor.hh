/*
 * vu|KNOB
 * Copyright (C) 2018 by Anton Persson
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

#ifndef VUKNOB_PATTERN_EDITOR
#define VUKNOB_PATTERN_EDITOR

#include <gnuVGcanvas.hh>
#include <kamogui_fling_detector.hh>

#include "engine_code/sequence.hh"
#include "../common.hh"

#include "timelines.hh"

typedef RemoteInterface::ClientSpace::Sequence RISequence;
typedef RemoteInterface::ClientSpace::Sequence::Note RINote;

struct NoteGraphic_s {
	uint32_t id;
	RINote note;
	bool selected;
	KammoGUI::GnuVGCanvas::ElementReference graphic_reference;
};

typedef std::shared_ptr<NoteGraphic_s> NoteGraphic;

class PatternEditorMenu
	: public KammoGUI::GnuVGCanvas::SVGDocument
{
private:
	enum MenuMode {
		no_mode,
		gridops_mode,
		patops_mode,
		memops_mode
	};
	MenuMode current_menu_mode = no_mode;

	PatternEditorMenu(KammoGUI::GnuVGCanvas* cnvs);
	~PatternEditorMenu();

	KammoGUI::GnuVGCanvas::ElementReference gridops_container, patops_container, memops_container;
	KammoGUI::GnuVGCanvas::ElementReference deselect_button, gridops_button, patops_button, memops_button;
	KammoGUI::GnuVGCanvas::ElementReference transpose_one_up_button, transpose_octave_up_button, transpose_one_left_button;
	KammoGUI::GnuVGCanvas::ElementReference transpose_one_down_button, transpose_octave_down_button, transpose_one_right_button;
	KammoGUI::GnuVGCanvas::ElementReference copy_button, paste_button, delete_button;
	KammoGUI::GnuVGCanvas::ElementReference pattern_id_plus_button, pattern_id_minus_button;
	KammoGUI::GnuVGCanvas::ElementReference deselect_text, pattern_id_text;

	KammoGUI::GnuVGCanvas::ElementReference root;
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	double scaling; // graphical scaling factor
	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

public:

	virtual void on_resize() override;
	virtual void on_render() override;

	static void prepare_menu(KammoGUI::GnuVGCanvas* cnvs);
	static void show();
	static void hide();
	static void set_deselectable_count(int k);
	static void set_pattern_id(uint32_t pattern_id);
};

class PatternEditor
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public RISequence::PatternListener
	, public std::enable_shared_from_this<PatternEditor>
{
public:
	enum PatternEditorOperation {
		deselect_all_notes,
		transpose_one_up, transpose_octave_up, transpose_one_left,
		transpose_one_down, transpose_octave_down, transpose_one_right,
		copy_notes, paste_notes, delete_notes,
		pattern_id_plus,
		pattern_id_minus
	};
private:
	static PatternEditor *singleton;

	KammoGUI::FlingGestureDetector fling_detector;

	KammoGUI::GnuVGCanvas::ElementReference backdrop_reference;
	KammoGUI::GnuVGCanvas::ElementReference pianorollscroll_reference;
	KammoGUI::GnuVGCanvas::ElementReference pianoroll_reference;
	uint32_t pattern_id;
	std::shared_ptr<RISequence> ri_seq;
	std::shared_ptr<TimeLines> timelines;
	std::function<void()> on_exit_pattern_editor;

	double pianoroll_offset = 0.0, pianoroll_horizontal_offset = 0;
	int finger_position, new_tone, pianoroll_tone;
	double event_left_x, event_right_x;
	double event_start_x, event_start_y;
	double event_current_x, event_current_y;
	int start_at_sequence_position, stop_at_sequence_position;
	bool display_action = false;

	// sizes in pixels
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	double finger_width = 10.0, finger_height = 10.0;

	// canvas size in "fingers"
	int canvas_width_fingers = 8, canvas_height_fingers = 8;
	float canvas_w_inches, canvas_h_inches;
	int canvas_w, canvas_h;

	IDAllocator note_graphics_id_allocator;
	std::map<RINote, NoteGraphic> note_graphics;
	std::set<RINote> clipboard;
	std::set<RINote> expected_new_selection;

	void update_selected_notes_counter();

	// Operations
	void deselect_all();
	void transpose_selected_notes(PatternEditorOperation p_operation);
	void copy_selected_notes_to_clipboard();
	void paste_notes_from_clipboard();
	void delete_selected_notes();

	std::set<RINote> get_selection(bool all_if_selection_is_empty);

	void select(NoteGraphic &ngph);
	void deselect(NoteGraphic &ngph);

	void show_velocity_slider(NoteGraphic &ngph);

	void note_on(int index);
	void note_off(int index);

	void pianoroll_scrolled_vertical(float pixels_changed);
	void refresh_note_graphics();
	void clear_note_graphics();
	void create_note_graphic(const RINote &note);
	void delete_note_graphic(const RINote &note);

	void on_backdrop_event(const KammoGUI::MotionEvent &event);
	void on_pianorollscroll_event(const KammoGUI::MotionEvent &event);
	void on_timelines_scroll(double minor_spacing, double horizontal_offset,
				 int min_visible_offset,
				 int max_visible_offset);
	void on_single_note_event(RINote selected_note,
				  KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				  const KammoGUI::MotionEvent &event);

	void internal_perform_operation(PatternEditorOperation p_operation);

	void cleanup_pattern_listening();
	void use_sequence_and_pattern(std::shared_ptr<RISequence> ri_seq, uint32_t pattern_id);

public:
	PatternEditor(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> timelines);
	~PatternEditor();

	void use_context(std::shared_ptr<RISequence> ri_seq, uint32_t pattern_id);
	void hide(bool hide_timelines = true);
	void show();

	virtual void on_resize() override;
	virtual void on_render() override;

	static void perform_operation(PatternEditorOperation p_operation);

	static std::shared_ptr<PatternEditor> get_pattern_editor(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> timelines);

	virtual void note_added(uint32_t pattern_id, const RINote &note);
	virtual void note_deleted(uint32_t pattern_id, const RINote &note);
	virtual void pattern_deleted(uint32_t pattern_id);
};

#endif
