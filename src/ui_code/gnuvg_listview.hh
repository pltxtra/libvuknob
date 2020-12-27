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

#ifndef LISTVIEW_CLASS
#define LISTVIEW_CLASS

#include <gnuVGcanvas.hh>
#include <functional>

class GnuVGListView : public KammoGUI::GnuVGCanvas::SVGDocument  {
private:
	class Row : public KammoGUI::GnuVGCanvas::ElementReference {
	private:
		int row_index;
		GnuVGListView *parent;

		static void on_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
				     KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				     const KammoGUI::MotionEvent &event);

		Row(GnuVGListView *parent, const std::string &text, int row_index, const std::string &id);
	public:
		std::string text;

		static Row *create(GnuVGListView *parent, int row_index, const std::string &text);
	};

	friend class Row;

	double offset, min_offset;

	std::vector<Row *> rows;

	KammoGUI::GnuVGCanvas::SVGMatrix transform_t, shade_transform_t;

	KammoGUI::GnuVGCanvas::ElementReference shade_layer;
	KammoGUI::GnuVGCanvas::ElementReference title_text;
	KammoGUI::GnuVGCanvas::ElementReference cancel_button;

	void *callback_context;
	std::function<void(void *context, bool, int row_number, const std::string &)> listview_callback;
	std::function<void(bool, int row_number, const std::string &)> listview_callback_new;

	void show();
	void hide();


	static void on_cancel_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
				    KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				    const KammoGUI::MotionEvent &event);

	void row_selected(int row_index, const std::string &selected_text);

public:
	GnuVGListView(KammoGUI::GnuVGCanvas *cnv);

	virtual void on_resize();
	virtual void on_render();

	void clear();
	void add_row(const std::string &new_row);
	void select_from_list(
		const std::string &title,
		void *callback_context,
		std::function<void(void *context, bool selected, int row_number, const std::string &)> listview_callback);
	void select_from_list(
		const std::string &title,
		std::function<void(bool selected, int row_number, const std::string &)> listview_callback);
};

#endif
