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

#include "timelines.hh"
#include "svg_loader.hh"
#include "common.hh"
#include "fling_animation.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

void TimeLines::create_timelines() {
	KammoGUI::GnuVGCanvas::ElementReference root_element(this, "timeline_group");

	{ // timeline group creation block
		std::stringstream ss;
		ss << "<svg id=\"timeline_container\" "
		   << " x=\"0\" \n"
		   << " y=\"0\" \n"
		   << " width=\"" << canvas_w << "\" \n"
		   << " height=\"" << canvas_h << "\" \n"
		   << "/>\n";
		root_element.add_svg_child(ss.str());

		timeline_container = new KammoGUI::GnuVGCanvas::ElementReference(this, "timeline_container");
	}

	{ // timeline creation block

		// "optimal" horizontal finger count
		double line_count_d = canvas_w_inches / INCHES_PER_FINGER;

		int line_count = (int)line_count_d; // line_count is now the number of VISIBLE lines
		minor_width = canvas_w / (line_count * minors_per_major);
		int line_width = canvas_w / line_count;

		// generate line_count * 2 + 2 line nodes, because of scrolling + zooming
		for(int k = 0; k < (line_count * 2 + 2) * minors_per_major ; k++) {
			std::stringstream new_id;
			new_id << "majorntimel_" << k;
			if(k % minors_per_major == 0) {
				// create main timeline

				std::stringstream ss;
				ss << "<svg id=\"" << new_id.str() << "\" "
				   << " x=\"0\" \n"
				   << " y=\"0\" \n"
				   << " width=\"" << line_width << "\" \n"
				   << " height=\"" << canvas_h << "\" \n"
				   << ">\n"
				   << "<line class=\"timeline\" "
				   << "stroke=\"black\" "
				   << "stroke-width=\"1.0\" "
				   << "x1=\"0\" "
				   << "x2=\"0\" "
				   << "y1=\"0\" "
				   << "y2=\"" << canvas_h << "\" "
				   << "/>\n"
				   << "<text x=\"5\" y=\"45\" "
				   << "class=\"timetext\" "
				   << "font-family=\"Roboto\" font-size=\"" << font_size << "\" fill=\"black\"> "
				   << "0"
				   << "</text>\n"
				   << "</svg>\n"
					;
				timeline_container->add_svg_child(ss.str());

				major_n_minors.push_back(new KammoGUI::GnuVGCanvas::ElementReference(this, new_id.str()));
			} else {
				// create minors_per_major - 1 dashed lines marking the "in between" minors per major timeline

				std::stringstream ss;
				ss << "<line id=\"" << new_id.str() << "\" "
				   << "stroke=\"black\" "
				   << "stroke-opacity=\"0.2\" "
				   << "stroke-width=\"1.0\" "
				   << "x1=\"0\" "
				   << "x2=\"0\" "
				   << "y1=\"0\" "
				   << "y2=\"" << canvas_h << "\" "
				   << "/>\n";
				timeline_container->add_svg_child(ss.str());

				major_n_minors.push_back(new KammoGUI::GnuVGCanvas::ElementReference(this, new_id.str()));
			}
		}
	}
}

void TimeLines::clear_timelines() {
	for(auto major_or_minor : major_n_minors) {
		major_or_minor->drop_element();
		delete major_or_minor;
	}
	major_n_minors.clear();

	if(timeline_container) {
		timeline_container->drop_element();
		delete timeline_container; timeline_container = NULL;
	}
}

void TimeLines::clear_time_index() {
	if(time_index_container) {
		time_index_container->drop_element();
		delete time_index_container; time_index_container = NULL;
	}

}

void TimeLines::create_time_index() {
	KammoGUI::GnuVGCanvas::ElementReference root_element(this, "timeline_group");

	{ // time index container
		std::stringstream ss;
		ss << "<svg id=\"time_index_container\" "
		   << " x=\"0\" \n"
		   << " y=\"0\" \n"
		   << " width=\"" << canvas_w << "\" \n"
		   << " height=\"" << finger_height << "\" \n"
		   << ">\n"
		   << "<rect id=\"time_index_rectangle\" "
		   << " style=\"fill:#33ccff;fill-opacity:0.5;stroke:none\" "
		   << " x=\"0\" \n"
		   << " y=\"0\" \n"
		   << " width=\"" << canvas_w << "\" \n"
		   << " height=\"" << finger_height << "\" \n"
		   << "/>\n"
		   << "</svg>\n";
		root_element.add_svg_child(ss.str());

		time_index_container = new KammoGUI::GnuVGCanvas::ElementReference(this, "time_index_container");
		time_index_container->set_event_handler(
			[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
			       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
			       const KammoGUI::MotionEvent &event) {
				SATAN_DEBUG("Begin timeline event...\n");
				on_time_index_event(event);
				SATAN_DEBUG("End timeline event...\n");
			}
			);
	}
}

