/*
 * vu|KNOB
 * Copyright (C) 2020 by Anton Persson
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

#include "settings_screen.hh"
#include "svg_loader.hh"
#include "tap_detector.hh"
#include "../musical_constants.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

SettingsScreen* SettingsScreen::singleton = nullptr;
static TapDetector tap_detector;

/***************************
 *
 *  Class SettingsScreen::Knobinstance
 *
 ***************************/

SettingsScreen::KnobInstance::KnobInstance(
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
}

SettingsScreen::KnobInstance::~KnobInstance()
{
	svg_reference.drop_element();
}

auto SettingsScreen::KnobInstance::create_knob_instance(
	std::shared_ptr<BMKnob> knob,
	KammoGUI::GnuVGCanvas::ElementReference &container,
	KammoGUI::GnuVGCanvas::ElementReference &knob_template,
	int offset) -> std::shared_ptr<KnobInstance> {

	char bfr[32];
	snprintf(bfr, 32, "knb_%p", knob.get());

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		container.add_element_clone(bfr, knob_template);
	new_graphic.set_display("inline");
	SATAN_DEBUG("SettingsScreen::KnobInstance::create_knob() -- bfr: %s\n", bfr);

	return std::make_shared<KnobInstance>(new_graphic, offset, knob);
}

void SettingsScreen::KnobInstance::refresh_value_indicators() {
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

void SettingsScreen::KnobInstance::refresh_transformation(double _canvas_width, double _finger_width, double _finger_height) {
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
 *  Class SettingsScreen
 *
 ***************************/

void SettingsScreen::refresh_knobs() {
	knob_instances.clear();
	int k = 0;
	for(auto knob : knobs) {
		SATAN_DEBUG("Creating instance for %s\n", knob->get_title().c_str());
		auto new_knob_instance = KnobInstance::create_knob_instance(knob, knob_container, knob_template, k++);
		knob_instances.push_back(new_knob_instance);
		new_knob_instance->refresh_transformation(canvas_w, finger_width, finger_height);
	}
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.translate(0.0, 0.0);
	knob_container.set_transform(transform_t);
}

void SettingsScreen::internal_show() {
	root.set_display("inline");
	refresh_knobs();
}

SettingsScreen::SettingsScreen(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/settingsScreen.svg"), cnvs)
{
	knobs.push_back(
		std::make_shared<BMKnob>(
			__MIN_BPM__, __MAX_BPM__, "BPM",
			[this](double new_bpm) {
				if(auto gco = gco_w.lock()) {
					gco->set_bpm(new_bpm);
				}
				SATAN_DEBUG("Should set BPM now to: %f\n", new_bpm);
			},
			[this]() -> double {
				double value = 120.0;
				if(auto gco = gco_w.lock()) {
					value = (double)gco->get_bpm();
				}
				SATAN_DEBUG("Will return BPM here: %f\n", value);
				return value;
			}
			)
		);
	knobs.push_back(
		std::make_shared<BMKnob>(
			__MIN_LPB__, __MAX_LPB__, "LPB",
			[this](double new_lpb) {
				if(auto gco = gco_w.lock()) {
					gco->set_lpb(new_lpb);
				}
				SATAN_DEBUG("Should set LPB now to: %f\n", new_lpb);
			},
			[this]() -> double {
				double value = 4.0;
				if(auto gco = gco_w.lock()) {
					value = (double)gco->get_lpb();
				}
				SATAN_DEBUG("Will return LPB here: %f\n", value);
				return value;
			}
			)
		);
	knobs.push_back(
		std::make_shared<BMKnob>(
			__MIN_SHUFFLE__, __MAX_SHUFFLE__, "Shuffle",
			[this](double new_shuffle) {
				SATAN_DEBUG("Should set shuffle now to: %f\n", new_shuffle);
				if(auto gco = gco_w.lock()) {
					SATAN_DEBUG("Got GCO - will set shuffle now to: %f\n", new_shuffle);
					gco->set_shuffle_factor(new_shuffle);
				}
			},
			[this]() -> double {
				double value = 0.0;
				if(auto gco = gco_w.lock()) {
					value = (double)gco->get_shuffle_factor();
				}
				SATAN_DEBUG("Will return shuffle here: %f\n", value);
				return value;
			}
			)
		);
	knob_container = KammoGUI::GnuVGCanvas::ElementReference(this, "knobContainer");
	knob_template = KammoGUI::GnuVGCanvas::ElementReference(this, "knobTemplate");
	button_container = KammoGUI::GnuVGCanvas::ElementReference(this, "buttonContainer");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

	knob_template.set_display("none");
	root.set_display("none");
}

void SettingsScreen::on_resize() {
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

	double buttons_scaling = ((double)canvas_w) / (6.0 * finger_width);
	transform_t.init_identity();
	transform_t.scale(buttons_scaling, buttons_scaling);
	transform_t.translate(0.0, 3.0 * finger_height);
	button_container.set_transform(transform_t);
	SATAN_ERROR(" --------------- SetSc: %d -> %f\n", canvas_width_fingers, buttons_scaling);
}

void SettingsScreen::on_render() {
}

void SettingsScreen::object_registered(std::shared_ptr<GCO> _gco) {
	SATAN_DEBUG("---------------------------------- SettingsScreen got gco object...\n");
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, _gco]() {
			SATAN_DEBUG("-----------**************  SettingsScreen got gco object...\n");
			gco_w = _gco;
			if(auto gco = gco_w.lock()) {
				SATAN_DEBUG("-------------**************  SettingsScreen locked gco object...\n");
				gco->add_global_control_listener(shared_from_this());
			}
		}
		);
}

void SettingsScreen::object_unregistered(std::shared_ptr<GCO> _gco) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this, _gco]() {
			gco_w.reset();
		}
		);
}

void SettingsScreen::bpm_changed(int bpm) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this]() {
			refresh_knobs();
		}
		);
}

void SettingsScreen::lpb_changed(int lpm) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this]() {
			refresh_knobs();
		}
		);
}

void SettingsScreen::shuffle_factor_changed(int shuffle_factor) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(
		__PRETTY_FUNCTION__,
		[this, shuffle_factor]() {
			SATAN_ERROR("SettingsScreen::shuffle_factor_changed(%d)\n", shuffle_factor);
			refresh_knobs();
		}
		);
}

std::shared_ptr<SettingsScreen> SettingsScreen::get_settings_screen(KammoGUI::GnuVGCanvas* cnvs) {
	if(singleton) return singleton->shared_from_this();
	auto response = std::make_shared<SettingsScreen>(cnvs);
	singleton = response.get();
	return response;
}

void SettingsScreen::hide() {
	if(singleton) {
		singleton->root.set_display("none");
		singleton->knob_instances.clear();
	}
}

void SettingsScreen::show() {
	SATAN_DEBUG("SettingsScreen::show() [%p]\n", singleton);
	if(singleton) {
		singleton->internal_show();
	}
}
