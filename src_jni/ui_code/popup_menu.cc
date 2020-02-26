/*
 * vu|KNOB
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include "popup_menu.hh"
#include "svg_loader.hh"
#include "common.hh"
#include "tap_detector.hh"

#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

static TapDetector tap_detector;
PopupMenu* PopupMenu::singleton = nullptr;

/***************************
 *
 *  Class PopupMenu::Knobinstance
 *
 ***************************/

PopupMenu::ItemInstance::ItemInstance(
	KammoGUI::GnuVGCanvas::ElementReference &elref, int _offset,
	std::function<void(int selection)> callback
	)
	: svg_reference(elref)
	, offset(_offset)
{
	svg_reference.set_event_handler(
		[callback, this](
			KammoGUI::GnuVGCanvas::SVGDocument *NOT_USED(source),
			KammoGUI::GnuVGCanvas::ElementReference *NOT_USED(e_ref),
			const KammoGUI::MotionEvent &event) {
			if(tap_detector.analyze_events(event)) {
				callback(offset);
			}
		}
		);
}

auto PopupMenu::ItemInstance::create_item_instance(
	KammoGUI::GnuVGCanvas::ElementReference &item_container,
	KammoGUI::GnuVGCanvas::ElementReference &item_template,
	std::function<void(int selection)> callback,
	int offset) -> std::shared_ptr<ItemInstance> {

	char bfr[32];
	snprintf(bfr, 32, "itm_%d", offset);

	KammoGUI::GnuVGCanvas::ElementReference new_graphic =
		item_container.add_element_clone(bfr, item_template);

	SATAN_DEBUG("PopupMenu::ItemInstance::create_item_instance() -- bfr: %s\n", bfr);

	auto new_item = std::make_shared<ItemInstance>(new_graphic, offset, callback);
	return new_item;
}

/***************************
 *
 *  Class PopupMenu
 *
 ***************************/

PopupMenu::PopupMenu(KammoGUI::GnuVGCanvas* cnvs)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/popupMenu.svg"), cnvs)
{
	backdrop = KammoGUI::GnuVGCanvas::ElementReference(this, "backdrop");
	item_container = KammoGUI::GnuVGCanvas::ElementReference(this, "itemContainer");
	item_template = KammoGUI::GnuVGCanvas::ElementReference(this, "itemTemplate");
	root = KammoGUI::GnuVGCanvas::ElementReference(this);
}

void PopupMenu::internal_show_menu(std::vector<std::string> new_items, std::function<void(int selection)> callback) {
	items.clear();
	int offset = 0;
	for(auto new_item : new_items) {
		items.push_back(ItemInstance::create_item_instance(item_container, item_template, callback, offset));
		++offset;
	}
}

void PopupMenu::on_resize() {
	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	double tmp;

	tmp = canvas_w_inches / INCHES_PER_FINGER;
	canvas_width_fingers = (int)tmp;
	tmp = canvas_h_inches / INCHES_PER_FINGER;
	canvas_height_fingers = (int)tmp;

	tmp = canvas_w / ((double)canvas_width_fingers);
	finger_width = tmp;
	tmp = canvas_h / ((double)canvas_height_fingers);
	finger_height = tmp;

	// get data
	root.get_viewport(document_size);
	scaling = finger_height / document_size.height;
}

void PopupMenu::on_render() {
}

void PopupMenu::prepare(KammoGUI::GnuVGCanvas* cnvs) {
	SATAN_DEBUG("PopupMenu::prepare() 1\n");
	if(singleton) return;
	SATAN_DEBUG("PopupMenu::prepare() 2\n");
	singleton = new PopupMenu(cnvs);
	SATAN_DEBUG("PopupMenu::prepare() 3\n");
}

void PopupMenu::show_menu(std::vector<std::string> new_items, std::function<void(int selection)> callback) {
	if(singleton)
		singleton->internal_show_menu(new_items, callback);
}