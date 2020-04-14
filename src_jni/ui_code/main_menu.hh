/*
 * VuKNOB
 * (C) 2020 by Anton Persson
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

#ifndef MAIN_MENU_CLASS
#define MAIN_MENU_CLASS

#include <gnuVGcanvas.hh>

class MainMenu : public KammoGUI::GnuVGCanvas::SVGDocument{
public:

private:
	KammoGUI::GnuVGCanvas::ElementReference root, backdrop;
	KammoGUI::GnuVGCanvas::ElementReference rewind_button, play_button, record_button;
	KammoGUI::GnuVGCanvas::ElementReference connector_button, jam_button, sequencer_button, settings_button;

	KammoGUI::GnuVGCanvas::SVGRect document_size;
	int canvas_w, canvas_h;
	float canvas_w_inches, canvas_h_inches;
	double finger_width = 10.0, finger_height = 10.0; // one finger's size in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	double scaling;
public:

	MainMenu(KammoGUI::GnuVGCanvas *cnv);
	~MainMenu();

	virtual void on_render();
	virtual void on_resize();
};

#endif
