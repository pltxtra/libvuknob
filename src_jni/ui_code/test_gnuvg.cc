/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
 * Copyright (C) 2008 by Anton Persson
 *
 * About SATAN:
 *   Originally developed as a small subproject in
 *   a course about applied signal processing.
 * Original Developers:
 *   Anton Persson
 *   Johan Thim
 *
 * http://www.733kru.org/
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
#include <iostream>
using namespace std;

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include <math.h>

#include <jngldrum/jinformer.hh>
#include <build_time_data.hh>
#include <gnuVGcanvas.hh>

#include "machine.hh"
#include "satan_project_entry.hh"
#include "test_gnuvg.hh"
#include "svg_loader.hh"
#include "../engine_code/server.hh"
#include "../engine_code/client.hh"
#include "controller_handler.hh"

#ifdef ANDROID
#include "android_java_interface.hh"
#endif

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#include "common.hh"


/***************************
 *
 *  Class TestGnuVG::ThumpAnimation
 *
 ***************************/
TestGnuVG::ThumpAnimation *TestGnuVG::ThumpAnimation::active = NULL;

TestGnuVG::ThumpAnimation::~ThumpAnimation() {
	active = NULL;
}

TestGnuVG::ThumpAnimation::ThumpAnimation(double *_thump_offset, float duration) :
	KammoGUI::Animation(duration),
	thump_offset(_thump_offset) {}

TestGnuVG::ThumpAnimation *TestGnuVG::ThumpAnimation::start_thump(double *_thump_offset, float duration) {
	if(active) return NULL;

	active = new ThumpAnimation(_thump_offset, duration);
	return active;
}

void TestGnuVG::ThumpAnimation::new_frame(float progress) {
	(*thump_offset) = sin((1.0 - progress) * 2.0 * M_PI);
}

void TestGnuVG::ThumpAnimation::on_touch_event() { /* ignore touch */ }

/***************************
 *
 *  Class TestGnuVG
 *
 ***************************/

void TestGnuVG::on_resize() {
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);

	int pixel_w, pixel_h;
	get_canvas_size(pixel_w, pixel_h);

	float canvas_w_inches, canvas_h_inches;
	get_canvas_size_inches(canvas_w_inches, canvas_h_inches);

	float w_fingers = canvas_w_inches / INCHES_PER_FINGER;
	float h_fingers = canvas_h_inches / INCHES_PER_FINGER;

	// calculate pixel size of 12 by 15 fingers
	double finger_width = (12.0 * pixel_w ) / w_fingers;
	double finger_height = (15.0 * pixel_h) / h_fingers;

	// if larger than the canvas pixel size, then limit it
	finger_width = finger_width < pixel_w ? finger_width : pixel_w;
	finger_height = finger_height < pixel_h ? finger_height : pixel_h;

	// calculate scaling factor
	double scaling_w = (finger_width) / (double)document_size.width;
	double scaling_h = (finger_height) / (double)document_size.height;

	double scaling = scaling_w < scaling_h ? scaling_w : scaling_h;

	// calculate translation
	double translate_x = (pixel_w - document_size.width * scaling) / 2.0;
	double translate_y = (pixel_h - document_size.height * scaling) / 2.0;

	// initiate transform_m
	transform_m.init_identity();
	transform_m.translate(translate_x, translate_y);
	transform_m.scale(scaling, scaling);
}

void TestGnuVG::on_render() {
	if(1)
	{ /* process thump */
		KammoGUI::GnuVGCanvas::SVGMatrix logo_base_t;
		KammoGUI::GnuVGCanvas::SVGMatrix logo_thump_t;

		logo_thump_t.translate(-600.0, -500.0);
		logo_thump_t.rotate(2.0 * 3.14 * thump_offset);
		logo_thump_t.scale(1.0 + 0.1 * thump_offset,
				   1.0 + 0.1 * thump_offset
			);
		logo_thump_t.translate(600.0, 500.0);

		if(!logo_base_got) {
			knobBody_element->get_transform(knob_base_t);
			logo_base_got = true;
		}

		logo_base_t = knob_base_t;
		logo_base_t.multiply(logo_thump_t);
		knobBody_element->set_transform(logo_base_t);
	}

	KammoGUI::GnuVGCanvas::ElementReference root_element(this);
	root_element.set_transform(transform_m);

//	KammoGUI::GnuVGCanvas::ElementReference(this, "versionString").set_text_content("v" VERSION_NAME);
}

void TestGnuVG::element_on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
				 KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				 const KammoGUI::MotionEvent &event) {
	TestGnuVG *ctx = (TestGnuVG *)source;

	if(event.get_action() == KammoGUI::MotionEvent::ACTION_UP) {
		if(e_ref == ctx->knobBody_element) {
			ThumpAnimation *thump = ThumpAnimation::start_thump(&(ctx->thump_offset), 5.0);
			if(thump)
				ctx->start_animation(thump);
		}
		if(e_ref == ctx->network_element)
			KammoGUI::GnuVGCanvas::ElementReference(ctx, "versionString").set_text_content("NETWORK");
		if(e_ref == ctx->start_element)
			KammoGUI::GnuVGCanvas::ElementReference(ctx, "versionString").set_text_content("START");
	}
}

TestGnuVG::TestGnuVG(KammoGUI::GnuVGCanvas *cnvs, std::string fname)
	: SVGDocument(fname, cnvs)
{
	try {
		knobBody_element = new KammoGUI::GnuVGCanvas::ElementReference(this, "knobBody");
		knobBody_element->set_event_handler(element_on_event);
	} catch(...) {
		SATAN_ERROR("Could not find knobBody element in test_gnuvg.cc\n");
	}
	try {
		network_element = new KammoGUI::GnuVGCanvas::ElementReference(this, "network");
		network_element->set_event_handler(element_on_event);
	} catch(...) {
		SATAN_ERROR("Could not find network element in test_gnuvg.cc\n");
	}

	try {
		start_element = new KammoGUI::GnuVGCanvas::ElementReference(this, "start");
		start_element->set_event_handler(element_on_event);
	} catch(...) {
		SATAN_ERROR("Could not find start element in test_gnuvg.cc\n");
	}
}

/***************************
 *
 *  Kamoflage Event Declaration
 *
 ***************************/

KammoEventHandler_Declare(TestGnuVGHandler,"gnuVG_test");

virtual void on_init(KammoGUI::Widget *wid) {
	KammoGUI::GnuVGCanvas *cnvs = (KammoGUI::GnuVGCanvas *)wid;
	cnvs->set_bg_color(1.0, 0.631373, 0.137254);

	SATAN_DEBUG("init TestGnuVG - id: %s\n", wid->get_id().c_str());

	(void)new TestGnuVG(cnvs, std::string(SVGLoader::get_svg_directory() + "/testGnuVG.svg"));
//	(void)new TestGnuVG(cnvs, std::string(SVGLoader::get_svg_directory() + "/logoScreen.svg"));
}

KammoEventHandler_Instance(TestGnuVGHandler);
