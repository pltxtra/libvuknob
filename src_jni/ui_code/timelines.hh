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

#ifndef VUKNOB_TIMELINES
#define VUKNOB_TIMELINES

#include <gnuVGcanvas.hh>
#include <kamogui_scale_detector.hh>
#include <kamogui_fling_detector.hh>

#include "machine_sequencer.hh"

class TimeLines
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public KammoGUI::ScaleGestureDetector::OnScaleGestureListener
{
private:
	// fling detector
	KammoGUI::FlingGestureDetector fling_detector;

	KammoGUI::GnuVGCanvas::ElementReference *timeline_container = 0;
	KammoGUI::GnuVGCanvas::ElementReference *time_index_container = 0;

	// sizes in pixels
	double finger_width = 10.0, finger_height = 10.0;

	// canvas size in "fingers"
	int canvas_width_fingers = 8, canvas_height_fingers = 8;
	int font_size = 55; // in pixels
	double scroll_start_x = 0, scroll_start_y = 0;
	bool ignore_scroll = false;
	unsigned int skip_interval;
	float canvas_w_inches, canvas_h_inches;
	double horizontal_zoom_factor = 1.0, line_offset = 0.0;
	int minor_width, canvas_w, canvas_h;
	int minors_per_major = 16;
	std::vector<KammoGUI::GnuVGCanvas::ElementReference *> major_n_minors;
	std::string prefix_string = "1:";

	// for calculating sequencer positions from pixel offsets
	double minor_spacing, line_offset_d;

	// clear/generate graphics
	void create_timelines();
	void clear_timelines();
	void clear_time_index();
	void create_time_index();
	void regenerate_graphics(); // this calls the others

	void scrolled_horizontal(float pixels_changed);
	void on_time_index_event(const KammoGUI::MotionEvent &event);

	// BEGIN scale detector
	KammoGUI::ScaleGestureDetector *sgd;
	virtual bool on_scale(KammoGUI::ScaleGestureDetector *detector);
	virtual bool on_scale_begin(KammoGUI::ScaleGestureDetector *detector);
	virtual void on_scale_end(KammoGUI::ScaleGestureDetector *detector);
	// END scale detector

	struct CallbackContainer {
		std::function<void(double, double, int, int)> cb;
		CallbackContainer(std::function<void(double, double, int, int)>_cb) : cb(_cb) {}
	};

	std::set<std::shared_ptr<CallbackContainer> > scroll_callbacks;

	void call_scroll_callbacks() {
		auto min_visible_offset = get_sequence_minor_position_at(0);
		auto max_visible_offset = get_sequence_minor_position_at(canvas_w);

		for(auto cbc : scroll_callbacks)
			cbc->cb(minor_spacing, line_offset, min_visible_offset, max_visible_offset);
	}

public:
	TimeLines(KammoGUI::GnuVGCanvas* cnvs);
	~TimeLines();

	void add_scroll_callback(std::function<void(double, double, int, int)>);

	int get_sequence_minor_position_at(int horizontal_pixel_value);
	int get_minor_spacing() {
		return minor_spacing;
	}

	void set_minor_steps(int minor_steps_per_major);
	void set_prefix_string(const std::string &prefix);

	virtual void on_resize() override;
	virtual void on_render() override;

};

#endif
