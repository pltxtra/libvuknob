/*
 * VuKNOB
 * (C) 2014 by Anton Persson
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

#ifndef SIMPLEBUTTON_CLASS
#define SIMPLEBUTTON_CLASS

#include <gnuVGcanvas.hh>
#include <functional>

class SimpleButton : public KammoGUI::GnuVGCanvas::SVGDocument{
private:
	KammoGUI::GnuVGCanvas::SVGMatrix base_transform_t;
	KammoGUI::GnuVGCanvas::ElementReference elref;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	double first_selection_x, first_selection_y;
	bool is_a_tap;

	double x = 0.0, y = 0.0, width = 0.0, height = 0.0;

	std::function<void()> callback_function;

	static void on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
			     KammoGUI::GnuVGCanvas::ElementReference *e_ref,
			     const KammoGUI::MotionEvent &event);
public:
	SimpleButton(KammoGUI::GnuVGCanvas *cnv, const std::string &svg_file);
	~SimpleButton();

	virtual void on_render();
	virtual void on_resize();

	void show();
	void hide();

	void set_size(double w, double h);
	void move_to(double x, double y);

	void set_select_callback(std::function<void()> callback_function);
};

#endif
