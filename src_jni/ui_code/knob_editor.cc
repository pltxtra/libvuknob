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
#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

KnobEditor* KnobEditor::singleton = nullptr;
static TapDetector tap_detector;

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

void KnobEditor::internal_show(std::shared_ptr<BMachine> machine) {
	root.set_display("inline");

	SATAN_DEBUG("Got the machine known as %s (%p)\n", machine->get_name().c_str(), machine.get());
	groups = machine->get_knob_groups();

	if(groups.size()) {
		current_group = groups[0];
		for(auto group : groups) {
			SATAN_DEBUG("   knobs in [%s]:\n", group.c_str());
			for(auto knob : machine->get_knobs_for_group(group)) {
				SATAN_DEBUG("   [%s]\n", knob->get_name().c_str());
			}
		}
	} else {
		current_group = "";
		SATAN_DEBUG("   all knobs:\n");
		for(auto knob : machine->get_knobs_for_group("")) {
			SATAN_DEBUG("   [%s]\n", knob->get_name().c_str());
		}
	}
}

KnobEditor::KnobEditor(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/knobsEditor.svg"), cnvs)
{
	knob_container = KammoGUI::GnuVGCanvas::ElementReference(this, "knobContainer");
	knob_template = KammoGUI::GnuVGCanvas::ElementReference(this, "knobTemplate");
	popup_container = KammoGUI::GnuVGCanvas::ElementReference(this, "popupTriggerContainer");
	popup_trigger = KammoGUI::GnuVGCanvas::ElementReference(this, "popupMenuTrigger");
	popup_text = KammoGUI::GnuVGCanvas::ElementReference(this, "popupTriggerTextContent");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

	popup_trigger.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				SATAN_DEBUG("popup trigger was tapped.\n");
			}
		}
		);
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

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.scale(scaling, scaling);
	root.set_transform(transform_t);

	transform_t.init_identity();
	transform_t.scale(canvas_width_fingers, 1.0);
	popup_trigger.set_transform(transform_t);
	transform_t.init_identity();
	transform_t.translate(finger_width, 0.0);
	popup_container.set_transform(transform_t);
}

void KnobEditor::on_render() {
	if(current_group == "") {
		popup_container.set_display("none");
	} else {
		popup_container.set_display("inline");
		popup_text.set_text_content(current_group);
	}
}

std::shared_ptr<KnobEditor> KnobEditor::get_knob_editor(KammoGUI::GnuVGCanvas* cnvs) {
	if(singleton) return singleton->shared_from_this();
	auto response = std::make_shared<KnobEditor>(cnvs);
	singleton = response.get();
	return response;
}

void KnobEditor::hide() {
	if(singleton) {
		singleton->root.set_display("none");
	}
}

void KnobEditor::show(std::shared_ptr<BMachine> machine) {
	SATAN_DEBUG("KnobEditor::show() [%p]\n", singleton);
	if(singleton && machine) {
		singleton->internal_show(machine);
	}
}
