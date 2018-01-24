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
//#include <kamogui.hh>
#include "engine_code/sequence.hh"

typedef RemoteInterface::ClientSpace::Sequence RISequence;

class Sequencer
	: public RemoteInterface::Context::ObjectSetListener<RISequence>
	, public KammoGUI::GnuVGCanvas::SVGDocument
{

private:
	class Sequence
		: public KammoGUI::GnuVGCanvas::ElementReference
		, public std::enable_shared_from_this<Sequence>
	{
	private:
		KammoGUI::GnuVGCanvas::ElementReference sequence_background;
		double inverse_scaling_factor;
		double event_start_x, event_start_y;
		double event_current_x, event_current_y;
		double width, height;
		bool display_action = false;

		std::shared_ptr<RISequence> ri_seq;
		int offset;

		static constexpr uint32_t NO_ACTIVE_PATTERN = IDAllocator::NO_ID_AVAILABLE;
		uint32_t active_pattern_id = NO_ACTIVE_PATTERN;

		void on_sequence_event(const KammoGUI::MotionEvent &event);

	public:
		virtual void pattern_added(const std::string &name, uint32_t id);
		virtual void pattern_deleted(uint32_t id);
		virtual void instance_added(const RIPatternInstance& instance);
		virtual void instance_deleted(const RIPatternInstance& instance);

		void set_graphic_parameters(double graphic_scaling_factor,
					    double width, double height,
					    double canvas_w, double canvas_h);

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

	KammoGUI::GnuVGCanvas::ElementReference root;
	KammoGUI::GnuVGCanvas::ElementReference sequence_graphic_template;

	std::map<std::shared_ptr<RISequence>, std::shared_ptr<Sequence> >machine2sequence;

	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

public:

	Sequencer(KammoGUI::GnuVGCanvas* cnvs);

	virtual void on_resize() override;
	virtual void on_render() override;

	virtual void object_registered(std::shared_ptr<RISequence> ri_seq) override;
	virtual void object_unregistered(std::shared_ptr<RISequence> ri_seq) override;
};

#endif
