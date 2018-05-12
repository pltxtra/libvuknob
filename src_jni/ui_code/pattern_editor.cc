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

#include "pattern_editor.hh"
#include "svg_loader.hh"
#include "common.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

void PatternEditor::on_resize() {
	get_canvas_size(canvas_w, canvas_h);
	get_canvas_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	canvas_height_fingers = (int)tmp;

	// calculate the "width" of a finger tip in pixels
	tmp = canvas_w / ((double)canvas_width_fingers);
	finger_width = tmp;

	// calculate the "height" of a finger tip in pixels
	tmp = canvas_h / ((double)canvas_height_fingers);
	finger_height = tmp;

	// get document parameters
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	auto root = KammoGUI::GnuVGCanvas::ElementReference(this);
	root.get_viewport(document_size);

	SATAN_DEBUG("::on_resize(%f / %f => %f, %f / %f => %f\n",
		    finger_width, document_size.width, finger_width / document_size.width,
		    finger_height, document_size.height, finger_height / document_size.height
		);

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.scale(
		finger_width / document_size.width,

		// we assume that there are 108 keys in the keyboard graphic....
		108.0 * finger_height / document_size.height
		);

	root.set_transform(transform_t);
}

void PatternEditor::on_render() {
}

PatternEditor::PatternEditor(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/VerticalKeyboard.svg"), cnvs) {
}

PatternEditor::~PatternEditor() {
}