void TimeLines::regenerate_graphics() {
	clear_time_index();
	clear_timelines();
	create_timelines();
	create_time_index();
}

void TimeLines::scrolled_horizontal(float pixels_changed) {
	line_offset += pixels_changed;

	call_scroll_callbacks();
}

bool TimeLines::on_scale(KammoGUI::ScaleGestureDetector *detector) {
	auto focal_x = detector->get_focus_x();
	auto sfactor = detector->get_scale_factor();
	if(horizontal_zoom_factor * sfactor < 1.0) {
		sfactor = 1.0 / horizontal_zoom_factor;
	}
	horizontal_zoom_factor *= sfactor;
	line_offset = focal_x - (focal_x - line_offset) * sfactor;

	SATAN_DEBUG("  new zoom factor: %f\n", horizontal_zoom_factor);

	call_scroll_callbacks();

	return true;
}

bool TimeLines::on_scale_begin(KammoGUI::ScaleGestureDetector *detector) { return true; }

void TimeLines::on_scale_end(KammoGUI::ScaleGestureDetector *detector) {}

void TimeLines::on_time_index_event(const KammoGUI::MotionEvent &event) {
	double scroll_x;

	SATAN_DEBUG("TimeLines::on_time_index_event()\n");

	bool scroll_event = sgd->on_touch_event(event);

	if(scroll_event && (!ignore_scroll)) {
		if(fling_detector.on_touch_event(event)) {
			SATAN_DEBUG("fling detected.\n");
			float speed_x, speed_y;
			float abs_speed_x, abs_speed_y;

			fling_detector.get_speed(speed_x, speed_y);
			fling_detector.get_absolute_speed(abs_speed_x, abs_speed_y);

			bool do_horizontal_fling = abs_speed_x > abs_speed_y ? true : false;

			float speed = 0.0f, abs_speed;
			std::function<void(float pixels_changed)> scrolled;

			if(do_horizontal_fling) {
				abs_speed = abs_speed_x;
				speed = speed_x;
				scrolled = [this](float pixels_changed){
					scrolled_horizontal(pixels_changed);
					SATAN_DEBUG("Scrolled horizontal: %f\n", pixels_changed);
				};
			} else {
				abs_speed = abs_speed_y;
				speed = speed_y;
				scrolled = [this](float pixels_changed){
//					scrolled_vertical(pixels_changed);
					SATAN_DEBUG("Scrolled vertical: %f\n", pixels_changed);
				};
			}

			float fling_duration = abs_speed / FLING_DEACCELERATION;

			FlingAnimation *flinganim =
				new FlingAnimation(speed, fling_duration, scrolled);
			start_animation(flinganim);
		}

		switch(event.get_action()) {
		case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
			ignore_scroll = true;
		case KammoGUI::MotionEvent::ACTION_CANCEL:
		case KammoGUI::MotionEvent::ACTION_OUTSIDE:
		case KammoGUI::MotionEvent::ACTION_POINTER_UP:
			break;
		case KammoGUI::MotionEvent::ACTION_DOWN:
			scroll_start_x = event.get_x();
			break;
		case KammoGUI::MotionEvent::ACTION_MOVE:
		case KammoGUI::MotionEvent::ACTION_UP:
			scroll_x = event.get_x() - scroll_start_x;
			scrolled_horizontal(scroll_x);
			scroll_start_x = event.get_x();
			break;
		}
	} else {
		ignore_scroll = true;
		switch(event.get_action()) {
		case KammoGUI::MotionEvent::ACTION_CANCEL:
		case KammoGUI::MotionEvent::ACTION_OUTSIDE:
		case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
		case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		case KammoGUI::MotionEvent::ACTION_DOWN:
		case KammoGUI::MotionEvent::ACTION_MOVE:
			break;
		case KammoGUI::MotionEvent::ACTION_UP:
			// when the last finger is lifted, break the ignore_scroll lock
			ignore_scroll = false;
			SATAN_DEBUG(" reset ignore_scroll!\n");
			break;
		}
	}
}

