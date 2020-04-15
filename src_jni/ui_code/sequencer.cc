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
#include "loop_settings.hh"
#include "pattern_editor.hh"
#include "knob_editor.hh"
#include "popup_window.hh"
#include "popup_menu.hh"
#include "gnuvg_corner_button.hh"
#include "svg_loader.hh"
#include "common.hh"
#include "fling_animation.hh"
#include "main_menu.hh"

#include "../engine_code/sequence.hh"
#include "../engine_code/client.hh"

#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static std::shared_ptr<TimeLines> timelines;
static std::shared_ptr<Sequencer> sequencer;
static std::shared_ptr<GnuVGCornerButton> plus_button;
static std::shared_ptr<GnuVGCornerButton> return_button;
static std::shared_ptr<PatternEditor> pattern_editor;
static std::shared_ptr<KnobEditor> knob_editor;
static std::shared_ptr<LoopSettings> loop_settings;
static std::shared_ptr<MainMenu> main_menu;
static TapDetector tap_detector;

/***************************
 *
 *  Class SequencerMenu
 *
 ***************************/

SequencerMenu::SequencerMenu(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/sequencerMachineMenu.svg"), cnvs)
{
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
}

void SequencerMenu::on_resize() {
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
}

void SequencerMenu::on_render() {}

/***************************
 *
 *  Class Sequencer::PatternInstance
 *
 ***************************/

Sequencer::PatternInstance::PatternInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref,
	const RIPatternInstance &_instance_data,
	std::shared_ptr<RISequence> ri_seq,
	std::function<void(const InstanceEvent &)> _event_callback,
	std::shared_ptr<TimeLines::ZoomContext> _zoom_context
	)
	: svg_reference(elref)
	, instance_data(_instance_data)
	, event_callback(_event_callback)
	, zoom_context(_zoom_context)
{
	instance_graphic = svg_reference.find_child_by_class("instance_graphic");
	pattern_id_graphic = svg_reference.find_child_by_class("id_graphic");

	std::stringstream ss;
	ss << instance_data.pattern_id;
	pattern_id_graphic.set_text_content(ss.str());

	svg_reference.set_event_handler(
		[this, ri_seq](SVGDocument *source,
			       KammoGUI::GnuVGCanvas::ElementReference *e,
			       const KammoGUI::MotionEvent &event) {
			on_instance_event(ri_seq, event);
		}
		);
}

void Sequencer::PatternInstance::on_instance_event(std::shared_ptr<RISequence> ri_seq,
						   const KammoGUI::MotionEvent &event) {
	timelines->process_external_scroll_event(event);
	auto last_moving_offset = moving_offset;
	moving_offset = 0;

	if(tap_detector.analyze_events(event)) {
		KammoGUI::GnuVGCanvas::SVGRect r;
		svg_reference.get_boundingbox(r);

		InstanceEvent e;
		e.type = InstanceEventType::tapped;
		e.x = r.x;
		e.y = r.y;
		event_callback(e);
		return;
	}

	auto event_current_x = event.get_x();
	auto current_sequence_position = timelines->get_sequence_line_position_at(event_current_x);

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
	{
		InstanceEvent e;
		e.type = InstanceEventType::finger_on;
		event_callback(e);
	}
		start_at_sequence_position = current_sequence_position;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		moving_offset = current_sequence_position - start_at_sequence_position;
		SATAN_DEBUG("move offset: %d\n", moving_offset);
		refresh_visibility();
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
	{
		InstanceEvent e;
		e.type = InstanceEventType::moved;
		e.moving_offset = last_moving_offset;
		event_callback(e);
	}
		break;
	}
}

void Sequencer::PatternInstance::refresh_visibility() {
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	float x = line_width * (double)(instance_data.start_at + moving_offset);
	transform_t.init_identity();
	transform_t.translate(x, 0.0);
	svg_reference.set_transform(transform_t);

	float w = (line_width * (double)(instance_data.stop_at - instance_data.start_at));
	instance_graphic.set_attribute("width", w - 2);
}

void Sequencer::PatternInstance::calculate_visibility(double _line_width,
						      int _minimum_visible_line,
						      int _maximum_visible_line) {
	SATAN_DEBUG("instance(%d, %d) ==> calculate_visibility(%f, %d, %d) - set_attribute('width', %f)\n",
		    instance_data.start_at,
		    instance_data.stop_at,
		    _line_width,
		    _minimum_visible_line,
		    _maximum_visible_line,
		    _line_width * (double)(instance_data.stop_at - instance_data.start_at)
		);
	line_width = _line_width;
	minimum_visible_line = _minimum_visible_line;
	maximum_visible_line = _maximum_visible_line;
	refresh_visibility();
}

std::shared_ptr<Sequencer::PatternInstance> Sequencer::PatternInstance::create_new_pattern_instance(
	const RIPatternInstance &_instance_data,
	KammoGUI::GnuVGCanvas::ElementReference &parent,
	int sequence_line_width, double height,
	std::shared_ptr<RISequence> ri_seq,
	std::function<void(const InstanceEvent &e)> event_callback,
	std::shared_ptr<TimeLines::ZoomContext> _zoom_context
	)
{
	std::stringstream ss_new_id;
	ss_new_id << "pattern_instance_" << _instance_data.pattern_id;

	std::stringstream ss;

	float w = (sequence_line_width * (_instance_data.stop_at - _instance_data.start_at));

	ss << ""
	   << "<g "
	   << " id=\"" << ss_new_id.str() << "\" "
	   << "> "
	   << "  <rect "
	   << "   style=\"fill:#ffa123;fill-opacity:1.0\" "
	   << "   class=\"instance_graphic\""
	   << "   width=\"" << w << "\" "
	   << "   height=\"" << height << "\" "
	   << "   x=\"0.0\" "
	   << "   y=\"0.0\" "
	   << "  />"
	   << "  <text "
	   << "   x=\"10.0\" "
	   << "   y=\"30.0\" "
	   << "   class=\"id_graphic\" "
	   << "   style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:25px;line-height:0%;font-family:Roboto;-inkscape-font-specification:Roboto;letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none\" "
	   << "   >0</text> "
	   << "</g>"
	   << ""
		;

	parent.add_svg_child(ss.str());

	KammoGUI::GnuVGCanvas::ElementReference elref(&parent, ss_new_id.str());

	auto rval = std::make_shared<PatternInstance>(elref, _instance_data, ri_seq, event_callback, _zoom_context);

	return rval;
}

