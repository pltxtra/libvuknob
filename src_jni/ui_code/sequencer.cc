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
#include "common.hh"

#include "../engine_code/sequence.hh"
#include "../engine_code/client.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

/***************************
 *
 *  Class Sequencer::PatternInstance
 *
 ***************************/

Sequencer::PatternInstance::PatternInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref,
	const RIPatternInstance &_instance_data
	)
	: KammoGUI::GnuVGCanvas::ElementReference(elref)
	, instance_data(_instance_data)
{
}

void Sequencer::PatternInstance::calculate_visibility(int minimum_minor_offset,
						      int maximum_minor_offset) {
}

std::shared_ptr<Sequencer::PatternInstance> Sequencer::PatternInstance::create_new_pattern_instance(
	const RIPatternInstance &_instance_data,
	KammoGUI::GnuVGCanvas::ElementReference &parent
	)
{
	std::stringstream ss_new_id;
	ss_new_id << "pattern_instance_" << _instance_data.pattern_id;

	std::stringstream ss;
	ss << "<rect "
	   << "style=\"fill:#ff0000;fill-opacity:1\" "
	   << "id=\"" << ss_new_id.str() << "\" "
	   << "width=\"4000\" "
	   << "height=\"200\" "
	   << "x=\"0.0\" "
	   << "y=\"0.0\" />"

		;

	parent.add_svg_child(ss.str());

	KammoGUI::GnuVGCanvas::ElementReference elref(&parent, ss_new_id.str());

	auto rval = std::make_shared<PatternInstance>(elref, _instance_data);

	return rval;
}

/***************************
 *
 *  Class Sequencer::Sequence
 *
 ***************************/

Sequencer::Sequence::Sequence(KammoGUI::GnuVGCanvas::ElementReference elref,
			      std::shared_ptr<RISequence> _ri_seq,
			      std::shared_ptr<TimeLines> _timelines,
			      int _offset)
	: KammoGUI::GnuVGCanvas::ElementReference(elref)
	, ri_seq(_ri_seq)
	, timelines(_timelines)
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
			SATAN_DEBUG("seqBackground event detected.\n");
			on_sequence_event(event);
		}
		);

	_timelines->add_scroll_callback(
		[this](double line_offset, int left_side_minor_offset, int right_side_minor_offset) {
			KammoGUI::GnuVGCanvas::SVGMatrix transform_t;
			transform_t.init_identity();
			transform_t.translate(line_offset * inverse_scaling_factor, 0.0);

			auto instanceContainer = find_child_by_class("instanceContainer");
			instanceContainer.set_transform(transform_t);

			for(auto p : instances) {
				auto inst = p.second;

				inst->calculate_visibility(
					left_side_minor_offset, right_side_minor_offset
					);
			}
		}
		);
}