void TimeLines::on_loop_marker_event(ModifyingLoop selected_marker, const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG("TimeLines::on_loop_marker_event()\n");

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		scroll_start_x = event.get_x();
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
	{
		currently_modifying = selected_marker;
		auto scroll_x = event.get_x() - scroll_start_x;

		double new_marker_position_delta = scroll_x / (minor_spacing * ((double)minors_per_major));
		SATAN_DEBUG("new_marker_position_delta: %f\n", new_marker_position_delta);
		switch(currently_modifying) {
		case start_marker:
			new_marker_position = loop_start + new_marker_position_delta;
			break;
		case stop_marker:
			new_marker_position = loop_start + loop_length + new_marker_position_delta;
			break;
		case neither_start_or_stop:
		default:
			break;
		}
	}
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		SATAN_DEBUG("calling request_new_loop_settings()....\n");
		request_new_loop_settings(
			currently_modifying == start_marker ? new_marker_position : loop_start,
			currently_modifying == stop_marker ?
			(new_marker_position - loop_start) : (loop_length + loop_start - new_marker_position)
			);
		currently_modifying = neither_start_or_stop;
		break;
	}
}

void TimeLines::request_new_loop_settings(int new_loop_start, int new_loop_length) {
	SATAN_DEBUG("request_new_loop_settings() : %d, %d\n", new_loop_start, new_loop_length);
	if(new_loop_start < 0 || new_loop_length < 0)
		return; // Illegal values
	SATAN_DEBUG("request_new_loop_settings() : requesting lock...\n");
	if(auto gco = gco_w.lock()) {
		SATAN_DEBUG("Calling gco->set_loop_start()....\n");
		gco->set_loop_start(new_loop_start);
		SATAN_DEBUG("Called gco->set_loop_start()....\n");
		gco->set_loop_length(new_loop_length);
	}
}

void TimeLines::loop_start_changed(int new_start) {
	KammoGUI::run_on_GUI_thread(
		[this, new_start]() {
			SATAN_DEBUG("TimeLines::loop_start_changed(%d)\n", new_start);
			loop_start = new_start;
		}
		);
}

void TimeLines::loop_length_changed(int new_length) {
	KammoGUI::run_on_GUI_thread(
		[this, new_length]() {
			SATAN_DEBUG("TimeLines::loop_length_changed(%d)\n", new_length);
			loop_length = new_length;
		}
		);
}

void TimeLines::on_resize() {
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

	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.get_viewport(document_size);

	scaling = finger_height / (double)document_size.height;

	SATAN_ERROR("Timelines finger_height: %d\n", (int)finger_height);

	// font_size = finger_height / 3 - but in integer math
	font_size = 10 * finger_height;
	font_size /= 30;

	SATAN_DEBUG("TimeLines::Font size set to : %d\n", font_size);
	regenerate_graphics();

	loop_marker = KammoGUI::GnuVGCanvas::ElementReference(this, "loopMarker");
	loop_start_marker = KammoGUI::GnuVGCanvas::ElementReference(this, "loopStart");
	loop_stop_marker = KammoGUI::GnuVGCanvas::ElementReference(this, "loopStop");

	loop_start_marker.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Begin loop start marker event...\n");
			on_loop_marker_event(start_marker, event);
			SATAN_DEBUG("End loop start marker event...\n");
		}
		);
	loop_stop_marker.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Begin loop stop marker event...\n");
			on_loop_marker_event(stop_marker, event);
			SATAN_DEBUG("End loop stop marker event...\n");
		}
		);
	SATAN_ERROR("--------- start_marker element ptr %p -> %p\n",
		    loop_start_marker.pointer(), &loop_start_marker);
	SATAN_ERROR("--------- loop_marker element ptr %p -> %p\n",
		    loop_marker.pointer(), &loop_marker);
	SATAN_ERROR("--------- stop_marker element ptr %p -> %p\n",
		    loop_stop_marker.pointer(), &loop_stop_marker);
}

void TimeLines::add_scroll_callback(std::function<void(double, double, int, int)> _cb) {
	scroll_callbacks.push_back(_cb);
	call_scroll_callbacks();
}

void TimeLines::call_scroll_callbacks() {
	auto min_visible_offset = get_sequence_minor_position_at(0);
	auto max_visible_offset = get_sequence_minor_position_at(canvas_w);
	for(auto sc : scroll_callbacks) {
		sc(minor_spacing, line_offset, min_visible_offset, max_visible_offset);
	}
}

void TimeLines::show_loop_markers() {
	loop_marker.set_display("inline");
	loop_start_marker.set_display("inline");
	loop_stop_marker.set_display("inline");
}

void TimeLines::hide_loop_markers() {
	loop_marker.set_display("none");
	loop_start_marker.set_display("none");
	loop_stop_marker.set_display("none");
}

double TimeLines::get_graphics_horizontal_offset() {
	return line_offset;
}

double TimeLines::get_horizontal_pixels_per_minor() {
	return minor_spacing;
}