/***************************
 *
 *  Class Sequencer::Sequence
 *
 ***************************/

Sequencer::Sequence::Sequence(KammoGUI::GnuVGCanvas::ElementReference elref,
			      std::shared_ptr<RISequence> _ri_seq,
			      int _offset)
	: KammoGUI::GnuVGCanvas::ElementReference(elref)
	, ri_seq(_ri_seq)
	, offset(_offset)
{
	SATAN_DEBUG("Sequencer::Sequence::Sequence()\n");
	set_display("inline");

	SATAN_DEBUG("Sequencer::Sequence::Sequence() --> ri_seq->get_name() = %s\n",
		    ri_seq->get_name().c_str());

	find_child_by_class("machineId").set_text_content(ri_seq->get_name());

	sequence_background = find_child_by_class("seqBackground");
	sequence_background.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			on_sequence_event(event);
		}
		);

	button_background = find_child_by_class("machineButtonBackground");
	controlls_button = find_child_by_class("controllsButton");
	mute_button = find_child_by_class("muteButton");
	envelope_button = find_child_by_class("envelopeButton");

	button_background.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			sequencer->vertical_scroll_event(event);
		}
		);
	controlls_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				SATAN_DEBUG("controlls_button was tapped.\n");
				auto destination_name = ri_seq->get_name();
				SATAN_DEBUG("Trying to get the machine known as %s\n", destination_name.c_str());
				auto destination = RemoteInterface::ClientSpace::BaseMachine::get_machine_by_name(destination_name);
				KnobEditor::show(destination);

				plus_button->hide();
				loop_settings->hide();
				sequencer->hide_all();
				timelines->hide_all();
				return_button->show();

				return;
			}
			sequencer->vertical_scroll_event(event);
		}
		);
	mute_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			sequencer->vertical_scroll_event(event);
		}
		);
	envelope_button.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
		       KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
		       const KammoGUI::MotionEvent &event) {
			sequencer->vertical_scroll_event(event);
		}
		);
}

void Sequencer::Sequence::on_sequence_event(const KammoGUI::MotionEvent &event) {
	timelines->process_external_scroll_event(event);

	auto evt_x = event.get_x();
	auto evt_y = event.get_y();
	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		display_action = true;
		event_current_x = event_start_x = evt_x;
		event_current_y = event_start_y = evt_y;

		start_at_sequence_position = timelines->get_sequence_line_position_at(event.get_x());

		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		event_current_x = evt_x;
		event_current_y = evt_y;

		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		stop_at_sequence_position = timelines->get_sequence_line_position_at(event.get_x());
		if(evt_x > event_start_x) {
			if(start_at_sequence_position < 0)
				start_at_sequence_position = 0;
			if(stop_at_sequence_position < 0)
				stop_at_sequence_position = 0;

			if(start_at_sequence_position > stop_at_sequence_position) {
				auto t = start_at_sequence_position;
				start_at_sequence_position = stop_at_sequence_position;
				stop_at_sequence_position = t;
			}
			if(start_at_sequence_position != stop_at_sequence_position) {
				SATAN_DEBUG("Directly adding pattern instance for machine...(%d)\n", active_pattern_id);
				ri_seq->insert_pattern_in_sequence(
					active_pattern_id,
					start_at_sequence_position, -1,
					stop_at_sequence_position);
			}
		}
		display_action = false;
		break;
	}

	auto newPieceIndicator = find_child_by_class("newPieceIndicator");

	newPieceIndicator.set_display(display_action ? "inline" : "none");
	if(display_action) {
		float b_x, e_x;
		float start_x = timelines->get_pixel_value_for_sequence_line_position(start_at_sequence_position);
		if(event_current_x < start_x) {
			b_x = event_current_x;
			e_x = start_x - event_current_x;
		} else {
			b_x = start_x;
			e_x = event_current_x - start_x;
		}
		SATAN_DEBUG("b_x: %f, e_x: %f\n", b_x, e_x);
		newPieceIndicator.set_rect_coords(b_x, 0, e_x, height);
	}
}

std::shared_ptr<TimeLines::ZoomContext> Sequencer::Sequence::require_zoom_context(uint32_t pattern_id) {
	auto zoom_context = pattern_zoom_contexts.find(pattern_id);
	if(zoom_context == pattern_zoom_contexts.end())
		pattern_zoom_contexts[pattern_id] = std::make_shared<TimeLines::ZoomContext>();

	return pattern_zoom_contexts[pattern_id];
}

void Sequencer::Sequence::set_active_pattern_id(uint32_t pattern_id) {
	active_pattern_id = pattern_id;
}

std::shared_ptr<Sequencer::PatternInstance> Sequencer::Sequence::get_pattern_instance(int start_at) {
	std::shared_ptr<PatternInstance> pi;
	auto found = instances.find(start_at);
	if(found != instances.end()) {
		pi = found->second;
	}
	return pi;
}

