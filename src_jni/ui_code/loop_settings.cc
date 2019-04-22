/*
 * VuKNOB
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
#include <math.h>

#include "loop_settings.hh"
#include "../machine.hh"
#include "svg_loader.hh"
#include "common.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class LoopSettings
 *
 ***************************/

bool LoopSettings::on_button_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
				   KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				   const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG(" LoopSettings::on_button_event. (doc %p - LoopSettings %p)\n", source, e_ref);
	LoopSettings *ctx = (LoopSettings *)source;

	double now_x = event.get_x();
	double now_y = event.get_y();

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		ctx->is_a_tap = false;
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		// check if the user is moving the finger too far, indicating abort action
		// if so - disable is_a_tap
		if(ctx->is_a_tap) {
			if(fabs(now_x - ctx->first_selection_x) > 20)
				ctx->is_a_tap = false;
			if(fabs(now_y - ctx->first_selection_y) > 20)
				ctx->is_a_tap = false;
		}
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		ctx->is_a_tap = true;
		ctx->first_selection_x = now_x;
		ctx->first_selection_y = now_y;
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		if(ctx->is_a_tap) {
			return true;
		}
		break;
	}
	return false;
}

LoopSettings::LoopSettings(KammoGUI::GnuVGCanvas *cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/loopEditor.svg"), cnvs)
{
	loopSettingsButton = KammoGUI::GnuVGCanvas::ElementReference(this, "loopSettingsButton");
	loopSettingsButton.set_event_handler(
		[this](SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e,
		       const KammoGUI::MotionEvent &event)
		{
			if(on_button_event(source, e, event)) {
				auto loopState = Machine::get_loop_state();
				Machine::set_loop_state(!loopState);
				SATAN_ERROR("Toggle looping\n");
				refresh_loop_state_icons();
			}
		}
		);
	loopParameterSettingButton = KammoGUI::GnuVGCanvas::ElementReference(this, "loopParameterSettingButton");
	loopParameterSettingButton.set_event_handler(
		[this](SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e,
		       const KammoGUI::MotionEvent &event)
		{
			if(on_button_event(source, e, event)) {
				SATAN_ERROR("Toggle loop editing\n");
			}
		}
		);
	playAsLoopIcon = KammoGUI::GnuVGCanvas::ElementReference(this, "playAsLoop");
	playAsEndlessIcon = KammoGUI::GnuVGCanvas::ElementReference(this, "playAsEndless");
	startStopIcon = KammoGUI::GnuVGCanvas::ElementReference(this, "startStopIcon");
	editStartStopIcon = KammoGUI::GnuVGCanvas::ElementReference(this, "editStartStopIcon");

	refresh_loop_state_icons();
}

LoopSettings::~LoopSettings() {}

void LoopSettings::refresh_loop_state_icons() {
	auto loopState = Machine::get_loop_state();
	playAsLoopIcon.set_display(loopState ? "inline" : "none");
	playAsEndlessIcon.set_display(loopState ? "none" : "inline");
}

void LoopSettings::on_render() {
	// Translate the document, and scale it properly to fit the defined viewbox
	KammoGUI::GnuVGCanvas::ElementReference root(this);

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t = base_transform_t;
	root.set_transform(transform_t);
}

void LoopSettings::on_resize() {
	auto canvas = get_canvas();
	int canvas_w, canvas_h;
	float canvas_w_inches, canvas_h_inches;
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	auto canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	auto canvas_height_fingers = (int)tmp;

	tmp = canvas_w / ((double)canvas_width_fingers);
	auto finger_width = tmp;
	tmp = canvas_h / ((double)canvas_height_fingers);
	auto finger_height = tmp;

	// get data
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	root.get_viewport(document_size);
	auto scaling = finger_height / document_size.height;

	// initiate transform_m
	base_transform_t.init_identity();
	base_transform_t.scale(scaling, scaling);
}
