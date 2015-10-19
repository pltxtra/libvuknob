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

#include "sequencer.hh"
#include "timelines.hh"
#include "svg_loader.hh"
#include "machine.hh"
#include "common.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class Sequencer::Sequence
 *
 ***************************/

Sequencer::Sequence::Sequence(KammoGUI::SVGCanvas::ElementReference elref,
			      std::shared_ptr<RemoteInterface::RIMachine> _ri_machine,
			      int _offset)
	: KammoGUI::SVGCanvas::ElementReference(elref)
	, ri_machine(_ri_machine)
	, offset(_offset)
{
	SATAN_DEBUG("Sequencer::Sequence::Sequence()\n");
	set_display("inline");

	find_child_by_class("machineId").set_text_content(ri_machine->get_sibling_name());

}

void Sequencer::Sequence::on_move() {
}

void Sequencer::Sequence::on_attach(std::shared_ptr<RemoteInterface::RIMachine> src_machine,
				    const std::string src_output,
				    const std::string dst_input) {
}

void Sequencer::Sequence::on_detach(std::shared_ptr<RemoteInterface::RIMachine> src_machine,
				    const std::string src_output,
				    const std::string dst_input) {
}

void Sequencer::Sequence::set_graphic_parameters(double graphic_scaling_factor,
						 double width, double height,
						 double canvas_w, double canvas_h) {
	find_child_by_class("seqBackground").set_rect_coords(
		width, 0, canvas_w, height);

	// initiate transform_t
	KammoGUI::SVGCanvas::SVGMatrix transform_t;

	transform_t.init_identity();
	transform_t.translate(0.0, (double)(1 + offset) * height);
	transform_t.scale(graphic_scaling_factor, graphic_scaling_factor);

	SATAN_DEBUG("offset: %d\n", offset);

	set_transform(transform_t);
}

auto Sequencer::Sequence::create_sequence(
	KammoGUI::SVGCanvas::ElementReference &root,
	KammoGUI::SVGCanvas::ElementReference &sequence_graphic_template,
	std::shared_ptr<RemoteInterface::RIMachine> ri_machine,
	int offset) -> std::shared_ptr<Sequence> {

	KammoGUI::SVGCanvas::ElementReference new_graphic =
		root.add_element_clone(ri_machine->get_name(), sequence_graphic_template);

	SATAN_DEBUG("Sequencer::Sequence::create_sequence()\n");

	return std::make_shared<Sequence>(new_graphic, ri_machine, offset);
}

/***************************
 *
 *  Class Sequencer
 *
 ***************************/

Sequencer::Sequencer(KammoGUI::SVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/sequencerMachine.svg"), cnvs) {
	sequence_graphic_template = KammoGUI::SVGCanvas::ElementReference(this, "sequencerMachineTemplate");
	root = KammoGUI::SVGCanvas::ElementReference(this);

	sequence_graphic_template.set_display("none");
}

void Sequencer::on_resize() {
	get_canvas_size(canvas_w, canvas_h);
	get_canvas_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	canvas_height_fingers = (int)tmp;

	tmp = canvas_w / ((double)canvas_width_fingers);
	finger_width = tmp;
	tmp = canvas_h / ((double)canvas_height_fingers);
	finger_height = tmp;

	KammoGUI::SVGCanvas::SVGRect document_size;

	// get data
	root.get_viewport(document_size);

	double scaling = finger_height / document_size.height;

	for(auto m2s : machine2sequence) {
		m2s.second->set_graphic_parameters(
			scaling,
			document_size.width, document_size.height,
			canvas_w, canvas_h);
	}
}

void Sequencer::on_render() {
}

void Sequencer::ri_machine_registered(std::shared_ptr<RemoteInterface::RIMachine> ri_machine) {
	// we don't want to show non MachineSequener objects
	if(ri_machine->get_machine_type() != "MachineSequencer") return;

	int new_offset = (int)machine2sequence.size();
	SATAN_DEBUG("new_offset is %d\n", new_offset);

	KammoGUI::SVGCanvas::ElementReference layer(this, "layer1");
	machine2sequence[ri_machine] =
		Sequence::create_sequence(
			layer, sequence_graphic_template,
			ri_machine, new_offset);
}

void Sequencer::ri_machine_unregistered(std::shared_ptr<RemoteInterface::RIMachine> ri_machine) {
	// we don't have any non MachineSequener objects on file
	if(ri_machine->get_machine_type() != "MachineSequencer") return;

	auto seq = machine2sequence.find(ri_machine);
	if(seq != machine2sequence.end()) {
		machine2sequence.erase(seq);
	}
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

		static auto current_timelines = new TimeLines(cnvs);
		static auto current_sequencer = std::make_shared<Sequencer>(cnvs);

		RemoteInterface::RIMachine::register_ri_machine_set_listener(current_sequencer);
	}
}

KammoEventHandler_Instance(SequencerHandler);
