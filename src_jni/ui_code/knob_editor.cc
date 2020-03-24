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

#include "knob_editor.hh"
#include "svg_loader.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class KnobEditor::Knobinstance
 *
 ***************************/

KnobEditor::KnobInstance::KnobInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref,
	int _offset,
	std::shared_ptr<BMKnob> _knob
	)
	: svg_reference(elref)
	, knob(_knob)
	, offset(_offset)
{
}


auto KnobEditor::KnobInstance::create_knob_instance(
	std::shared_ptr<BMKnob> knob,
	KammoGUI::GnuVGCanvas::ElementReference &container,
	KammoGUI::GnuVGCanvas::ElementReference &knob_template,
	int offset) -> std::shared_ptr<KnobInstance> {

	char bfr[32];
	snprintf(bfr, 32, "knb_%p", knob.get());

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		container.add_element_clone(bfr, knob_template);

	SATAN_DEBUG("KnobEditor::KnobInstance::create_knob() -- bfr: %s\n", bfr);

	return std::make_shared<KnobInstance>(new_graphic, offset, knob);
}

/***************************
 *
 *  Class KnobEditor
 *
 ***************************/

KnobEditor::KnobEditor(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/knobEditor.svg"), cnvs)
{
	knob_container = KammoGUI::GnuVGCanvas::ElementReference(this, "knobContainer");
	knob_template = KammoGUI::GnuVGCanvas::ElementReference(this, "knobTemplate");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
}

void KnobEditor::on_resize() {
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

	// get data
	root.get_viewport(document_size);
	scaling = finger_height / document_size.height;
}

void KnobEditor::on_render() {
}
