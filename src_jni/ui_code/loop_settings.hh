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

#ifndef LOOP_SETTINGS_CLASS
#define LOOP_SETTINGS_CLASS

#include <gnuVGcanvas.hh>
#include <functional>
#include "engine_code/global_control_object.hh"

typedef RemoteInterface::ClientSpace::GlobalControlObject GCO;

class LoopSettings
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public RemoteInterface::Context::ObjectSetListener<GCO>
	, public GCO::GlobalControlListener
	, public std::enable_shared_from_this<LoopSettings>
{
private:
	std::weak_ptr<GCO> gco_w;
	bool loop_state;

	KammoGUI::GnuVGCanvas::ElementReference loopSettingsButton;
	KammoGUI::GnuVGCanvas::ElementReference loopParameterSettingButton;

	KammoGUI::GnuVGCanvas::ElementReference playAsLoopIcon, playAsEndlessIcon;
	KammoGUI::GnuVGCanvas::ElementReference startStopIcon, editStartStopIcon;

	double first_selection_x, first_selection_y;
	bool is_a_tap;

	KammoGUI::GnuVGCanvas::SVGMatrix base_transform_t;

	static bool on_button_event(KammoGUI::GnuVGCanvas::SVGDocument *source,
				    KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				    const KammoGUI::MotionEvent &event);

	void refresh_loop_state_icons();
public:

	LoopSettings(KammoGUI::GnuVGCanvas *cnv);
	~LoopSettings();

	virtual void on_render();
	virtual void on_resize();

	void show();
	void hide();

	virtual void loop_state_changed(bool new_state);

	virtual void object_registered(std::shared_ptr<GCO> gco) override;
	virtual void object_unregistered(std::shared_ptr<GCO> gco) override;
};

#endif
