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

struct NoteGraphic {
	uint32_t id;
	RINote note;
	KammoGUI::GnuVGCanvas::ElementReference graphic_reference;
};

class PatternEditor
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public RISequence::PatternListener
	, public std::enable_shared_from_this<PatternEditor>
{
private:
	static PatternEditor *singleton;

	KammoGUI::FlingGestureDetector fling_detector;

	KammoGUI::GnuVGCanvas::ElementReference *backdrop_reference = 0;
	KammoGUI::GnuVGCanvas::ElementReference *pianorollscroll_reference = 0;
	KammoGUI::GnuVGCanvas::ElementReference *pianoroll_reference = 0;
	uint32_t pattern_id;
	std::shared_ptr<RISequence> ri_seq;
	std::shared_ptr<TimeLines> timelines;
	std::function<void()> on_exit_pattern_editor;

	double pianoroll_offset = 0.0;
	int finger_position, new_tone;
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

	void pianoroll_scrolled_vertical(float pixels_changed);
	void refresh_note_graphics();
	void create_note_graphic(const RINote &note);
	void delete_note_graphic(const RINote &note);

	void on_backdrop_event(const KammoGUI::MotionEvent &event);
	void on_pianorollscroll_event(const KammoGUI::MotionEvent &event);
	void on_timelines_scroll(double minor_spacing, double horizontal_offset,
				 int min_visible_offset,
				 int max_visible_offset);
public:
	PatternEditor(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> timelines);
	~PatternEditor();

	virtual void on_resize() override;
	virtual void on_render() override;

	static void hide();
	static void show(std::function<void()> on_exit_pattern_editor,
			 std::shared_ptr<RISequence> ri_seq,
			 uint32_t pattern_id);

	virtual void note_added(uint32_t pattern_id, const RINote &note);
	virtual void note_deleted(uint32_t pattern_id, const RINote &note);
	virtual void pattern_deleted(uint32_t pattern_id);
};

#endif