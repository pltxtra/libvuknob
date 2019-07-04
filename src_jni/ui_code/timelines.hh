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

#include "engine_code/global_control_object.hh"

typedef RemoteInterface::ClientSpace::GlobalControlObject GCO;

class TimeLines
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public KammoGUI::ScaleGestureDetector::OnScaleGestureListener
	, public RemoteInterface::Context::ObjectSetListener<GCO>
	, public GCO::GlobalControlListener
	, public std::enable_shared_from_this<TimeLines>
{
public:
	typedef std::function<void(int loop_start, int loop_stop)> LoopSettingCallback;

private:
	// fling detector
	KammoGUI::FlingGestureDetector fling_detector;

	KammoGUI::GnuVGCanvas::ElementReference *timeline_container = 0;
	KammoGUI::GnuVGCanvas::ElementReference *time_index_container = 0;

	// Stuff for loop data
	enum ModifyingLoop {
		neither_start_or_stop,
		start_marker,
		stop_marker
	};
	std::weak_ptr<GCO> gco_w;
	KammoGUI::GnuVGCanvas::ElementReference loop_marker;
	KammoGUI::GnuVGCanvas::ElementReference loop_start_marker;
	KammoGUI::GnuVGCanvas::ElementReference loop_stop_marker;
	int lpb = 3, loop_start = 0, loop_length = 16, new_marker_position;
	ModifyingLoop currently_modifying = neither_start_or_stop;
	void on_loop_marker_event(ModifyingLoop selected_marker, const KammoGUI::MotionEvent &event);
	void request_new_loop_settings(int new_loop_start, int new_loop_stop);

	// sizes in pixels
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	double finger_width = 10.0, finger_height = 10.0, scaling = 1.0;

	// canvas size in "fingers"
	int canvas_width_fingers = 8, canvas_height_fingers = 8;
	int font_size = 55; // in pixels
	double scroll_start_x = 0;
	bool ignore_scroll = false;
	float canvas_w_inches, canvas_h_inches;
	int minors_per_major, sequence_lines_per_minor;
	// sequence_line_width means how wide a line in the sequencer is, not a visible time line.
	// A time line may represent one or more sequencer lines, depending on zoom etc.
	double horizontal_zoom_factor = 1.0, line_offset = 0.0, default_sequence_line_width, zoomed_sequence_line_width;
	int canvas_w, canvas_h;
	std::vector<KammoGUI::GnuVGCanvas::ElementReference *> majors;
	std::vector<KammoGUI::GnuVGCanvas::ElementReference *> minors;
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

	typedef std::function<void(double, double, int, int)> ScrollCallback;
	std::vector<ScrollCallback> scroll_callbacks;

public:
	TimeLines(KammoGUI::GnuVGCanvas* cnvs);
	~TimeLines();

	void add_scroll_callback(std::function<void(double, double, int, int)>);
	void call_scroll_callbacks();

	void show_loop_markers();
	void hide_loop_markers();

	double get_graphics_horizontal_offset();
	double get_horizontal_pixels_per_line();
	int get_sequence_line_position_at(int horizontal_pixel_value);

	void set_prefix_string(const std::string &prefix);

	virtual void on_resize() override;
	virtual void on_render() override;

	virtual void loop_start_changed(int new_start) override;
	virtual void loop_length_changed(int new_length) override;
	virtual void lpb_changed(int new_lpb) override;
	virtual void object_registered(std::shared_ptr<GCO> gco) override;
	virtual void object_unregistered(std::shared_ptr<GCO> gco) override;
};

#endif
