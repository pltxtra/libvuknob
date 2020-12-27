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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include <sstream>
#include <kamogui.hh>
#include <functional>

#include "gnuvg_scaleeditor.hh"
#include "svg_loader.hh"
#include "scales.hh"
//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

#include "common.hh"

GnuVGScaleEditor::Key::Key(GnuVGScaleEditor *parent, const std::string &id,
			   int index,
			   std::function<void(bool note_on, int index)> callback)
	: ElementReference(parent, id) {
	set_event_handler(
		[id, index, callback](KammoGUI::GnuVGCanvas::SVGDocument *source,
				      KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				      const KammoGUI::MotionEvent &event) {
			switch(event.get_action()) {
			case KammoGUI::MotionEvent::ACTION_CANCEL:
			case KammoGUI::MotionEvent::ACTION_OUTSIDE:
			case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
			case KammoGUI::MotionEvent::ACTION_POINTER_UP:
			case KammoGUI::MotionEvent::ACTION_MOVE:
				break;
			case KammoGUI::MotionEvent::ACTION_DOWN:
				callback(true, index);
				break;
			case KammoGUI::MotionEvent::ACTION_UP:
				callback(false, index);
				break;
			}
		}
		);
}

GnuVGScaleEditor::Setting::Setting(GnuVGScaleEditor *parent, int _offset, const std::string &id,
				   std::function<void(Setting*)> set_callback,
				   std::function<void(bool note_on,
						      int key_index)> play_callback)
	: ElementReference(parent, id + "set")
	, key(0), offset(_offset)
{
	play_button = ElementReference(parent, id + "play");
	setting_text = play_button.find_child_by_class("key_text");

	set_event_handler(
		[this, id, set_callback](KammoGUI::GnuVGCanvas::SVGDocument *source,
					 KammoGUI::GnuVGCanvas::ElementReference *e_ref,
					 const KammoGUI::MotionEvent &event) {
			SATAN_DEBUG("Setting pressed: %s\n", id.c_str());
			switch(event.get_action()) {
			case KammoGUI::MotionEvent::ACTION_CANCEL:
			case KammoGUI::MotionEvent::ACTION_OUTSIDE:
			case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
			case KammoGUI::MotionEvent::ACTION_POINTER_UP:
			case KammoGUI::MotionEvent::ACTION_MOVE:
			case KammoGUI::MotionEvent::ACTION_DOWN:
				break;
			case KammoGUI::MotionEvent::ACTION_UP:
				set_callback(this);
				break;
			}
		}
		);

	play_button.set_event_handler(
		[this, play_callback](KammoGUI::GnuVGCanvas::SVGDocument *source,
				      KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				      const KammoGUI::MotionEvent &event) {
			switch(event.get_action()) {
			case KammoGUI::MotionEvent::ACTION_CANCEL:
			case KammoGUI::MotionEvent::ACTION_OUTSIDE:
			case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
			case KammoGUI::MotionEvent::ACTION_POINTER_UP:
			case KammoGUI::MotionEvent::ACTION_MOVE:
				break;
			case KammoGUI::MotionEvent::ACTION_DOWN:
				play_callback(true, key);
				break;
			case KammoGUI::MotionEvent::ACTION_UP:
				play_callback(false, key);
				break;
			}
		}
		);

	set_selected(false);
	change_key(key);
}

void GnuVGScaleEditor::Setting::change_key(int key_index) {
	if(auto scalo = Scales::get_scales_object()) {

		SATAN_DEBUG("change_setting(%d) -- octave: %d\n", key_index, key_index / 12);
		key = key_index;

		std::string octave_text = ((key_index / 12) == 1) ? "2" : "1";
		std::string key_text(scalo->get_key_text(key_index));
		key_text = key_text + octave_text;

//		SATAN_DEBUG("change_setting(%d) -- octave: %d -> %s\n", key_index, key_index / 12, key_text.c_str());
		setting_text.set_text_content(key_text);
	} else {
		SATAN_ERROR("GnuVGScaleEditor::Setting::change_key() - failed to get Scales object from client.\n");
	}
}

