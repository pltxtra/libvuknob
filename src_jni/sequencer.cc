/*
 * VuKNOB
 * (C) 2014 by Anton Persson
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

#include "sequencer.hh"
#include "timelines.hh"
#include "svg_loader.hh"
#include "machine.hh"
#include "common.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class Sequencer
 *
 ***************************/

Sequencer::Sequencer(KammoGUI::SVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/empty.svg"), cnvs) {
}

Sequencer::~Sequencer() {
}

void Sequencer::on_resize() {
}

void Sequencer::on_render() {
	SATAN_DEBUG("Sequencer::on_render()\n");
}

/***************************
 *
 *  Kamoflage Event Declaration
 *
 ***************************/

KammoEventHandler_Declare(SequencerHandler,"sequence_container");

virtual void on_init(KammoGUI::Widget *wid) {
	SATAN_DEBUG("on_init() for sequence_container\n");
	if(wid->get_id() == "sequence_container") {
		KammoGUI::SVGCanvas *cnvs = (KammoGUI::SVGCanvas *)wid;
		cnvs->set_bg_color(1.0, 1.0, 1.0);

		static auto current_sequencer = new Sequencer(cnvs);
		static auto current_timelines = new TimeLines(cnvs);
	}
}

KammoEventHandler_Instance(SequencerHandler);
