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
#include "gnuvg_corner_button.hh"
#include "svg_loader.hh"
#include "common.hh"

#include "../engine_code/sequence.hh"
#include "../engine_code/client.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static std::shared_ptr<TimeLines> timelines;
static std::shared_ptr<Sequencer> sequencer;
static std::shared_ptr<GnuVGCornerButton> plus_button;
static std::shared_ptr<GnuVGCornerButton> return_button;
static std::shared_ptr<PatternEditor> pattern_editor;
static std::shared_ptr<LoopSettings> loop_settings;

/***************************
 *
 *  Class Sequencer::PatternInstance
 *
 ***************************/

Sequencer::PatternInstance::PatternInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref,
	const RIPatternInstance &_instance_data,
	std::shared_ptr<RISequence> ri_seq,
	std::function<void(const InstanceEvent &)> _event_callback
	)
	: svg_reference(elref)
	, instance_data(_instance_data)
	, event_callback(_event_callback)
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
	auto last_moving_offset = moving_offset;
	moving_offset = 0;

	if(tap_detector.analyze_events(event)) {
		InstanceEvent e;
		e.type = InstanceEventType::tapped;
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
	std::function<void(const InstanceEvent &e)> event_callback
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

	auto rval = std::make_shared<PatternInstance>(elref, _instance_data, ri_seq, event_callback);

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
			SATAN_DEBUG("seqBackground event detected.\n");
			on_sequence_event(event);
		}
		);
}

void Sequencer::Sequence::on_sequence_event(const KammoGUI::MotionEvent &event) {
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

		SATAN_DEBUG("event_start_x: %f\n", event_start_x);
		break;
	case KammoGUI::MotionEvent::ACTION_MOVE:
		event_current_x = evt_x;
		event_current_y = evt_y;

		SATAN_DEBUG("event_current_x: %f\n", event_current_x);
		break;
	case KammoGUI::MotionEvent::ACTION_UP:
		stop_at_sequence_position = timelines->get_sequence_line_position_at(event.get_x());
		if(evt_x > event_start_x) {
			SATAN_DEBUG("Start: %d - Stop: %d\n",
				    start_at_sequence_position,
				    stop_at_sequence_position);
			if(start_at_sequence_position < 0)
				start_at_sequence_position = 0;
			if(stop_at_sequence_position < 0)
				stop_at_sequence_position = 0;

			if(start_at_sequence_position > stop_at_sequence_position) {
				auto t = start_at_sequence_position;
				start_at_sequence_position = stop_at_sequence_position;
				stop_at_sequence_position = t;
			}
			SATAN_DEBUG("Forced - Start: %d - Stop: %d\n",
				    start_at_sequence_position,
				    stop_at_sequence_position);

			if(start_at_sequence_position != stop_at_sequence_position) {
				pending_add = std::make_shared<PendingAdd>(
					start_at_sequence_position,
					stop_at_sequence_position
					);
			}

			if(active_pattern_id == NO_ACTIVE_PATTERN) {
				ri_seq->add_pattern("New pattern");
			} else if(pending_add != nullptr) {
				ri_seq->insert_pattern_in_sequence(
					active_pattern_id,
					pending_add->start_at, -1,
					pending_add->stop_at);
				pending_add.reset();
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
	KammoGUI::run_on_GUI_thread(
		[this, name, id]() {
			SATAN_DEBUG("::pattern_added(%s, %d)\n", name.c_str(), id);
			active_pattern_id = id;

			patterns[id] = name;

			if(pending_add) {
				SATAN_DEBUG("::pattern_added(%s, %d) --> insert\n", name.c_str(), id);
				ri_seq->insert_pattern_in_sequence(
					id,
					pending_add->start_at, -1,
					pending_add->stop_at);
				pending_add.reset();
				SATAN_DEBUG("::pattern_added(%s, %d) insert <--\n", name.c_str(), id);
			}
		}
		);
}

void Sequencer::Sequence::pattern_deleted(uint32_t id) {
	KammoGUI::run_on_GUI_thread(
		[this, id]() {
			SATAN_DEBUG("::pattern_deleted()\n");

			auto pattern_to_erase = patterns.find(id);
			if(pattern_to_erase != patterns.end())
				patterns.erase(pattern_to_erase);
		}
		);
}

void Sequencer::Sequence::instance_added(const RIPatternInstance& instance){
	KammoGUI::run_on_GUI_thread(
		[this, instance]() {
			auto restore_sequencer = [this]() {
				auto main_container = get_root();
				main_container.set_display("inline");
				plus_button->show();
				timelines->show_loop_markers();
				loop_settings->show();
			};

			auto event_callback = [this, instance, restore_sequencer](const InstanceEvent &e) {
				switch(e.type) {
				case InstanceEventType::selected:
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
					SATAN_DEBUG("Tapped on pattern instance for pattern %d, %p (%s)\n",
						    instance.pattern_id, ri_seq.get(), ri_seq->get_name().c_str());

					PatternEditor::show(restore_sequencer, ri_seq, instance.pattern_id);
					auto main_container = get_root();
					main_container.set_display("none");
					timelines->hide_loop_markers();
					loop_settings->hide();
					plus_button->hide();
					return_button->show();
				}
				}
			};

			auto instanceContainer = find_child_by_class("instanceContainer");
			auto i = PatternInstance::create_new_pattern_instance(
				instance,
				instanceContainer,
				timelines->get_horizontal_pixels_per_line(),
				height,
				ri_seq,
				event_callback
				);
			instances[instance.start_at] = i;
			timelines->call_scroll_callbacks();
			i->calculate_visibility(
				line_width, minimum_visible_line, maximum_visible_line
				);
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
	sequence_graphic_template = KammoGUI::GnuVGCanvas::ElementReference(this, "sequencerMachineTemplate");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);

	sequence_graphic_template.set_display("none");

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

			KammoGUI::GnuVGCanvas::ElementReference sequencer_container(this, "sequencerContainer");
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

		timelines = std::make_shared<TimeLines>(cnvs);
		sequencer = std::make_shared<Sequencer>(cnvs);
		plus_button = std::make_shared<GnuVGCornerButton>(
			cnvs,
			std::string(SVGLoader::get_svg_directory() + "/plusButton.svg"),
			GnuVGCornerButton::bottom_right);
		pattern_editor = std::make_shared<PatternEditor>(cnvs, timelines);
		return_button = std::make_shared<GnuVGCornerButton>(
			cnvs,
			std::string(SVGLoader::get_svg_directory() + "/leftArrow.svg"),
			GnuVGCornerButton::top_left);
		loop_settings = std::make_shared<LoopSettings>(cnvs);
		PatternEditor::hide();
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
				pattern_editor->hide();
				return_button->hide();
			});

		RemoteInterface::ClientSpace::Client::register_object_set_listener<RISequence>(sequencer);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<GCO>(timelines);
		RemoteInterface::ClientSpace::Client::register_object_set_listener<GCO>(loop_settings);
	}
}

KammoEventHandler_Instance(SequencerHandler);
