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

#include "gnuvg_corner_button.hh"
#include "svg_loader.hh"
#include "common.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class ConerButton::Transition
 *
 ***************************/

GnuVGCornerButton::Transition::Transition(
	GnuVGCornerButton *_context,
	std::function<void(GnuVGCornerButton *context, float progress)> _callback) :
	KammoGUI::Animation(TRANSITION_TIME), ctx(_context), callback(_callback) {
	SATAN_DEBUG(" created new Transition for GnuVGCornerButton %p\n", ctx);
}

void GnuVGCornerButton::Transition::new_frame(float progress) {
	callback(ctx, progress);
}

void GnuVGCornerButton::Transition::on_touch_event() { /* ignored */ }

/***************************
 *
 *  Class GnuVGCornerButton
 *
 ***************************/

void GnuVGCornerButton::transition_progressed(GnuVGCornerButton *ctx, float progress) {
	SATAN_DEBUG("  GnuVGCornerButton transition progressed for ctx %p\n", ctx);
	if(ctx->hidden)
		ctx->offset_factor = (double)progress;
	else
		ctx->offset_factor = 1.0 - ((double)progress);
}

void GnuVGCornerButton::run_transition() {
	Transition *transition = new Transition(this, transition_progressed);
	SATAN_DEBUG(" Created new GnuVGCornerButton::Transition - %p\n", transition);
	if(transition) {
		start_animation(transition);
		SATAN_DEBUG("   GnuVGCornerButton animation started.\n");
	} else {
		transition_progressed(this, 1.0); // skip to final state directly
	}
}

void GnuVGCornerButton::on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
			  KammoGUI::GnuVGCanvas::ElementReference *e_ref,
			  const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG(" GnuVGCornerButton::on_event. (doc %p - GnuVGCornerButton %p)\n", source, e_ref);
	GnuVGCornerButton *ctx = (GnuVGCornerButton *)source;

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
			ctx->callback_function();
		}
		break;
	}
}

GnuVGCornerButton::GnuVGCornerButton(KammoGUI::GnuVGCanvas *cnvs, const std::string &svg_file, WhatCorner _corner) : SVGDocument(svg_file, cnvs), my_corner(_corner), hidden(false), offset_factor(0.0), callback_function([](){}) {
	elref = KammoGUI::GnuVGCanvas::ElementReference(this);
	elref.set_event_handler(on_event);
}

GnuVGCornerButton::~GnuVGCornerButton() {}

void GnuVGCornerButton::on_render() {
	// Translate the document, and scale it properly to fit the defined viewbox
	KammoGUI::GnuVGCanvas::ElementReference root(this);

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t = base_transform_t;
	transform_t.translate(offset_factor * offset_target_x, offset_factor * offset_target_y);
	root.set_transform(transform_t);
}

void GnuVGCornerButton::on_resize() {
	int canvas_w, canvas_h;
	float canvas_w_inches, canvas_h_inches;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	// get data
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);
	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	{ // calculate transform for the main part of the document
		double tmp;

		// calculate the width of the canvas in "fingers"
		tmp = canvas_w_inches / INCHES_PER_FINGER;
		double canvas_width_fingers = tmp;

		// calculate the size of a finger in pixels
		tmp = canvas_w / (canvas_width_fingers);
		double finger_width = tmp;

		// calculate scaling factor
		double scaling = (1.5 * finger_width) / (double)document_size.width;

		// calculate translation
		double translate_x = 0.0, translate_y = 0.0;

		switch(my_corner) {
		case top_left:
		{
			translate_x = 0.25 * document_size.width * scaling;
			translate_y = 0.25 * document_size.height * scaling;
			offset_target_x = -3.0 * document_size.width * scaling;
			offset_target_y = -2.0 * document_size.height * scaling;
		}
		break;

		case top_right:
		{
			translate_x = canvas_w - 1.25 * document_size.width * scaling;
			translate_y = 0.25 * document_size.height * scaling;
			offset_target_x = 3.0 * document_size.width * scaling;
			offset_target_y = -2.0 * document_size.height * scaling;
		}
		break;

		case bottom_left:
		{
			translate_x = 0.25 * document_size.width * scaling;
			translate_y = canvas_h - 1.25 * document_size.height * scaling;
			offset_target_x = -3.0 * document_size.width * scaling;
			offset_target_y = 3.0 * document_size.height * scaling;
		}
		break;
		case bottom_right:
		{
			translate_x = canvas_w - 1.25 * document_size.width * scaling;
			translate_y = canvas_h - 1.25 * document_size.height * scaling;
			offset_target_x = 3.0 * document_size.width * scaling;
			offset_target_y = 3.0 * document_size.height * scaling;
		}
		break;
		}

		// initiate transform_m
		base_transform_t.init_identity();
		base_transform_t.scale(scaling, scaling);
		base_transform_t.translate(translate_x, translate_y);
	}
}

void GnuVGCornerButton::show() {
	hidden = false;
	run_transition();
}

void GnuVGCornerButton::hide() {
	hidden = true;
	run_transition();
}

void GnuVGCornerButton::set_select_callback(std::function<void()> _callback_function) {
	callback_function = _callback_function;
}
