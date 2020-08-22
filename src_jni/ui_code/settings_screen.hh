/*
 * vu|KNOB
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

#ifndef VUKNOB_SETTINGS_SCREEN
#define VUKNOB_SETTINGS_SCREEN

#include <map>
#include <gnuVGcanvas.hh>
#include "../common.hh"
#include "../engine_code/global_control_object.hh"
#include "../engine_code/abstract_knob.hh"
#include "knob_instance.hh"

typedef RemoteInterface::ClientSpace::GlobalControlObject GCO;
typedef RemoteInterface::ClientSpace::AbstractKnob AbstractKnob;

class SettingsScreen
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public RemoteInterface::Context::ObjectSetListener<GCO>
	, public RemoteInterface::ClientSpace::GlobalControlObject::GlobalControlListener
	, public std::enable_shared_from_this<SettingsScreen>
{

private:
	static SettingsScreen *singleton;

	/* This is a hack class for letting me copy
	 * code from the KnobEditor class...
	 */
	class BMKnob : public AbstractKnob {
	private:
		double min, max;
		std::string title;
		std::function<void(double new_value)> set_value_callback;
		std::function<double()> get_value_callback;
		std::function<void()> new_value_callback;
	public:
		BMKnob(double _min, double _max,
		       std::string _title,
		       std::function<void(double new_value)> _set_value_callback,
		       std::function<double()> _get_value_callback
		       )
			: min(_min), max(_max)
			, title(_title)
			, set_value_callback(_set_value_callback)
			, get_value_callback(_get_value_callback)
			{}

		void new_value_exists() {
			if(new_value_callback)
				new_value_callback();
		}

		virtual void set_callback(std::function<void()> cb) override {
			new_value_callback = cb;
		}

		virtual void set_value_as_double(double new_value_d) override {
			int new_value = (int)new_value_d;
			if(set_value_callback)
				set_value_callback(new_value);
		}
		virtual double get_value() override {
			if(get_value_callback)
				return get_value_callback();
			else
				return min;
		}
		virtual double get_min() override {
			return min;
		}
		virtual double get_max() override {
			return max;
		}
		virtual double get_step() override {
			return 1;
		}

		virtual std::string get_title() override {
			return title;
		}

		virtual std::string get_value_as_text() override {
			int i_value = 0;
			if(get_value_callback)
				i_value = get_value_callback();
			std::stringstream ss;
			ss <<  i_value;
			return ss.str();
		}
	};

	std::weak_ptr<GCO> gco_w;
	KammoGUI::GnuVGCanvas::ElementReference root, knob_container, knob_template, button_container;
	KammoGUI::GnuVGCanvas::ElementReference new_button, load_button, save_button;
	KammoGUI::GnuVGCanvas::ElementReference export_button, samples_button, demo_button;
	KammoGUI::GnuVGCanvas::ElementReference quit_button, about_button, tutorial_button;
	KammoGUI::GnuVGCanvas::SVGRect document_size;

	double scaling; // graphical scaling factor
	double finger_width = 10.0, finger_height = 10.0; // sizes in pixels
	int canvas_width_fingers = 8, canvas_height_fingers = 8; // sizes in "fingers"
	float canvas_w_inches, canvas_h_inches; // sizes in inches
	int canvas_w, canvas_h; // sizes in pixels

	std::vector<std::shared_ptr<BMKnob>> knobs;
	std::vector<std::shared_ptr<KnobInstance> > knob_instances;

	void create_knobs();
	void internal_show();
public:
	SettingsScreen(KammoGUI::GnuVGCanvas* cnvs);

	virtual void on_resize() override;
	virtual void on_render() override;

	virtual void object_registered(std::shared_ptr<GCO> gco) override;
	virtual void object_unregistered(std::shared_ptr<GCO> gco) override;

	virtual void bpm_changed(int bpm) override;
	virtual void lpb_changed(int lpb) override;
	virtual void shuffle_factor_changed(int shuffle_factor) override;

	static std::shared_ptr<SettingsScreen> get_settings_screen(KammoGUI::GnuVGCanvas* cnvs);

	static void hide();
	static void show();
};

#endif