void Sequencer::Sequence::instance_added(const RIPatternInstance& instance){
	KammoGUI::run_on_GUI_thread(
		[this, instance]() {
			auto event_callback = [this, instance](const InstanceEvent &e) {
				switch(e.type) {
				case InstanceEventType::finger_on:
				break;
				case InstanceEventType::moved:
				{
					auto new_start_at = instance.start_at + e.moving_offset;
					auto length = instance.stop_at - instance.start_at;
					if(new_start_at < 0) new_start_at = 0;

					ri_seq->delete_pattern_from_sequence(instance);
					ri_seq->insert_pattern_in_sequence(
						instance.pattern_id,
						new_start_at, instance.loop_length,
						new_start_at + length);
				}
				break;
				case InstanceEventType::tapped:
				{
					auto found = instances.find(instance.start_at);
					if(found != instances.end()) {
						sequencer->focus_on_pattern_instance(
							e.x, e.y,
							ri_seq,
							instance.start_at);
					}
				}
				}
			};

			auto found_zoom_context = require_zoom_context(instance.pattern_id);

			SATAN_DEBUG("Instance was added for machine...(%d)\n", instance.pattern_id);

			auto instanceContainer = find_child_by_class("instanceContainer");
			auto i = PatternInstance::create_new_pattern_instance(
				instance,
				instanceContainer,
				timelines->get_horizontal_pixels_per_line(),
				height,
				ri_seq,
				event_callback,
				found_zoom_context
				);
			instances[instance.start_at] = i;
			timelines->call_scroll_callbacks();
			i->calculate_visibility(
				line_width, minimum_visible_line, maximum_visible_line
				);

			sequencer->refresh_focus(ri_seq, instance.start_at);
		}
		);
}

void Sequencer::Sequence::instance_deleted(const RIPatternInstance& original_i) {
	RIPatternInstance instance = original_i;
	KammoGUI::run_on_GUI_thread(
		[this, instance]() {
			SATAN_DEBUG("::instance_deleted()\n");

			auto instance_to_erase = instances.find(instance.start_at);
			if(instance_to_erase != instances.end()) {
				instances.erase(instance_to_erase);
			}
		}
		);
}

void Sequencer::Sequence::set_graphic_parameters(double graphic_scaling_factor,
						 double _width, double _height,
						 double canvas_w, double canvas_h) {
	width = _width; height = _height * graphic_scaling_factor;

	inverse_scaling_factor = 1.0 / graphic_scaling_factor;

	find_child_by_class("seqBackground").set_rect_coords(
		width, 0, canvas_w, height);

	auto instance_window = find_child_by_class("instanceWindow");
	SATAN_DEBUG("instanceWindow found! setting w/h to %f/%f", canvas_w, height);

	instance_window.set_attribute("x", 0);
	instance_window.set_attribute("y", 0);
	instance_window.set_attribute("width", canvas_w);
	instance_window.set_attribute("height", height);

	// initiate transform_t
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;

	transform_t.init_identity();
	transform_t.translate(0.0, (double)(1 + offset) * height);
	set_transform(transform_t);

	SATAN_DEBUG("scaling: %f, offset: %d\n", graphic_scaling_factor, offset);

	transform_t.init_identity();
	transform_t.scale(graphic_scaling_factor, graphic_scaling_factor);
	auto button_container = find_child_by_class("buttonContainer");
	button_container.set_transform(transform_t);
}


void Sequencer::Sequence::on_scroll(double _line_width,
				    double _line_offset,
				    int _minimum_visible_line,
				    int _maximum_visible_line) {
	line_width = _line_width;
	line_offset = _line_offset;
	minimum_visible_line = _minimum_visible_line;
	_maximum_visible_line = _maximum_visible_line;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(line_offset, 0.0);

	auto instanceContainer = find_child_by_class("instanceContainer");
	instanceContainer.set_transform(transform_t);

	for(auto p : instances) {
		auto inst = p.second;

		inst->calculate_visibility(
			line_width, minimum_visible_line, maximum_visible_line
			);
	}
}

auto Sequencer::Sequence::create_sequence(
	KammoGUI::GnuVGCanvas::ElementReference &root,
	KammoGUI::GnuVGCanvas::ElementReference &sequence_graphic_template,
	std::shared_ptr<RISequence> ri_sequence,
	int offset) -> std::shared_ptr<Sequence> {

	char bfr[32];
	snprintf(bfr, 32, "seq_%p", ri_sequence.get());

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		root.add_element_clone(bfr, sequence_graphic_template);

	SATAN_DEBUG("Sequencer::Sequence::create_sequence() -- bfr: %s\n", bfr);

	auto new_sequence = std::make_shared<Sequence>(new_graphic, ri_sequence, offset);
	ri_sequence->add_sequence_listener(new_sequence);
	return new_sequence;
}

/***************************
 *
 *  Class Sequencer
 *
 ***************************/