int GnuVGScaleEditor::Setting::get_key() {
	return key;
}

void GnuVGScaleEditor::Setting::set_selected(bool is_selected) {
	SATAN_DEBUG("set_selected(%s) for %p\n", is_selected ? "true" : "false", this);
	find_child_by_class("selectIndicator").set_display(is_selected ? "inline" : "none");
	SATAN_DEBUG("    completed set_selected() for %p\n", this);
}

GnuVGScaleEditor::GnuVGScaleEditor(KammoGUI::GnuVGCanvas *cnv)
	: SVGDocument(std::string(SVGLoader::get_svg_directory() + "/ScaleEditor.svg"), cnv) {
	auto on_cancel_event = [this](KammoGUI::GnuVGCanvas::SVGDocument *source,
				      KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				      const KammoGUI::MotionEvent &event) {
		SATAN_DEBUG("cancel GnuVGScaleEditor.\n");
		hide();
	};

	shade_layer = KammoGUI::GnuVGCanvas::ElementReference(this, "shadeLayer");
	shade_layer.set_event_handler(on_cancel_event);
	no_event = KammoGUI::GnuVGCanvas::ElementReference(this, "noEvent");
	no_event.set_event_handler([](KammoGUI::GnuVGCanvas::SVGDocument *source,
				      KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				      const KammoGUI::MotionEvent &event) {
					   SATAN_DEBUG("no event area pressed.\n");
				   }
		);

	auto select_setting = [this](Setting* new_setting) {
		SATAN_DEBUG("Active Setting: %p --- New setting: %p\n", active_setting, new_setting);
		if(active_setting) active_setting->set_selected(false);
		new_setting->set_selected(true);

		active_setting = new_setting;
		SATAN_DEBUG("Active setting set.\n");
	};

	auto change_key = [this](bool note_on, int index) {
		if(note_on) {
			unsigned char midi_data[] = {
				0x90, (unsigned char)(36 + index), 0x7f
			};
			if(mseq) mseq->enqueue_midi_data(3, midi_data);
		} else {
			unsigned char midi_data[] = {
				0x80, (unsigned char)(36 + index), 0x7f
			};
			if(mseq) mseq->enqueue_midi_data(3, midi_data);
			if(active_setting) {
				active_setting->change_key(index);
			}
		}
	};

	auto play_key = [this](bool note_on, int index) {
		if(!mseq) return;

		if(note_on) {
			unsigned char midi_data[] = {
				0x90, (unsigned char)(36 + index), 0x7f
			};
			mseq->enqueue_midi_data(3, midi_data);
		} else {
			unsigned char midi_data[] = {
				0x80, (unsigned char)(36 + index), 0x7f
			};
			mseq->enqueue_midi_data(3, midi_data);
		}
	};

	keys.push_back(Key(this, "c_1",  0, change_key));
	keys.push_back(Key(this, "cs1",  1, change_key));
	keys.push_back(Key(this, "d_1",  2, change_key));
	keys.push_back(Key(this, "ds1",  3, change_key));
	keys.push_back(Key(this, "e_1",  4, change_key));
	keys.push_back(Key(this, "f_1",  5, change_key));
	keys.push_back(Key(this, "fs1",  6, change_key));
	keys.push_back(Key(this, "g_1",  7, change_key));
	keys.push_back(Key(this, "gs1",  8, change_key));
	keys.push_back(Key(this, "a_1",  9, change_key));
	keys.push_back(Key(this, "as1", 10, change_key));
	keys.push_back(Key(this, "b_1", 11, change_key));

	keys.push_back(Key(this, "c_2", 12, change_key));
	keys.push_back(Key(this, "cs2", 13, change_key));
	keys.push_back(Key(this, "d_2", 14, change_key));
	keys.push_back(Key(this, "ds2", 15, change_key));
	keys.push_back(Key(this, "e_2", 16, change_key));
	keys.push_back(Key(this, "f_2", 17, change_key));
	keys.push_back(Key(this, "fs2", 18, change_key));
	keys.push_back(Key(this, "g_2", 19, change_key));
	keys.push_back(Key(this, "gs2", 20, change_key));
	keys.push_back(Key(this, "a_2", 21, change_key));
	keys.push_back(Key(this, "as2", 22, change_key));
	keys.push_back(Key(this, "b_2", 23, change_key));

	settings.push_back(new Setting(this, 0, "s1_", select_setting, play_key));
	settings.push_back(new Setting(this, 1, "s2_", select_setting, play_key));
	settings.push_back(new Setting(this, 2, "s3_", select_setting, play_key));
	settings.push_back(new Setting(this, 3, "s4_", select_setting, play_key));
	settings.push_back(new Setting(this, 4, "s5_", select_setting, play_key));
	settings.push_back(new Setting(this, 5, "s6_", select_setting, play_key));
	settings.push_back(new Setting(this, 6, "s7_", select_setting, play_key));

	active_setting = settings[0];
	active_setting->set_selected(true);

	bt_OK = KammoGUI::GnuVGCanvas::ElementReference(this, "bt_OK");
	bt_OK.set_event_handler(
		[this](KammoGUI::GnuVGCanvas::SVGDocument *source,
		       KammoGUI::GnuVGCanvas::ElementReference *e_ref,
		       const KammoGUI::MotionEvent &event) {
			if(event.get_action() != KammoGUI::MotionEvent::ACTION_UP) return;

			if(auto scalo = Scales::get_scales_object()) {
				for(auto sett : settings) {
					scalo->set_custom_scale_key(sett->get_offset(), sett->get_key());
				}
			}
			hide();
		}
		);

	hide();
}

