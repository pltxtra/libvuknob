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
#include <unistd.h>

#include "gnuvg_connection_list.hh"
#include "svg_loader.hh"
#include "common.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class GnuVGConnectionList::ConnectionGraphic
 *
 ***************************/

GnuVGConnectionList::ConnectionGraphic::ConnectionGraphic(GnuVGConnectionList *context, const std::string &id,
							  std::function<void()> on_delete,
							  const std::string &src, const std::string &dst) : ElementReference(context, id) {
	set_display("inline");

	find_child_by_class("outputName").set_text_content(src);
	find_child_by_class("inputName").set_text_content(dst);

	delete_button = find_child_by_class("deleteButton");
	delete_button.set_event_handler(
		[this, context, on_delete](KammoGUI::GnuVGCanvas::SVGDocument *,
					   KammoGUI::GnuVGCanvas::ElementReference *,
					   const KammoGUI::MotionEvent &event) {
			if(event.get_action() == KammoGUI::MotionEvent::ACTION_UP) {
				on_delete();
				context->hide();
			}
		}
		);
}

GnuVGConnectionList::ConnectionGraphic::~ConnectionGraphic() {
	drop_element();
}

auto GnuVGConnectionList::ConnectionGraphic::create(GnuVGConnectionList *context, int id,
						    std::function<void()> on_delete,
						    const std::string &src, const std::string &dst) -> std::shared_ptr<ConnectionGraphic> {
	std::stringstream id_stream;
	id_stream << "graphic_" << id;

	SATAN_DEBUG("Create GnuVGConnectionList::ConnectionGraphic - context: %p\n", context);

	KammoGUI::GnuVGCanvas::ElementReference graphic_template = KammoGUI::GnuVGCanvas::ElementReference(context, "connectionTemplate");
	KammoGUI::GnuVGCanvas::ElementReference connection_layer = KammoGUI::GnuVGCanvas::ElementReference(context, "connectionLayer");

	(void) connection_layer.add_element_clone(id_stream.str(), graphic_template);

	auto retval = std::make_shared<ConnectionGraphic>(context, id_stream.str(), on_delete, src, dst);

	return retval;
}

/***************************
 *
 *  Class GnuVGConnectionList::ZoomTransition
 *
 ***************************/

GnuVGConnectionList::ZoomTransition::ZoomTransition(
	std::function<void(double zoom)> _callback,
	double _zoom_start, double _zoom_stop) :

	KammoGUI::Animation(TRANSITION_TIME), callback(_callback),
	zoom_start(_zoom_start), zoom_stop(_zoom_stop)

{}

void GnuVGConnectionList::ZoomTransition::new_frame(float progress) {
	double T = (double)progress;
	double zoom = (zoom_stop - zoom_start) * T + zoom_start;
	callback(zoom);
}

void GnuVGConnectionList::ZoomTransition::on_touch_event() { /* ignored */ }

/***************************
 *
 *  Class GnuVGConnectionList
 *
 ***************************/

GnuVGConnectionList::GnuVGConnectionList(KammoGUI::GnuVGCanvas *cnvs) : SVGDocument(SVGLoader::get_svg_path("connection.svg"), cnvs) {
	KammoGUI::GnuVGCanvas::ElementReference(this, "connectionTemplate").set_display("none");
	KammoGUI::GnuVGCanvas::ElementReference(this).set_display("none");

	backdrop = KammoGUI::GnuVGCanvas::ElementReference(this, "backDrop");
	backdrop.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *,
		       KammoGUI::GnuVGCanvas::ElementReference *,
		       const KammoGUI::MotionEvent &event) {
			if(event.get_action() == KammoGUI::MotionEvent::ACTION_UP) {
				hide();
			}
		}
		);
}

GnuVGConnectionList::~GnuVGConnectionList() {}

void GnuVGConnectionList::on_render() {
	// Resize the backdrop
	{
		std::stringstream opacity;
		opacity << "opacity:" << (zoom_factor);
		backdrop.set_style(opacity.str());
		backdrop.set_rect_coords(0.0, 0.0, canvas_w, canvas_h);
	}

	// translate each list element
	{
		KammoGUI::GnuVGCanvas::SVGMatrix element_transform_t = base_transform_t;

		for(auto gfx : graphics) {
			gfx->set_transform(element_transform_t);
			element_transform_t.translate(0.0, element_vertical_offset);
		}
	}

	// zoom the connectionLayer
	{
		KammoGUI::GnuVGCanvas::ElementReference layer(this, "connectionLayer");

		KammoGUI::GnuVGCanvas::SVGMatrix zoom_transform_t;
		zoom_transform_t.translate(-(document_size.width / 2.0), -(document_size.height / 2.0));
		zoom_transform_t.scale(zoom_factor, zoom_factor);
		zoom_transform_t.translate((document_size.width / 2.0), (document_size.height / 2.0));
		zoom_transform_t.translate(0.0, -(element_vertical_offset * ((double)graphics.size()) / 2.0) + 0.5 * element_vertical_offset);

		layer.set_transform(base_transform_t);
		layer.set_transform(zoom_transform_t);
	}
}

void GnuVGConnectionList::on_resize() {
	float canvas_w_inches, canvas_h_inches;

	// get data
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);
	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	element_vertical_offset = document_size.height;

	{ // calculate transform for the main part of the document
		double tmp;

		// calculate the width of the canvas in "fingers"
		tmp = canvas_w_inches / INCHES_PER_FINGER;
		double canvas_width_fingers = tmp;

		// calculate the size of a finger in pixels
		tmp = canvas_w / (canvas_width_fingers);
		double finger_width = tmp;

		// calculate scaling factor
		double scaling = (4.0 * finger_width) / (double)document_size.width;

		// calculate translation
		double translate_x, translate_y;

		translate_x = (canvas_w - document_size.width * scaling) / 2.0;
		translate_y = (canvas_h - document_size.height * scaling) / 2.0;

		// initiate transform_m
		base_transform_t.init_identity();
		base_transform_t.scale(scaling, scaling);
		base_transform_t.translate(translate_x, translate_y);
	}
}

void GnuVGConnectionList::clear() {
	graphics.clear();
}

void GnuVGConnectionList::add(const std::string &src, const std::string &dst, std::function<void()> on_delete) {
	graphics.push_back(ConnectionGraphic::create(this, graphics.size(), on_delete, src, dst));
}

void GnuVGConnectionList::show() {
	KammoGUI::GnuVGCanvas::ElementReference(this).set_display("inline");

	ZoomTransition *transition = new ZoomTransition([this](double factor){
			zoom_factor = factor;
		}
		, 0.0001, 1.0);
	start_animation(transition);
}

void GnuVGConnectionList::hide() {
	KammoGUI::GnuVGCanvas::ElementReference(this).set_display("inline");

	ZoomTransition *transition = new ZoomTransition([this](double factor){
			zoom_factor = factor;
			if(factor <= 0.0004) {
				KammoGUI::GnuVGCanvas::ElementReference(this).set_display("none");
			}
		}
		, 1.0, 0.0001);
	start_animation(transition);
}