Sequencer::Sequencer(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/sequencerMachine.svg"), cnvs)
{
	sequencer_container = KammoGUI::GnuVGCanvas::ElementReference(this, "sequencerContainer");
	sequence_graphic_template = KammoGUI::GnuVGCanvas::ElementReference(this, "sequencerMachineTemplate");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

	sequence_graphic_template.set_display("none");
}

void Sequencer::prepare_sequencer(KammoGUI::GnuVGCanvas* cnvs) {
	sequencer_menu = std::make_shared<SequencerMenu>(cnvs);

	pattern_id_container = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "patternIdContainer");
	pattern_id_plus = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "patternIdPlus");
	pattern_id_minus = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "patternIdMinus");
	pattern_id_text = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "patternIdText");
	trashcan_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "trashcanIcon");
	notes_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "notesIcon");
	loop_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "loopIcon");
	loop_enabled_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "loopEnabled");
	loop_disabled_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "loopDisabled");
	length_icon = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "lengthIcon");
	sequencer_shade = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "sequencerShade");
	tapped_instance = KammoGUI::GnuVGCanvas::ElementReference(sequencer_menu.get(), "tappedInstance");

	trashcan_icon.set_display("none");
	notes_icon.set_display("none");
	loop_icon.set_display("none");
	length_icon.set_display("none");
	sequencer_shade.set_display("none");
	pattern_id_container.set_display("none");

	timelines->add_scroll_callback(
		[this](double _line_width, double _line_offset,
		       int _minimum_visible_line,
		       int _maximum_visible_line) {
			for(auto m2s : machine2sequence) {
				m2s.second->on_scroll(
					_line_width, _line_offset,
					_minimum_visible_line,
					_maximum_visible_line
					);
			}
		}
		);
}

void Sequencer::set_active_pattern_id(std::weak_ptr<RISequence> ri_seq_w, uint32_t pattern_id) {
	std::shared_ptr<PatternInstance> ri_pi;
	if(auto ri_seq = ri_seq_w.lock()) {
		auto found = machine2sequence.find(ri_seq);
		if(found != machine2sequence.end()) {
			found->second->set_active_pattern_id(pattern_id);
		}
	}
}

std::shared_ptr<Sequencer::PatternInstance> Sequencer::get_tapped_pattern_instance(std::weak_ptr<RISequence> ri_seq_w) {
	std::shared_ptr<PatternInstance> ri_pi;
	if(auto ri_seq = ri_seq_w.lock()) {
		auto found = machine2sequence.find(ri_seq);
		if(found != machine2sequence.end()) {
			ri_pi = found->second->get_pattern_instance(tapped_instance_start_at);
		}
	}
	return ri_pi;
}

void Sequencer::refresh_focus(std::weak_ptr<RISequence>ri_seq_w, int instance_start_at) {
	auto ri_seq_A = ri_seq_w.lock();
	auto ri_seq_B = tapped_sequence_w.lock();
	if(!(ri_seq_A == ri_seq_B && instance_start_at == tapped_instance_start_at))
		return;
	if(auto instance = get_tapped_pattern_instance(tapped_sequence_w)) {
		auto instance_data = instance->data();
		auto instance_length = instance_data.stop_at - instance_data.start_at;
		auto instance_start_pixel = timelines->get_pixel_value_for_sequence_line_position(instance_data.start_at);
		set_active_pattern_id(tapped_sequence_w, instance_data.pattern_id);
		tapped_instance.set_rect_coords(
			instance_start_pixel, icon_anchor_y,
			pixels_per_line * instance_length,
			finger_height
			);

		SATAN_DEBUG("Loop length: %d\n", instance_data.loop_length);
		double loop_translation = (double)instance_data.loop_length;
		bool loop_enabled = true;
		if(instance_data.loop_length < 0) {
			loop_translation = 0.0;
			loop_enabled = false;
		}
		KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
		transform_t.init_identity();
		transform_t.translate(instance_start_pixel + pixels_per_line * loop_translation, icon_anchor_y);
		loop_icon.set_transform(transform_t);
		transform_t.init_identity();
		transform_t.translate(instance_start_pixel + pixels_per_line * instance_length , icon_anchor_y + 0.5 * finger_height);
		length_icon.set_transform(transform_t);
		SATAN_ERROR("INITIAL %f, %f, %f, %d, %f\n",
			    icon_anchor_x,
			    icon_anchor_y,
			    pixels_per_line,
			    instance_length,
			    finger_height);
		set_pattern_id_text(instance_data.pattern_id);
		loop_enabled_icon.set_display(loop_enabled ? "inline" : "none");
		loop_disabled_icon.set_display(loop_enabled ? "none" : "inline");
	}
}