void GnuVGScaleEditor::show(std::shared_ptr<RISequence> _mseq) {
	KammoGUI::GnuVGCanvas::ElementReference root_element = KammoGUI::GnuVGCanvas::ElementReference(this);
	root_element.set_display("inline");
	mseq = _mseq;

	if(auto scalo = Scales::get_scales_object()) {
		for(auto sett : settings) {
			sett->change_key(scalo->get_custom_scale_key(sett->get_offset()));
		}
	}
}

void GnuVGScaleEditor::hide() {
	KammoGUI::GnuVGCanvas::ElementReference root_element = KammoGUI::GnuVGCanvas::ElementReference(this);
	root_element.set_display("none");
}

void GnuVGScaleEditor::on_resize() {
	int canvas_w, canvas_h;
	float canvas_w_inches, canvas_h_inches;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	// get data
	KammoGUI::GnuVGCanvas::ElementReference root(this);
	root.get_viewport(document_size);

	auto canvas = get_canvas();
	canvas->get_size_pixels(canvas_w, canvas_h);
	canvas->get_size_inches(canvas_w_inches, canvas_h_inches);

	{ // calculate transform for the shadeLayer
		// calculate scaling factor
		double scale_w = canvas_w / (double)document_size.width;
		double scale_h = canvas_h / (double)document_size.height;

		// initiate transform_t
		shade_transform_t.init_identity();
		shade_transform_t.scale(scale_w, scale_h);
	}

	{ // calculate transform for the main part of the document
		double scaling = fit_to_inches(
			&root,
			8.0 * INCHES_PER_FINGER,
			8.0 * INCHES_PER_FINGER);

		// calculate translation
		double translate_x = (canvas_w - document_size.width * scaling) / 2.0;
		double translate_y = (canvas_h - document_size.height * scaling) / 2.0;

		// initiate transform_t
		transform_t.init_identity();
		transform_t.scale(scaling, scaling);
		transform_t.translate(translate_x, translate_y);
	}
}

void GnuVGScaleEditor::on_render() {
	{
		// Translate shade_layer, and scale it properly to fit the defined viewbox
		shade_layer.set_transform(shade_transform_t);

		// Translate GnuVGScaleEditor, and scale it properly to fit the defined viewbox
		KammoGUI::GnuVGCanvas::ElementReference main_layer(this, "mainLayer");
		main_layer.set_transform(transform_t);
	}
}
