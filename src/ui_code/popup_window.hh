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

#ifndef VUKNOB_POPUP_WINDOW
#define VUKNOB_POPUP_WINDOW

#include <gnuVGcanvas.hh>

class PopupWindow
	: public KammoGUI::GnuVGCanvas::SVGDocument
{
public:
	enum UserResponse {
		yes, no
	};

private:
	KammoGUI::GnuVGCanvas::SVGRect document_size;
	int canvas_w, canvas_h;
	int canvas_width_fingers = 8, canvas_height_fingers = 8;
	float canvas_w_inches, canvas_h_inches;
	double finger_width = 10.0, finger_height = 10.0, scaling = 1.0;

	KammoGUI::GnuVGCanvas::ElementReference root, container, backdrop, yes_button, no_button;
	std::function<void(UserResponse response)> response_callback;

	void hide();
	void show();

	static PopupWindow *singleton;
	PopupWindow(KammoGUI::GnuVGCanvas* cnvs);

public:
	~PopupWindow();

	virtual void on_resize() override;
	virtual void on_render() override;

	static void prepare(KammoGUI::GnuVGCanvas* cnvs);
	static void ask_yes_or_no(
		std::string row1,
		std::string row2,
		std::string row3,
		std::function<void(UserResponse response)> response_callback
		);
};

#endif