void Sequencer::focus_on_pattern_instance(double _icon_anchor_x, double _icon_anchor_y,
					  std::weak_ptr<RISequence>ri_seq_w,
					  int instance_start_at
	) {
	icon_anchor_x = _icon_anchor_x;
	icon_anchor_y = _icon_anchor_y;
	pixels_per_line = timelines->get_horizontal_pixels_per_line();
	icon_anchor_x_line_position = timelines->get_sequence_line_position_at(icon_anchor_x);
	tapped_sequence_w = ri_seq_w;

	loop_settings->hide();
	plus_button->hide();
	tapped_instance_start_at = instance_start_at;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(icon_anchor_x, icon_anchor_y + finger_height);
	trashcan_icon.set_transform(transform_t);
	transform_t.translate(finger_width, 0.0);
	notes_icon.set_transform(transform_t);
	transform_t.init_identity();
	transform_t.translate(icon_anchor_x, icon_anchor_y + 2.0 * finger_height);
	pattern_id_container.set_transform(transform_t);

	show_elements({&trashcan_icon, &notes_icon, &loop_icon, &sequencer_shade,
				&length_icon, &tapped_instance, &pattern_id_container
				});

	sequencer_shade.set_event_handler(
		[](SVGDocument *source,
		   KammoGUI::GnuVGCanvas::ElementReference *e,
		   const KammoGUI::MotionEvent &event) {
			/* ignored during transition */
			SATAN_DEBUG("Ignoring event handler during hiding...\n");
		}
		);
	sequencer_shade.set_display("inline");
	sequencer_shade.set_style("opacity:1.0");
	sequencer_shade.set_rect_coords(0, 0, canvas_w, canvas_h);

	refresh_focus(tapped_sequence_w, tapped_instance_start_at);

	auto loop_drag_completed_callback =
		[this, ri_seq_w](int new_loop_length) {
		auto instance = get_tapped_pattern_instance(ri_seq_w);
		auto ri_seq = ri_seq_w.lock();
		if(instance && ri_seq) {
			auto instance_data = instance->data();
			SATAN_ERROR("new loop length: %d\n", new_loop_length);

			auto old_loop_length = instance_data.loop_length;
			if(new_loop_length != old_loop_length) {
				if(new_loop_length <= 0)
					new_loop_length = -1;
				ri_seq->delete_pattern_from_sequence(instance_data);
				ri_seq->insert_pattern_in_sequence(
					instance_data.pattern_id,
					instance_data.start_at, new_loop_length,
					instance_data.stop_at);
			}
		}

		sequencer->show_sequencers({&loop_icon, &tapped_instance, &sequencer_shade, &pattern_id_container});
		loop_settings->show();
		plus_button->show();
	};
	loop_icon.set_event_handler(
		[this, ri_seq_w, loop_drag_completed_callback](
			 SVGDocument *source,
			 KammoGUI::GnuVGCanvas::ElementReference *e,
			 const KammoGUI::MotionEvent &event
			 ) {
			if(event.get_action() == KammoGUI::MotionEvent::ACTION_DOWN) {
				sequencer->hide_elements({&trashcan_icon, &notes_icon, &length_icon});
			}
			if(auto instance = get_tapped_pattern_instance(ri_seq_w)) {
				auto instance_data = instance->data();
				auto instance_length = instance_data.stop_at - instance_data.start_at;
				drag_loop_icon(event,
					       icon_anchor_x_line_position, icon_anchor_y,
					       pixels_per_line,
					       instance_data.start_at,
					       instance_length,
					       loop_drag_completed_callback);
			}
		}
		);

	auto length_drag_completed_callback =
		[this, ri_seq_w](int drag_offset_in_lines) {
		auto instance = get_tapped_pattern_instance(ri_seq_w);
		auto ri_seq = ri_seq_w.lock();
		if(instance && ri_seq) {
			auto instance_data = instance->data();
			SATAN_DEBUG("drag_offset_in_lines: %d\n", drag_offset_in_lines);

			auto new_start_at = instance_data.start_at;
			auto old_length = instance_data.stop_at - instance_data.start_at;
			auto new_length = old_length + drag_offset_in_lines;
			if(new_length > 0 && new_length != old_length) {
				ri_seq->delete_pattern_from_sequence(instance_data);
				ri_seq->insert_pattern_in_sequence(
					instance_data.pattern_id,
					new_start_at, instance_data.loop_length,
					new_start_at + new_length);
			}
		}

		sequencer->show_sequencers({&length_icon, &tapped_instance, &sequencer_shade, &pattern_id_container});
		loop_settings->show();
		plus_button->show();
	};
	length_icon.set_event_handler(
		[this, ri_seq_w, length_drag_completed_callback](
			 SVGDocument *source,
			 KammoGUI::GnuVGCanvas::ElementReference *e,
			 const KammoGUI::MotionEvent &event
			 ) {
			if(event.get_action() == KammoGUI::MotionEvent::ACTION_DOWN) {
				sequencer->hide_elements({&trashcan_icon, &notes_icon, &loop_icon});
			}
			if(auto instance = get_tapped_pattern_instance(ri_seq_w)) {
				auto instance_data = instance->data();
				auto instance_length = instance_data.stop_at - instance_data.start_at;
				drag_length_icon(event,
						 icon_anchor_x_line_position, icon_anchor_y,
						 pixels_per_line,
						 instance_data.start_at,
						 instance_length,
						 length_drag_completed_callback);
			}
		}
		);

	auto on_completion = [this, ri_seq_w]() {
		SATAN_DEBUG("  completed hiding - injecting new event handlers for icons...\n");

		notes_icon.set_event_handler(
			[this, ri_seq_w](
				SVGDocument *source,
				KammoGUI::GnuVGCanvas::ElementReference *e,
				const KammoGUI::MotionEvent &event
				) {
				auto restore_sequencer = [this]() {
					timelines->use_zoom_context(nullptr);
					root.set_display("inline");
					timelines->show_loop_markers();
					loop_settings->show();
					plus_button->show();
				};

				if(tap_detector.analyze_events(event)) {
					sequencer->show_sequencers({});
					auto instance = get_tapped_pattern_instance(ri_seq_w);
					auto ri_seq = ri_seq_w.lock();
					if(instance && ri_seq) {
						timelines->use_zoom_context(instance->zoom_context);

						PatternEditor::show(restore_sequencer,
								    ri_seq,
								    instance->data().pattern_id);
						root.set_display("none");
						timelines->hide_loop_markers();
						return_button->show();
					}
				}
			}
			);

		trashcan_icon.set_event_handler(
			[this, ri_seq_w](SVGDocument *source,
			       KammoGUI::GnuVGCanvas::ElementReference *e,
			       const KammoGUI::MotionEvent &event) {
				if(tap_detector.analyze_events(event)) {
					auto instance = get_tapped_pattern_instance(ri_seq_w);
					auto ri_seq = ri_seq_w.lock();
					if(instance && ri_seq) {
						ri_seq->delete_pattern_from_sequence(instance->data());
					}

					sequencer->show_sequencers({});
					loop_settings->show();
					plus_button->show();
				}
			}
			);

		sequencer_shade.set_event_handler(
			[this](SVGDocument *source,
			       KammoGUI::GnuVGCanvas::ElementReference *e,
			       const KammoGUI::MotionEvent &event) {
				if(tap_detector.analyze_events(event)) {
					sequencer->show_sequencers({});
					loop_settings->show();
					plus_button->show();
				}
			}
			);

		pattern_id_plus.set_event_handler(
			[this, ri_seq_w](SVGDocument *source,
			       KammoGUI::GnuVGCanvas::ElementReference *e,
			       const KammoGUI::MotionEvent &event) {
				if(tap_detector.analyze_events(event)) {
					SATAN_DEBUG("Pattern id plus!\n");
					auto instance = get_tapped_pattern_instance(ri_seq_w);
					auto ri_seq = ri_seq_w.lock();
					if(instance && ri_seq) {
						SATAN_DEBUG("Pattern id plus - 1!\n");
						auto instance_data = instance->data();
						auto instance_pattern_id = instance_data.pattern_id;
						auto instance_start = instance_data.start_at;
						auto instance_loop_length = instance_data.loop_length;
						auto instance_stop = instance_data.stop_at;
						if(instance_pattern_id + 1 < IDAllocator::NO_ID_AVAILABLE) {
							SATAN_DEBUG("Pattern id plus - 2!\n");
							ri_seq->delete_pattern_from_sequence(instance->data());
							ri_seq->insert_pattern_in_sequence(
								instance_data.pattern_id + 1,
								instance_data.start_at,
								instance_data.loop_length,
								instance_data.stop_at);
						}
					}
				}
			}
			);

		pattern_id_minus.set_event_handler(
			[this, ri_seq_w](SVGDocument *source,
			       KammoGUI::GnuVGCanvas::ElementReference *e,
			       const KammoGUI::MotionEvent &event) {
				if(tap_detector.analyze_events(event)) {
					SATAN_DEBUG("Pattern id minus!\n");
					auto instance = get_tapped_pattern_instance(ri_seq_w);
					auto ri_seq = ri_seq_w.lock();
					if(instance && ri_seq) {
						SATAN_DEBUG("Pattern id minus - 1!\n");
						auto instance_data = instance->data();
						auto instance_pattern_id = instance_data.pattern_id;
						auto instance_start = instance_data.start_at;
						auto instance_loop_length = instance_data.loop_length;
						auto instance_stop = instance_data.stop_at;
						if(instance_pattern_id > 0) {
							SATAN_DEBUG("Pattern id minus - 2!\n");
							ri_seq->delete_pattern_from_sequence(instance->data());
							ri_seq->insert_pattern_in_sequence(
								instance_pattern_id - 1,
								instance_start, instance_loop_length,
								instance_stop);
						}
					}
				}
			}
			);

	};

	auto shade_transition = new KammoGUI::SimpleAnimation(
		TRANSITION_TIME,
		[this, on_completion](float progress) mutable {
			SATAN_DEBUG("hide transition animation %f...\n", progress);
			if(progress >= 1.0f) {
				on_completion();
			}
		}
		);
	sequencer->start_animation(shade_transition);
}

