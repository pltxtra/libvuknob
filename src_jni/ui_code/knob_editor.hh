/*
 * VuKNOB
 * Copyright (C) 2020 by Anton Persson
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

#ifndef VUKNOB_KNOB_EDITOR
#define VUKNOB_KNOB_EDITOR

#include <map>
#include <gnuVGcanvas.hh>

#include "../engine_code/base_machine.hh"

typedef RemoteInterface::ClientSpace::BaseMachine       BMachine;
typedef RemoteInterface::ClientSpace::BaseMachine::Knob BMKnob;

class KnobEditor
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public std::enable_shared_from_this<KnobEditor>
{

private:
	static KnobEditor *singleton;

	class KnobInstance
		: public std::enable_shared_from_this<KnobInstance>
	{
	private:
		KammoGUI::GnuVGCanvas::ElementReference svg_reference;
		std::shared_ptr<BMKnob> knob;
		int offset;

	public:
		KnobInstance(
			KammoGUI::GnuVGCanvas::ElementReference &elref,
			int _offset,
			std::shared_ptr<BMKnob> _knob
			);

		virtual ~KnobInstance();

		void refresh_transformation(double width, double height);

		static std::shared_ptr<KnobInstance> create_knob_instance(
			std::shared_ptr<BMKnob> knob,
			KammoGUI::GnuVGCanvas::ElementReference &container,
			KammoGUI::GnuVGCanvas::ElementReference &knob_template,
			int offset);
	};

	KammoGUI::GnuVGCanvas::ElementReference root, knob_container, knob_template, popup_trigger, popup_text, popup_container;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	double scaling; // graphical scaling factor
	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

	std::shared_ptr<BMachine> current_machine;
	std::vector<std::string> groups;
	std::string current_group;
	std::vector<std::shared_ptr<KnobInstance> > knob_instances;

	void refresh_knobs();
	void internal_show(std::shared_ptr<BMachine> machine);
public:
	KnobEditor(KammoGUI::GnuVGCanvas* cnvs);

	virtual void on_resize() override;
	virtual void on_render() override;

	static std::shared_ptr<KnobEditor> get_knob_editor(KammoGUI::GnuVGCanvas* cnvs);

	static void hide();
	static void show(std::shared_ptr<BMachine> machine);
};

#endif