int TimeLines::get_sequence_minor_position_at(int horizontal_pixel_value) {
	/*
	SATAN_DEBUG("pixel value: %d - spacing: %f - offset: %f\n",
		    horizontal_pixel_value,
		    minor_spacing,
		    line_offset_d);
	*/
	double rval = (double)horizontal_pixel_value;
	rval /= minor_spacing;
	rval -= (line_offset_d * minors_per_major);
	return (int)rval;
}

void TimeLines::set_minor_steps(int minor_steps_per_major) {
	minors_per_major = minor_steps_per_major;
	regenerate_graphics();
}

void TimeLines::set_prefix_string(const std::string &_prefix) {
	prefix_string = _prefix;
}

void TimeLines::on_render() {
	// calcualte current space between minor lines, given the current zoom factor
	minor_spacing = horizontal_zoom_factor * minor_width;

	// calculate line offset
	line_offset_d = line_offset / (minor_spacing * (double)minors_per_major);
	int line_offset_i = line_offset_d; // we need the pure integer version too

	{ // Move loop markers
		KammoGUI::GnuVGCanvas::SVGMatrix loop_start_mtrx, loop_stop_mtrx;

		auto _loop_start = loop_start;
		auto _loop_stop = loop_start + loop_length;

		switch(currently_modifying) {
		case neither_start_or_stop:
			break;
		case start_marker:
			_loop_start = new_marker_position;
			break;
		case stop_marker:
			_loop_stop = new_marker_position;
			break;
		}

		auto loop_start_offset = (_loop_start + line_offset_d) * minor_spacing * ((double)minors_per_major);
		auto loop_stop_offset = (_loop_stop + line_offset_d) * minor_spacing * ((double)minors_per_major);

		loop_start_mtrx.scale(scaling, scaling);
		loop_start_mtrx.translate(loop_start_offset, 0);
		loop_stop_mtrx.scale(scaling, scaling);
		loop_stop_mtrx.translate(loop_stop_offset, 0);

		loop_start_marker.set_transform(loop_start_mtrx);
		loop_stop_marker.set_transform(loop_stop_mtrx);

		auto fhh = finger_height / 2.0;
		loop_marker.set_rect_coords(loop_start_offset, fhh, loop_stop_offset - loop_start_offset, fhh);
	}

	{ // select visible minors, and translate time line graphics
		double graphics_offset = (line_offset_d - (double)line_offset_i) * minor_spacing * ((double)minors_per_major);

		double zfactor = ((double)minors_per_major) / ((double)horizontal_zoom_factor * 4.0);

		skip_interval = (unsigned int)zfactor;

		// clamp skip_interval to [1, MAX_LEGAL]
		static unsigned int legal_intervals[] = {
			1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8,  8,  8,  8,  8,  8,  16
		};
#define MAX_LEGAL (sizeof(legal_intervals) / sizeof(unsigned int))
		if(skip_interval >= MAX_LEGAL) skip_interval = MAX_LEGAL - 1;
		skip_interval = legal_intervals[skip_interval];

		KammoGUI::GnuVGCanvas::SVGMatrix mtrx;

		// translate by graphics_offset
		mtrx.translate(graphics_offset, 0);

		for(unsigned int k = 0; k < major_n_minors.size(); k++) {
			// transform the line marker
			(major_n_minors[k])->set_transform(mtrx);

			// then do an additional translation for the next one
			mtrx.translate(minor_spacing, 0);

			// calculate prospective line number
			int line_number = (-line_offset_i + (k / minors_per_major));

			if((line_number >= 0) && (k % skip_interval == 0)) {
				major_n_minors[k]->set_display("inline");

				try { // for major lines (not minors) set timetext to the current line number
					std::stringstream ss;
					ss << line_number;

					major_n_minors[k]->find_child_by_class("timetext").set_text_content(
						prefix_string + ss.str());
				} catch(KammoGUI::GnuVGCanvas::NoSuchElementException e) { /* timetext only available on major lines, not minors */ }
			} else {
				major_n_minors[k]->set_display("none");
			}
		}
	}
}

TimeLines::TimeLines(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/timeLines.svg"), cnvs) {
	sgd = new KammoGUI::ScaleGestureDetector(this);
}

TimeLines::~TimeLines() {
}

void TimeLines::object_registered(std::shared_ptr<GCO> _gco) {
	KammoGUI::run_on_GUI_thread(
		[this, _gco]() {
			SATAN_DEBUG("TimeLines got gco object...\n");
			gco_w = _gco;
			if(auto gco = gco_w.lock()) {
				SATAN_DEBUG("TimeLines locked gco object...\n");
				gco->add_global_control_listener(shared_from_this());
			}
		}
		);
}

void TimeLines::object_unregistered(std::shared_ptr<GCO> _gco) {
	KammoGUI::run_on_GUI_thread(
		[this, _gco]() {
		}
		);
}
