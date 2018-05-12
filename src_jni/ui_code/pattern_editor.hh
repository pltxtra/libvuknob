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

#ifndef VUKNOB_PATTERN_EDITOR
#define VUKNOB_PATTERN_EDITOR

#include <gnuVGcanvas.hh>
#include <kamogui_scale_detector.hh>
#include <kamogui_fling_detector.hh>

#include "machine_sequencer.hh"

class PatternEditor
	: public KammoGUI::GnuVGCanvas::SVGDocument
{
private:

	// sizes in pixels
	double finger_width = 10.0, finger_height = 10.0;

	// canvas size in "fingers"
	int canvas_width_fingers = 8, canvas_height_fingers = 8;
	float canvas_w_inches, canvas_h_inches;
	int canvas_w, canvas_h;

public:
	PatternEditor(KammoGUI::GnuVGCanvas* cnvs);
	~PatternEditor();

	virtual void on_resize() override;
	virtual void on_render() override;

};

#endif
