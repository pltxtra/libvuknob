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

#ifndef SCALE_SLIDE_CLASS
#define SCALE_SLIDE_CLASS

#include <gnuVGcanvas.hh>
#include <functional>

class ScaleSlide : public KammoGUI::GnuVGCanvas::SVGDocument {
private:
	class Transition : public KammoGUI::Animation {
	private:
		ScaleSlide *sc;
		std::function<void(ScaleSlide *context, float progress)> callback;

	public:
		Transition(ScaleSlide *sc, std::function<void(ScaleSlide *context, float progress)> callback);
		virtual void new_frame(float progress);
		virtual void on_touch_event();
	};

	std::function<void(double new_value)> callback;
	double value, last_value;

	// graphical data
	KammoGUI::GnuVGCanvas::SVGRect document_size; // the size of the document as loaded
	KammoGUI::GnuVGCanvas::SVGRect knob_size; // visual size of the knob (in document coordinates, not screen)

	double x, y, width, height; // the viewport which we should squeeze the document into (animate TO)
	double initial_x, initial_y, initial_width, initial_height; // the viewport which we animate FROM
	double translate_x, translate_y, scale; // current translation and scale

	double last_y;

	bool transition_to_active_state;

	KammoGUI::GnuVGCanvas::ElementReference *scale_knob;

	KammoGUI::GnuVGCanvas::ElementReference front_text;
	KammoGUI::GnuVGCanvas::ElementReference shade_text;
	KammoGUI::GnuVGCanvas::ElementReference front_label;
	KammoGUI::GnuVGCanvas::ElementReference shade_label;

	void interpolate(double progress);
	static void transition_progressed(ScaleSlide *sc, float progress);

	static void on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
			     KammoGUI::GnuVGCanvas::ElementReference *e_ref,
			     const KammoGUI::MotionEvent &event);

public:
	ScaleSlide(KammoGUI::GnuVGCanvas *cnv);
	~ScaleSlide();

	void show(
		const std::string &label, double initial_value,
		double initial_x, double initial_y, double initial_width, double initial_height,
		double x, double y, double width, double height,
		std::function<void(double new_value)> callback);
	void hide();

	virtual void on_resize();
	virtual void on_render();
};

#endif
