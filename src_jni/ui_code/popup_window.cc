/*
 * vu|KNOB
 * (C) 2020 by Anton Persson
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

#include "popup_window.hh"
#include "svg_loader.hh"
#include "common.hh"
#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static TapDetector tap_detector;

void PopupWindow::hide() {
	auto fade_out = new KammoGUI::SimpleAnimation(
		TRANSITION_TIME,
		[this](float progress) mutable {
			auto reverse_progress = 1.0f - progress;
			std::stringstream opacity;
			opacity << "opacity:" << (reverse_progress);
			root.set_style(opacity.str());
			if(progress >= 1.0f) {
				root.set_display("none");
			}
		}
		);
	start_animation(fade_out);
}

void PopupWindow::show() {
	root.set_display("inline");
	root.set_style("opacity:0.0");
	auto fade_in = new KammoGUI::SimpleAnimation(
		TRANSITION_TIME,
		[this](float progress) mutable {
			std::stringstream opacity;
			opacity << "opacity:" << (progress);
			root.set_style(opacity.str());
		}
		);
	start_animation(fade_in);
}

PopupWindow::PopupWindow(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/popupWindow.svg"), cnvs) {
}

PopupWindow::~PopupWindow() {
}

void PopupWindow::on_resize() {
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

	// scale document
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
	container = KammoGUI::GnuVGCanvas::ElementReference(this, "container");
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	root.get_viewport(document_size);
	scaling = (3 * finger_height) / document_size.height;
	transform_t.scale(scaling, scaling);
	transform_t.translate(
		(canvas_width_fingers - 5) * 0.5 * finger_width,
		(canvas_height_fingers - 3) * 0.5 * finger_height
		);
	container.set_transform(transform_t);

	// Fill backdrop
	backdrop = KammoGUI::GnuVGCanvas::ElementReference(this, "backdrop");
	backdrop.set_rect_coords(0.0, 0.0, canvas_w, canvas_h);

	backdrop.set_event_handler(
		[this](SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e,
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Backdrop pressed\n");
		}
		);

	yes_button = KammoGUI::GnuVGCanvas::ElementReference(this, "yesButton");
	no_button = KammoGUI::GnuVGCanvas::ElementReference(this, "noButton");

	yes_button.set_event_handler(
		[this](SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e,
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Yes pressed\n");
			if(tap_detector.analyze_events(event)) {
				hide();
			}
		}
		);
	no_button.set_event_handler(
		[this](SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e,
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("No pressed\n");
			if(tap_detector.analyze_events(event)) {
				hide();
			}
		}
		);
}

void PopupWindow::on_render() {
}

void PopupWindow::ask_yes_or_no(std::function<void(UserResponse response)> response_callback) {
}
