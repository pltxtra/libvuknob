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

#include "knob_instance.hh"
#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static TapDetector tap_detector;

void KnobInstance::refresh_value_indicators() {
	char bfr[128];
	snprintf(bfr, 128, "%s: [%s]", knob->get_title().c_str(), knob->get_value_as_text().c_str());
	value_text.set_text_content(bfr);

	auto snapped_position = calculate_snapped_position(knob->get_value());
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(snapped_position, finger_height * 0.5);
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
		snapped_position - 1.5 * finger_width,
		thickness
		);
}

double KnobInstance::calculate_snapped_position(double value) {
	auto value_as_percentage = value / max;
	auto value_width = value_as_percentage * (canvas_width - 2.0 * (1.5 * finger_width));
	return 1.5 * finger_width + value_width;
}

void KnobInstance::handle_slide_event(const KammoGUI::MotionEvent &event) {
	double now_position = event.get_x();
	double snapped_position = calculate_snapped_position(knob->get_value());

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
	case KammoGUI::MotionEvent::ACTION_MOVE:
	{
		now_position -= snapped_position_offset;
		now_position -= 1.5 * finger_width;
		auto new_value_as_percentage = now_position / (canvas_width - 2.0 * (1.5 * finger_width));
		auto new_value = new_value_as_percentage * max;
		auto snapped_new_value = step * floor(new_value / step);

		auto trapped_value = snapped_new_value >= min ? snapped_new_value : min;
		trapped_value = trapped_value <= max ? trapped_value : max;

		knob->set_value_as_double(trapped_value);
	}
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		snapped_position_offset = now_position - snapped_position;
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		break;
	}
}

KnobInstance::KnobInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref,
	int _offset,
	std::shared_ptr<AbstractKnob> _knob
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

	value_slider_knob.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG(" slider knob for %p [%s]\n", this, knob->get_title().c_str());
			handle_slide_event(event);
			SATAN_DEBUG(" howdy!\n");
		}
		);

	knob->set_callback([this]() {
			SATAN_DEBUG(" callback for %p [%s]\n", this, knob->get_title().c_str());
			refresh_value_indicators();
		});
}

KnobInstance::~KnobInstance()
{
	knob->set_callback([](){});
	svg_reference.drop_element();
}

void KnobInstance::refresh_transformation(double _canvas_width, double _finger_width, double _finger_height) {
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
auto KnobInstance::create_knob_instance(
	std::shared_ptr<AbstractKnob> knob,
	KammoGUI::GnuVGCanvas::ElementReference &container,
	KammoGUI::GnuVGCanvas::ElementReference &knob_template,
	int offset) -> std::shared_ptr<KnobInstance> {

	char bfr[32];
	snprintf(bfr, 32, "knb_%p", knob.get());

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		container.add_element_clone(bfr, knob_template);
	new_graphic.set_display("inline");
	SATAN_DEBUG("KnobInstance::create_knob() -- bfr: %s\n", bfr);

	return std::make_shared<KnobInstance>(new_graphic, offset, knob);
}