void Sequencer::show_sequencers(std::vector<KammoGUI::GnuVGCanvas::ElementReference *> elements_to_hide) {
	if(elements_to_hide.size() == 0) {
		elements_to_hide = {&trashcan_icon, &notes_icon, &loop_icon, &sequencer_shade,
				    &length_icon, &tapped_instance, &pattern_id_container};
	}
	hide_elements(elements_to_hide);

	sequencer_shade.set_event_handler(
		[](SVGDocument *source,
		   KammoGUI::GnuVGCanvas::ElementReference *e,
		   const KammoGUI::MotionEvent &event) {
			/* ignored during transition */
			SATAN_DEBUG("ignoring event handler during show...\n");
		}
		);
}

void Sequencer::drag_length_icon(const KammoGUI::MotionEvent &event,
				 double icon_anchor_x_line_position,
				 double icon_anchor_y,
				 double pixels_per_line,
				 int instance_start,
				 int instance_length,
				 std::function<void(int)> drag_length_completed_callback) {
	timelines->process_external_scroll_event(event);

	auto icon_anchor_x = timelines->get_pixel_value_for_sequence_line_position(icon_anchor_x_line_position);
	auto instance_start_pixel = timelines->get_pixel_value_for_sequence_line_position(instance_start);
	auto current_line_position = timelines->get_sequence_line_position_at(event.get_x());
	auto drag_offset = current_line_position - drag_event_start_line;
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		drag_event_start_line = current_line_position;
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_ERROR("CURRENT %d, %f, %f, %d, %f\n",
			    icon_anchor_x,
			    icon_anchor_y,
			    pixels_per_line,
			    instance_length,
			    finger_height);
		transform_t.init_identity();
		transform_t.translate(instance_start_pixel + pixels_per_line * (instance_length + drag_offset), icon_anchor_y + 0.5 * finger_height);
		length_icon.set_transform(transform_t);
		tapped_instance.set_rect_coords(
			instance_start_pixel, icon_anchor_y,
			pixels_per_line * (instance_length + drag_offset),
			finger_height
			);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		drag_length_completed_callback(drag_offset);
		break;
	}
}

