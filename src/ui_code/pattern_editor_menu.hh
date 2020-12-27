/*
 * vu|KNOB
 * Copyright (C) 2018 by Anton Persson
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

#ifndef VUKNOB_PATTERN_EDITOR_MENU
#define VUKNOB_PATTERN_EDITOR_MENU

#include <gnuVGcanvas.hh>
#include <kamogui_fling_detector.hh>

class PatternEditorMenu : public KammoGUI::SVGCanvas::SVGDocument {
private:
	class HelpTextAnimation : public KammoGUI::Animation {
	private:
		KammoGUI::SVGCanvas::ElementReference *help_text;
	public:
		HelpTextAnimation(KammoGUI::SVGCanvas::ElementReference* _help_text, const std::string &display_text);
		virtual void new_frame(float progress);
		virtual void on_touch_event();
	};

	bool rim_visible;

	Tracker::SnapMode last_snap_mode;

	int pixel_w, pixel_h;
	float scale, translate_h, translate_v;

	KammoGUI::SVGCanvas::ElementReference menu_center;
	KammoGUI::SVGCanvas::ElementReference menu_rim;
	KammoGUI::SVGCanvas::ElementReference menu_loopNumberText;
	KammoGUI::SVGCanvas::ElementReference menu_help;

	KammoGUI::SVGCanvas::ElementReference menu_ignoreCircle;

	KammoGUI::SVGCanvas::ElementReference menu_copy;
	KammoGUI::SVGCanvas::ElementReference menu_paste;
	KammoGUI::SVGCanvas::ElementReference menu_trash;

	KammoGUI::SVGCanvas::ElementReference menu_quantize;
	KammoGUI::SVGCanvas::ElementReference menu_shiftLeft;
	KammoGUI::SVGCanvas::ElementReference menu_shiftRight;
	KammoGUI::SVGCanvas::ElementReference menu_transposeUp;
	KammoGUI::SVGCanvas::ElementReference menu_transposeDown;
	KammoGUI::SVGCanvas::ElementReference menu_octaveUp;
	KammoGUI::SVGCanvas::ElementReference menu_octaveDown;

	KammoGUI::SVGCanvas::ElementReference menu_undo;
	KammoGUI::SVGCanvas::ElementReference menu_disableUndo;

	KammoGUI::SVGCanvas::ElementReference menu_previousLoop;
	KammoGUI::SVGCanvas::ElementReference menu_nextLoop;

	KammoGUI::SVGCanvas::ElementReference menu_snapTo;
	KammoGUI::SVGCanvas::ElementReference menu_disableSnap;

	double first_selection_x, first_selection_y;
	bool is_a_tap;
	static void menu_selection_on_event(KammoGUI::SVGCanvas::SVGDocument *source,
					    KammoGUI::SVGCanvas::ElementReference *e_ref,
					    const KammoGUI::MotionEvent &event);
	static void menu_center_on_event(KammoGUI::SVGCanvas::SVGDocument *source,
					 KammoGUI::SVGCanvas::ElementReference *e_ref,
					 const KammoGUI::MotionEvent &event);

public:
	PatternEditorMenu(KammoGUI::SVGCanvas *cnv);

	void set_loop_number(int loop_nr);

	virtual void on_resize();
	virtual void on_render();
};

#endif
