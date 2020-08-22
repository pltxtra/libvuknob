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
 *  Class SettingsScreen
 *
 ***************************/

void SettingsScreen::create_knobs() {
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
	create_knobs();
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
			for(auto knob : knobs)
				if(knob->get_title() == "BPM")
					knob->new_value_exists();
		}
		);
}

void SettingsScreen::lpb_changed(int lpm) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(__PRETTY_FUNCTION__,
		[this]() {
			for(auto knob : knobs)
				if(knob->get_title() == "LPB")
					knob->new_value_exists();
		}
		);
}

void SettingsScreen::shuffle_factor_changed(int shuffle_factor) {
	KammoGUI::GnuVGCanvas::run_on_ui_thread(
		__PRETTY_FUNCTION__,
		[this, shuffle_factor]() {
			SATAN_ERROR("SettingsScreen::shuffle_factor_changed(%d)\n", shuffle_factor);
			for(auto knob : knobs)
				if(knob->get_title() == "Shuffle")
					knob->new_value_exists();
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