void Sequencer::drag_loop_icon(const KammoGUI::MotionEvent &event,
			       double icon_anchor_x_line_position,
			       double icon_anchor_y,
			       double pixels_per_line,
			       int instance_start,
			       int instance_length,
			       std::function<void(int)> drag_loop_completed_callback) {
	timelines->process_external_scroll_event(event);

	auto icon_anchor_x = timelines->get_pixel_value_for_sequence_line_position(icon_anchor_x_line_position);
	auto instance_start_pixel = timelines->get_pixel_value_for_sequence_line_position(instance_start);
	auto current_line_position = timelines->get_sequence_line_position_at(event.get_x());
	auto drag_offset = current_line_position - drag_event_start_line;
	auto new_loop_length = instance_start + drag_offset;
	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;

	if(new_loop_length > instance_length) {
		new_loop_length = instance_length;
	}
	if(new_loop_length < 0) {
		new_loop_length = 0;
	}
	double loop_translation = (double)new_loop_length;
	bool loop_enabled = loop_translation <= 0.0 ? false : true;
	loop_enabled_icon.set_display(loop_enabled ? "inline" : "none");
	loop_disabled_icon.set_display(loop_enabled ? "none" : "inline");

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_DOWN:
		drag_event_start_line = current_line_position;
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		SATAN_ERROR("CURRENT %d, %f, %f, %d, %f\n",
			    icon_anchor_x,
			    icon_anchor_y,
			    pixels_per_line,
			    instance_length,
			    finger_height);
		transform_t.init_identity();
		transform_t.translate(instance_start_pixel + pixels_per_line * loop_translation, icon_anchor_y);
		loop_icon.set_transform(transform_t);
		tapped_instance.set_rect_coords(
			instance_start_pixel, icon_anchor_y,
			pixels_per_line * instance_length,
			finger_height
			);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		drag_loop_completed_callback(new_loop_length);
		break;
	}
}

void Sequencer::scrolled_vertical(double pixels_changed) {
	sequencer_vertical_offset += pixels_changed;

	auto minimum_offset = -((double)(machine2sequence.size() - 1) * finger_height);
	SATAN_DEBUG("Scrolled vertical, new offset: %f (min: %f)\n", sequencer_vertical_offset, minimum_offset);
	if(sequencer_vertical_offset > 0)
		sequencer_vertical_offset = 0;
	else if(sequencer_vertical_offset < minimum_offset)
		sequencer_vertical_offset = minimum_offset;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
	transform_t.init_identity();
	transform_t.translate(0.0, sequencer_vertical_offset);
	sequencer_container.set_transform(transform_t);
}

void Sequencer::set_pattern_id_text(uint32_t pattern_id) {
	std::stringstream ss;
	ss << pattern_id;
	pattern_id_text.set_text_content(ss.str());
}

void Sequencer::show_all() {
	KammoGUI::GnuVGCanvas::ElementReference root_element(this);
	root_element.set_display("inline");
}

void Sequencer::hide_all() {
	KammoGUI::GnuVGCanvas::ElementReference root_element(this);
	root_element.set_display("none");
}

void Sequencer::hide_elements(std::vector<KammoGUI::GnuVGCanvas::ElementReference *> elements_to_hide) {
	for(auto element : elements_to_hide) {
		element->set_style("opacity:1.0");
	}

	auto shade_transition = new KammoGUI::SimpleAnimation(
		TRANSITION_TIME,
		[elements_to_hide](float progress) mutable {
			auto reverse_progress = 1.0f - progress;
			std::stringstream opacity;
			opacity << "opacity:" << (reverse_progress);
			for(auto element : elements_to_hide) {
				element->set_style(opacity.str());
			}

			SATAN_DEBUG("hide elements transition animation %f...\n", progress);
			if(progress >= 1.0f) {
				SATAN_DEBUG("hide elements transmission complete...\n");
				for(auto element : elements_to_hide) {
					element->set_display("none");
				}
			}
		}
		);
	sequencer->start_animation(shade_transition);
}

void Sequencer::show_elements(std::vector<KammoGUI::GnuVGCanvas::ElementReference *> elements_to_hide) {
	for(auto element : elements_to_hide) {
		element->set_style("opacity:0.0");
		element->set_display("inline");
	}

	auto shade_transition = new KammoGUI::SimpleAnimation(
		TRANSITION_TIME,
		[elements_to_hide](float progress) mutable {
			std::stringstream opacity;
			opacity << "opacity:" << (progress);
			for(auto element : elements_to_hide) {
				element->set_style(opacity.str());
			}

			SATAN_DEBUG("show elements transition animation %f...\n", progress);
			if(progress >= 1.0f) {
				SATAN_DEBUG("show elements transmission complete...\n");
			}
		}
		);
	sequencer->start_animation(shade_transition);
}

void Sequencer::vertical_scroll_event(const KammoGUI::MotionEvent &event) {
	if(fling_detector.on_touch_event(event)) {
		SATAN_DEBUG("fling detected.\n");
		float speed_x, speed_y;
		float abs_speed_x, abs_speed_y;

		fling_detector.get_speed(speed_x, speed_y);
		fling_detector.get_absolute_speed(abs_speed_x, abs_speed_y);

		bool do_horizontal_fling = abs_speed_x > abs_speed_y ? true : false;

		float speed = 0.0f, abs_speed;
		std::function<void(float pixels_changed)> scrolled;

		if(do_horizontal_fling) {
			abs_speed = abs_speed_x;
			speed = speed_x;
			scrolled = [this](float pixels_changed){
				SATAN_DEBUG("Scrolled horizontal: %f\n", pixels_changed);
			};
		} else {
			abs_speed = abs_speed_y;
			speed = speed_y;
			scrolled = [this](float pixels_changed){
				scrolled_vertical((double)pixels_changed);
			};
		}

		float fling_duration = abs_speed / FLING_DEACCELERATION;

		FlingAnimation *flinganim =
			new FlingAnimation(speed, fling_duration, scrolled);
		start_animation(flinganim);
	}

	switch(event.get_action()) {
	case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
	case KammoGUI::MotionEvent::ACTION_CANCEL:
	case KammoGUI::MotionEvent::ACTION_OUTSIDE:
	case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
	case KammoGUI::MotionEvent::ACTION_UP:
		scrolled_vertical(event.get_y() - scroll_start_y);
	case KammoGUI::MotionEvent::ACTION_DOWN:
		scroll_start_y = event.get_y();
		break;
	}
}

