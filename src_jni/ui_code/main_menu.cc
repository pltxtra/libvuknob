/*
 * VuKNOB
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

#include <iostream>
#include <math.h>

#include "main_menu.hh"
#include "svg_loader.hh"
#include "common.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class MainMenu
 *
 ***************************/

MainMenu::MainMenu(KammoGUI::GnuVGCanvas *cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/MainMenu.svg"), cnvs)
{
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
	backdrop = KammoGUI::GnuVGCanvas::ElementReference(this, "backdrop");

	rewind_button = KammoGUI::GnuVGCanvas::ElementReference(this, "rewindButton");
	play_button = KammoGUI::GnuVGCanvas::ElementReference(this, "playButton");
	record_button = KammoGUI::GnuVGCanvas::ElementReference(this, "recordButton");
	connector_button = KammoGUI::GnuVGCanvas::ElementReference(this, "connectorButton");
	jam_button = KammoGUI::GnuVGCanvas::ElementReference(this, "jamButton");
	sequencer_button = KammoGUI::GnuVGCanvas::ElementReference(this, "sequencerButton");
	settings_button = KammoGUI::GnuVGCanvas::ElementReference(this, "settingsButton");
}

MainMenu::~MainMenu() {}

void MainMenu::on_render() {
}

void MainMenu::on_resize() {
	SATAN_DEBUG("MainMenu::on_resize()\n");

	// get data
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);
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

	scaling = finger_height / document_size.height;

	SATAN_DEBUG(" ----------------- MAIN MENU w/h: %f, %f\n", document_size.width, document_size.height);
	SATAN_DEBUG(" ----------------- MAIN MENU scaling: %f, height: %f\n", scaling, finger_height);

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;;
	transform_t.scale(scaling, scaling);
	transform_t.translate(0.0, canvas_h - finger_height);
	root.set_transform(transform_t);

	auto scaled_fw = finger_width / scaling;

	transform_t.init_identity();
	rewind_button.set_transform(transform_t);
	transform_t.translate(scaled_fw, 0.0);
	play_button.set_transform(transform_t);
	transform_t.translate(scaled_fw, 0.0);
	record_button.set_transform(transform_t);

	transform_t.init_identity();
	transform_t.translate(canvas_w / scaling - 4.0 * scaled_fw, 0.0);
	jam_button.set_transform(transform_t);
	transform_t.translate(scaled_fw, 0.0);
	sequencer_button.set_transform(transform_t);
	transform_t.translate(scaled_fw, 0.0);
	connector_button.set_transform(transform_t);
	transform_t.translate(scaled_fw, 0.0);
	settings_button.set_transform(transform_t);

	backdrop.set_rect_coords(0.0, 0.0, canvas_w, finger_height);
}
