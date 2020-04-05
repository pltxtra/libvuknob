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
#include "popup_menu.hh"

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
	value_text = svg_reference.find_child_by_class("valueText");
	value_decrease_button = svg_reference.find_child_by_class("valueDecreaseButton");
	value_increase_button = svg_reference.find_child_by_class("valueIncreaseButton");
	value_base_bar = svg_reference.find_child_by_class("valueBaseBar");
	value_actual_bar = svg_reference.find_child_by_class("valueActualBar");
	value_slider_knob = svg_reference.find_child_by_class("valueSliderKnob");

	min = knob->get_min();
	max = knob->get_max();
	step = knob->get_step();

	value_decrease_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				SATAN_DEBUG("value decrease was tapped. [%p]\n", this);
				auto new_value = knob->get_value() - step;
				knob->set_value_as_double(new_value >= min ? new_value : min);
			}
		}
		);

	value_increase_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				SATAN_DEBUG("value increase was tapped. [%p]\n", this);
				auto new_value = knob->get_value() + step;
				knob->set_value_as_double(new_value <= max ? new_value : max);
			}
		}
		);

	knob->set_callback([this]() {
			SATAN_DEBUG(" callback for %p [%s]\n", this, knob->get_title().c_str());
			refresh_value_indicators();
		});
}

KnobEditor::KnobInstance::~KnobInstance()
{
	knob->set_callback([](){});
	svg_reference.drop_element();
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
	new_graphic.set_display("inline");
	SATAN_DEBUG("KnobEditor::KnobInstance::create_knob() -- bfr: %s\n", bfr);

	return std::make_shared<KnobInstance>(new_graphic, offset, knob);
}

void KnobEditor::KnobInstance::refresh_value_indicators() {
	char bfr[128];
	snprintf(bfr, 128, "%s: [%s]", knob->get_title().c_str(), knob->get_value_as_text().c_str());
	value_text.set_text_content(bfr);

	auto value_as_percentage = knob->get_value() / max;
	auto value_width = value_as_percentage * (canvas_width - 2.0 * (1.5 * finger_width));

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(1.5 * finger_width + value_width, finger_height * 0.5);
	value_slider_knob.set_transform(transform_t);

	double thickness = finger_height / 20.0;
	value_base_bar.set_rect_coords(
		1.5 * finger_width, finger_height * 0.5 - 0.5 * thickness,
		canvas_width - 2.0 * (1.5 * finger_width),
		thickness
		);
	thickness *= 3.0;
	value_actual_bar.set_rect_coords(
		1.5 * finger_width, finger_height * 0.5 - 0.5 * thickness,
		value_width,
		thickness
		);
}

void KnobEditor::KnobInstance::refresh_transformation(double _canvas_width, double _finger_width, double _finger_height) {
	canvas_width = _canvas_width;
	finger_width = _finger_width;
	finger_height = _finger_height;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(0.0, ((double)offset) * finger_height);
	svg_reference.set_transform(transform_t);
	transform_t.init_identity();
	transform_t.translate(canvas_width - finger_width, 0.0);
	value_increase_button.set_transform(transform_t);

	refresh_value_indicators();
}

/***************************
 *
 *  Class KnobEditor
 *
 ***************************/

void KnobEditor::refresh_knobs() {
	knob_instances.clear();
	int k = 0;
	for(auto knob : current_machine->get_knobs_for_group(current_group)) {
		SATAN_DEBUG("Creating instance for %s\n", knob->get_name().c_str());
		auto new_knob_instance = KnobInstance::create_knob_instance(knob, knob_container, knob_template, k++);
		knob_instances.push_back(new_knob_instance);
		new_knob_instance->refresh_transformation(canvas_w, finger_width, finger_height);
	}
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.translate(0.0, finger_height);
	knob_container.set_transform(transform_t);
}

void KnobEditor::internal_show(std::shared_ptr<BMachine> new_machine) {
	current_machine = new_machine;
	root.set_display("inline");

	SATAN_DEBUG("Got the machine known as %s (%p)\n", current_machine->get_name().c_str(), current_machine.get());
	groups = current_machine->get_knob_groups();

	current_group = groups.size() ? groups[0] : "";
	refresh_knobs();
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

				PopupMenu::show_menu(
					groups,
					[this](int selection) {
						SATAN_DEBUG("User selected item %s\n", groups[selection].c_str());
						current_group = groups[selection];
						refresh_knobs();
					}
					);
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
		singleton->knob_instances.clear();
		singleton->current_machine = nullptr;
	}
}

void KnobEditor::show(std::shared_ptr<BMachine> machine) {
	SATAN_DEBUG("KnobEditor::show() [%p]\n", singleton);
	if(singleton && machine) {
		singleton->internal_show(machine);
	}
}
