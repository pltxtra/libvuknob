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

#ifndef VUKNOB_KNOB_INSTANCE
#define VUKNOB_KNOB_INSTANCE

#include <gnuVGcanvas.hh>
#include "../engine_code/abstract_knob.hh"

typedef RemoteInterface::ClientSpace::AbstractKnob      AbstractKnob;

class KnobInstance
	: public std::enable_shared_from_this<KnobInstance>
{
private:
	KammoGUI::GnuVGCanvas::ElementReference svg_reference;
	KammoGUI::GnuVGCanvas::ElementReference value_text;
	KammoGUI::GnuVGCanvas::ElementReference value_decrease_button;
	KammoGUI::GnuVGCanvas::ElementReference value_increase_button;
	KammoGUI::GnuVGCanvas::ElementReference value_base_bar;
	KammoGUI::GnuVGCanvas::ElementReference value_actual_bar;
	KammoGUI::GnuVGCanvas::ElementReference value_slider_knob;

	std::shared_ptr<AbstractKnob> knob;
	int offset;

	double min, max, step;
	double canvas_width, finger_width, finger_height;

	void refresh_value_indicators();
public:
	KnobInstance(
		KammoGUI::GnuVGCanvas::ElementReference &elref,
		int _offset,
		std::shared_ptr<AbstractKnob> _knob
		);

	virtual ~KnobInstance();

	void refresh_transformation(double canvas_width, double finger_width, double finger_height);

	static std::shared_ptr<KnobInstance> create_knob_instance(
		std::shared_ptr<AbstractKnob> knob,
		KammoGUI::GnuVGCanvas::ElementReference &container,
		KammoGUI::GnuVGCanvas::ElementReference &knob_template,
		int offset);
};

#endif
