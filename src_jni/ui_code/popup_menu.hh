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

#ifndef VUKNOB_POPUP_MENU
#define VUKNOB_POPUP_MENU

#include <map>
#include <gnuVGcanvas.hh>

class PopupMenu
	: public KammoGUI::GnuVGCanvas::SVGDocument
{

private:
	class ItemInstance
		: public std::enable_shared_from_this<ItemInstance>
	{
	private:
		KammoGUI::GnuVGCanvas::ElementReference svg_reference, background, text, tspan;
		int offset;

	public:
		ItemInstance(KammoGUI::GnuVGCanvas::ElementReference &elref, int offset, const std::string &content,
			     std::function<void(int selection)> callback
			);

		virtual ~ItemInstance() {
			svg_reference.drop_element();
		}

		static std::shared_ptr<ItemInstance> create_item_instance(
			KammoGUI::GnuVGCanvas::ElementReference &item_container,
			KammoGUI::GnuVGCanvas::ElementReference &item_template,
			std::function<void(int selection)> callback,
			int offset, const std::string &content);

		void set_graphic_parameters(double finger_height, double canvas_w, double canvas_h);
	};

	KammoGUI::GnuVGCanvas::ElementReference root, item_container, item_template, backdrop;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	std::vector<std::shared_ptr<ItemInstance> > items;

	double scaling; // graphical scaling factor
	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

	static PopupMenu *singleton;
	PopupMenu(KammoGUI::GnuVGCanvas* cnvs);
	void internal_show_menu(std::vector<std::string> items, std::function<void(int selection)> callback);
public:

	virtual void on_resize() override;
	virtual void on_render() override;

	static void prepare(KammoGUI::GnuVGCanvas* cnvs);
	static void show_menu(std::vector<std::string> items, std::function<void(int selection)> callback);
};

#endif