void Sequencer::Sequence::on_sequence_event(const KammoGUI::MotionEvent &event) {
	auto evt_x = inverse_scaling_factor * event.get_x();
	auto evt_y = inverse_scaling_factor * event.get_y();
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

		start_at_sequence_position = timelines->get_sequence_minor_position_at(event.get_x());

		SATAN_DEBUG("event_start_x: %f\n", event_start_x);
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		event_current_x = evt_x;
		event_current_y = evt_y;

		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		stop_at_sequence_position = timelines->get_sequence_minor_position_at(event.get_x());
		if(evt_x > event_start_x) {
			if(active_pattern_id == NO_ACTIVE_PATTERN) {
				ri_seq->add_pattern("New pattern");
			} else {
				SATAN_DEBUG("Start: %f - Stop: %f\n",
					    start_at_sequence_position,
					    stop_at_sequence_position);
				ri_seq->insert_pattern_in_sequence(active_pattern_id,
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

		if(event_current_x < event_start_x) {
			b_x = event_current_x;
			e_x = event_start_x - event_current_x;
		} else {
			b_x = event_start_x;
			e_x = event_current_x - event_start_x;
		}
		SATAN_DEBUG("b_x: %f, e_x: %f\n", b_x, e_x);
		newPieceIndicator.set_rect_coords(b_x, 0, e_x, height);
	}
}

void Sequencer::Sequence::pattern_added(const std::string &name, uint32_t id) {
	SATAN_DEBUG("::pattern_added(%s, %d)\n", name.c_str(), id);
	active_pattern_id = id;

	patterns[id] = name;

}

void Sequencer::Sequence::pattern_deleted(uint32_t id) {
	SATAN_DEBUG("::pattern_deleted()\n");

	auto pattern_to_erase = patterns.find(id);
	if(pattern_to_erase != patterns.end())
		patterns.erase(pattern_to_erase);
}


void Sequencer::Sequence::instance_added(const RIPatternInstance& instance) {
	SATAN_DEBUG("::instance_added() callback...\n");

	auto instanceContainer = find_child_by_class("instanceContainer");

	auto i = PatternInstance::create_new_pattern_instance(
		instance,
		instanceContainer
		);

	instances[instance.start_at] = i;
}

void Sequencer::Sequence::instance_deleted(const RIPatternInstance& instance) {
	SATAN_DEBUG("::instance_deleted()\n");

	auto instance_to_erase = instances.find(instance.start_at);
	if(instance_to_erase != instances.end()) {
		instance_to_erase->second->drop_element();
		instances.erase(instance_to_erase);
	}
}

void Sequencer::Sequence::set_graphic_parameters(double graphic_scaling_factor,
						 double _width, double _height,
						 double canvas_w, double canvas_h) {
	width = _width; height = _height;

	inverse_scaling_factor = 1.0 / graphic_scaling_factor;

	find_child_by_class("seqBackground").set_rect_coords(
		width, 0, canvas_w, height);

	SATAN_DEBUG("Trying to find instanceWindow - by class...");
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
	transform_t.scale(graphic_scaling_factor, graphic_scaling_factor);

	SATAN_DEBUG("offset: %d\n", offset);

	set_transform(transform_t);
}

auto Sequencer::Sequence::create_sequence(
	KammoGUI::GnuVGCanvas::ElementReference &root,
	KammoGUI::GnuVGCanvas::ElementReference &sequence_graphic_template,
	std::shared_ptr<RISequence> ri_machine,
	std::shared_ptr<TimeLines> _timelines,
	int offset) -> std::shared_ptr<Sequence> {

	char bfr[32];
	snprintf(bfr, 32, "seq_%p", ri_machine.get());

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		root.add_element_clone(bfr, sequence_graphic_template);

	SATAN_DEBUG("Sequencer::Sequence::create_sequence() -- bfr: %s\n", bfr);

	auto new_sequence = std::make_shared<Sequence>(new_graphic, ri_machine, _timelines, offset);
	ri_machine->add_sequence_listener(new_sequence);
	return new_sequence;
}

/***************************
 *
 *  Class Sequencer
 *
 ***************************/

Sequencer::Sequencer(KammoGUI::GnuVGCanvas* cnvs, std::shared_ptr<TimeLines> _timelines)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/sequencerMachine.svg"), cnvs)
	, timelines(_timelines)
{
	sequence_graphic_template = KammoGUI::GnuVGCanvas::ElementReference(this, "sequencerMachineTemplate");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

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

	KammoGUI::GnuVGCanvas::SVGRect document_size;

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

			KammoGUI::GnuVGCanvas::ElementReference layer(this, "layer1");
			machine2sequence[ri_seq] =
				Sequence::create_sequence(
					layer, sequence_graphic_template,
					ri_seq, timelines,
					new_offset);
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

		static auto current_timelines = std::make_shared<TimeLines>(cnvs);
		static auto current_sequencer = std::make_shared<Sequencer>(cnvs, current_timelines);

		auto ptr =
			std::dynamic_pointer_cast<RemoteInterface::Context::ObjectSetListener<RISequence> >(current_sequencer);
		std::weak_ptr<RemoteInterface::Context::ObjectSetListener<RISequence> > w_ptr = ptr;
		RemoteInterface::ClientSpace::Client::register_object_set_listener(w_ptr);
	}
}

KammoEventHandler_Instance(SequencerHandler);
