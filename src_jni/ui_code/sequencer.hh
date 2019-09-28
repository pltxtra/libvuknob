/*
 * VuKNOB
 * Copyright (C) 2015 by Anton Persson
 *
 * http://www.vuknob.com/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef VUKNOB_SEQUENCER
#define VUKNOB_SEQUENCER

#include <map>
#include <gnuVGcanvas.hh>
#include <kamogui_fling_detector.hh>
#include "engine_code/sequence.hh"

#include "timelines.hh"


typedef RemoteInterface::ClientSpace::Sequence RISequence;
typedef RemoteInterface::ClientSpace::Sequence::PatternInstance RIPatternInstance;

class Sequencer
	: public RemoteInterface::Context::ObjectSetListener<RISequence>
	, public KammoGUI::GnuVGCanvas::SVGDocument
{

private:
	enum InstanceEventType {
		finger_on,
		moved,
		tapped
	};
	struct InstanceEvent {
		InstanceEventType type;
		int moving_offset;
		double x, y; // top left corner of tapped instance
	};
	class PatternInstance
		: public std::enable_shared_from_this<PatternInstance>
	{
	private:
		KammoGUI::GnuVGCanvas::ElementReference svg_reference;

		bool display_action = false;
		RIPatternInstance instance_data;
		KammoGUI::GnuVGCanvas::ElementReference instance_graphic, pattern_id_graphic;
		std::function<void(const InstanceEvent &e)> event_callback;

		double line_width = 1.0;
		int moving_offset = 0, start_at_sequence_position = 0, minimum_visible_line = 0, maximum_visible_line = 10;

		void refresh_visibility();
		void on_instance_event(std::shared_ptr<RISequence> ri_seq,
				       const KammoGUI::MotionEvent &event);
	public:
		std::shared_ptr<TimeLines::ZoomContext> zoom_context;

		PatternInstance(
			KammoGUI::GnuVGCanvas::ElementReference &elref,
			const RIPatternInstance &instance_data,
			std::shared_ptr<RISequence> ri_seq,
			std::function<void(const InstanceEvent &e)> event_callback,
			std::shared_ptr<TimeLines::ZoomContext> zoom_context
			);

		virtual ~PatternInstance() {
			svg_reference.drop_element();
		}

		const RIPatternInstance& data() {
			return instance_data;
		}

		static std::shared_ptr<PatternInstance> create_new_pattern_instance(
			const RIPatternInstance &instance_data,
			KammoGUI::GnuVGCanvas::ElementReference &parent,
			int line_width, double height,
			std::shared_ptr<RISequence> ri_seq,
			std::function<void(const InstanceEvent &e)> event_callback,
			std::shared_ptr<TimeLines::ZoomContext> zoom_context
			);

		void calculate_visibility(double line_width,
					  int minimum_visible_line,
					  int maximum_visible_line);
	};

	class Sequence
		: public KammoGUI::GnuVGCanvas::ElementReference
		, public RISequence::SequenceListener
		, public std::enable_shared_from_this<Sequence>
	{
	private:
		struct PendingAdd {
			int start_at, stop_at;

			PendingAdd(int strt, int stop) : start_at(strt), stop_at(stop) {}
		};

		std::shared_ptr<PendingAdd> pending_add;

		KammoGUI::GnuVGCanvas::ElementReference sequence_background, button_background,
			controlls_button, mute_button, envelope_button;
		double inverse_scaling_factor;
		double event_start_x, event_start_y;
		double event_current_x, event_current_y;
		int start_at_sequence_position, stop_at_sequence_position;
		double width, height;
		bool display_action = false;

		std::shared_ptr<RISequence> ri_seq;
		int offset;

		std::map<uint32_t, std::string> patterns;
		std::map<int, std::shared_ptr<PatternInstance> > instances;

		static constexpr uint32_t NO_ACTIVE_PATTERN = IDAllocator::NO_ID_AVAILABLE;
		uint32_t active_pattern_id = NO_ACTIVE_PATTERN;

		void on_sequence_event(const KammoGUI::MotionEvent &event);

		double line_width, line_offset;
		int minimum_visible_line, maximum_visible_line;

		std::map<uint32_t, std::shared_ptr<TimeLines::ZoomContext> > pattern_zoom_contexts;

	public:
		virtual void pattern_added(const std::string &name, uint32_t id);
		virtual void pattern_deleted(uint32_t id);
		virtual void instance_added(const RIPatternInstance& instance);
		virtual void instance_deleted(const RIPatternInstance& instance);

		void set_graphic_parameters(double graphic_scaling_factor,
					    double width, double height,
					    double canvas_w, double canvas_h);

		void on_scroll(double _line_width, double _line_offset,
			       int _minimum_visible_line,
			       int _maximum_visible_line);

		Sequence(
			KammoGUI::GnuVGCanvas::ElementReference elref,
			std::shared_ptr<RISequence> ri_seq,
			int offset);

		static std::shared_ptr<Sequence> create_sequence(
			KammoGUI::GnuVGCanvas::ElementReference &root,
			KammoGUI::GnuVGCanvas::ElementReference &sequence_graphic_template,
			std::shared_ptr<RISequence> ri_seq,
			int offset);
	};

	KammoGUI::GnuVGCanvas::ElementReference root, sequencer_container;
	KammoGUI::GnuVGCanvas::ElementReference sequence_graphic_template;
	KammoGUI::GnuVGCanvas::ElementReference trashcan_icon, notes_icon, tapped_instance, sequencer_shade;
	KammoGUI::GnuVGCanvas::ElementReference loop_icon, loop_enabled_icon, loop_disabled_icon, length_icon;
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	KammoGUI::FlingGestureDetector fling_detector;
	double scroll_start_y, sequencer_vertical_offset = 0.0;

	std::map<std::shared_ptr<RISequence>, std::shared_ptr<Sequence> >machine2sequence;

	double scaling; // graphical scaling factor
	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

	float sequencer_shade_hiding_opacity;
	double drag_event_start_x;

	void drag_length_icon(const KammoGUI::MotionEvent &event, // returns true when drag is completed.
			      double icon_anchor_x, double icon_anchor_y,
			      double pixels_per_line,
			      int instance_length,
			      std::function<void(int)> drag_length_completed_callback
		);
	void scrolled_vertical(double pixels_changed);

public:

	Sequencer(KammoGUI::GnuVGCanvas* cnvs);

	void hide_sequencers(float hiding_opacity,
			     bool show_icons, double icon_anchor_x, double icon_anchor_y,
			     std::weak_ptr<RISequence>ri_seq,
			     std::weak_ptr<PatternInstance>instance
		);
	void show_sequencers();

	void vertical_scroll_event(const KammoGUI::MotionEvent &event);

	virtual void on_resize() override;
	virtual void on_render() override;

	virtual void object_registered(std::shared_ptr<RISequence> ri_seq) override;
	virtual void object_unregistered(std::shared_ptr<RISequence> ri_seq) override;
};

#endif