void Sequencer::on_resize() {
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

	// resize all machines
	for(auto m2s : machine2sequence) {
		m2s.second->set_graphic_parameters(
			scaling,
			document_size.width, document_size.height,
			canvas_w, canvas_h);
	}
}

static double rolling_avg_time = 0.0;

void print_timing_report() {
	static struct timespec last_time;
	struct timespec this_time, time_difference;

	(void) clock_gettime(CLOCK_MONOTONIC_RAW, &this_time);

	time_difference.tv_sec = this_time.tv_sec - last_time.tv_sec;
	time_difference.tv_nsec = this_time.tv_nsec - last_time.tv_nsec;
	if(time_difference.tv_nsec < 0)
		time_difference.tv_sec--;

	if(time_difference.tv_sec >= 100) {
		last_time = this_time;
		SATAN_ERROR(" ==========> rolling_avg_time: %f\n", rolling_avg_time);
	}
}

#include <time.h>
void Sequencer::on_render() {
	static struct timespec last_time;
	struct timespec new_time, time_diff;

	(void) clock_gettime(CLOCK_MONOTONIC_RAW, &new_time);

	time_diff.tv_sec = new_time.tv_sec - last_time.tv_sec;
	time_diff.tv_nsec = new_time.tv_nsec - last_time.tv_nsec;
	if(time_diff.tv_nsec < 0) {
		time_diff.tv_sec  -= 1;
		time_diff.tv_nsec += 1000000000;
	}

	last_time.tv_sec = new_time.tv_sec;
	last_time.tv_nsec = new_time.tv_nsec;

	double latest_time =
		(double)time_diff.tv_sec +
		(double)time_diff.tv_nsec / 1000000000.0;

	rolling_avg_time = (0.95 * rolling_avg_time) + (0.05 * latest_time);

	print_timing_report();
}

void Sequencer::object_registered(std::shared_ptr<RemoteInterface::ClientSpace::Sequence> ri_seq) {
	KammoGUI::run_on_GUI_thread(
		[this, ri_seq]() {
			int new_offset = (int)machine2sequence.size();
			SATAN_DEBUG("new_offset is %d\n", new_offset);

			auto new_sequence = Sequence::create_sequence(
				sequencer_container, sequence_graphic_template,
				ri_seq,
				new_offset);
			machine2sequence[ri_seq] = new_sequence;
			new_sequence->set_graphic_parameters(
				scaling,
				document_size.width, document_size.height,
				canvas_w, canvas_h);
		}
		);
}

void Sequencer::object_unregistered(std::shared_ptr<RemoteInterface::ClientSpace::Sequence> ri_seq) {
	KammoGUI::run_on_GUI_thread(
		[this, ri_seq]() {
			auto seq = machine2sequence.find(ri_seq);
			if(seq != machine2sequence.end()) {
				machine2sequence.erase(seq);
			}
		}
		);
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
		KammoGUI::GnuVGCanvas *cnvs = (KammoGUI::GnuVGCanvas *)wid;
		cnvs->set_bg_color(1.0, 1.0, 1.0);

		sequencer = std::make_shared<Sequencer>(cnvs);
		timelines = std::make_shared<TimeLines>(cnvs);
		sequencer->prepare_sequencer(cnvs);
		plus_button = std::make_shared<GnuVGCornerButton>(
			cnvs,
			std::string(SVGLoader::get_svg_directory() + "/plusButton.svg"),
			GnuVGCornerButton::bottom_right);
		pattern_editor = PatternEditor::get_pattern_editor(cnvs, timelines);
		loop_settings = std::make_shared<LoopSettings>(cnvs);
		knob_editor = KnobEditor::get_knob_editor(cnvs);
		main_menu = std::make_shared<MainMenu>(cnvs);
		return_button = std::make_shared<GnuVGCornerButton>(
			cnvs,
			std::string(SVGLoader::get_svg_directory() + "/leftArrow.svg"),
			GnuVGCornerButton::bottom_left);
		PatternEditor::hide();
		KnobEditor::hide();
		return_button->hide();

		plus_button->set_select_callback(
			[]() {
				static KammoGUI::UserEvent *ue = NULL;
				static KammoGUI::UserEvent *callback_ue = NULL;
				KammoGUI::get_widget((KammoGUI::Widget **)&ue, "showMachineTypeListScroller");
				KammoGUI::get_widget((KammoGUI::Widget **)&callback_ue, "showComposeContainer");
				if(ue != NULL && callback_ue != NULL) {
					std::map<std::string, void *> args;
					args["hint_match"] = strdup("generator");
					args["callback_event"] = callback_ue;
					KammoGUI::EventHandler::trigger_user_event(ue, args);
				}
			});

		return_button->set_select_callback(
			[]() {
				KnobEditor::hide();
				pattern_editor->hide();
				return_button->hide();
				sequencer->show_all();
				timelines->show_all();
				loop_settings->show();
				plus_button->show();
			});

		RemoteInterface::ClientSpace::Client::register_object_set_listener<RISequence>(sequencer);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<GCO>(timelines);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<GCO>(loop_settings);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<GCO>(main_menu);

		PopupMenu::prepare(cnvs);
		PopupWindow::prepare(cnvs);
	}
}

KammoEventHandler_Instance(SequencerHandler);
