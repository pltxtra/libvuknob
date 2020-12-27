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

#include "simple_button.hh"
#include "svg_loader.hh"
#include "common.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class SimpleButton
 *
 ***************************/

void SimpleButton::on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
			    KammoGUI::GnuVGCanvas::ElementReference *e_ref,
			    const KammoGUI::MotionEvent &event) {
	SATAN_DEBUG(" SimpleButton::on_event. (doc %p - SimpleButton %p)\n", source, e_ref);
	SimpleButton *ctx = (SimpleButton *)source;

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
		if(ctx->is_a_tap && ctx->callback_function) {
			ctx->callback_function();
		}
		break;
	}
}

SimpleButton::SimpleButton(KammoGUI::GnuVGCanvas *cnvs, const std::string &svg_file)
	: SVGDocument(svg_file, cnvs)
	, callback_function([](){})
{
	elref = KammoGUI::GnuVGCanvas::ElementReference(this);
	elref.set_event_handler(on_event);
}

SimpleButton::~SimpleButton() {}

void SimpleButton::on_render() {
	// Translate the document, and scale it properly to fit the defined viewbox
	KammoGUI::GnuVGCanvas::ElementReference root(this);

	root.set_transform(base_transform_t);
}

void SimpleButton::on_resize() {
	// get data
	elref.get_viewport(document_size);

	if(width == 0.0 || height == 0.0) {
		set_size(document_size.width, document_size.height);
	}
}

void SimpleButton::show() {
}

void SimpleButton::hide() {
}

void SimpleButton::set_size(double w, double h) {
	width = w;
	height = h;

	auto scale_w = width / document_size.width;
	auto scale_h = height / document_size.height;

	base_transform_t.init_identity();
	base_transform_t.scale(scale_w, scale_h);
	base_transform_t.translate(x, y);
}

void SimpleButton::move_to(double _x, double _y) {
	x = _x; y = _y;
}

void SimpleButton::set_select_callback(std::function<void()> _callback_function) {
	callback_function = _callback_function;
}
