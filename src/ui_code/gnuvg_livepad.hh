/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2014 by Anton Persson
 *
 * About SATAN:
 *   Originally developed as a small subproject in
 *   a course about applied signal processing.
 * Original Developers:
 *   Anton Persson
 *   Johan Thim
 *
 * http://www.733kru.org/
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

#ifndef GNUVGLIVEPAD_CLASS
#define GNUVGLIVEPAD_CLASS

#include <gnuVGcanvas.hh>

#include "gnuvg_listview.hh"
#include "gnuvg_scaleeditor.hh"
#include "engine_code/sequence.hh"

#define HZONES PAD_HZONES
#define VZONES PAD_VZONES

typedef RemoteInterface::ClientSpace::Sequence RISequence;

class GnuVGLivePad
	: public KammoGUI::GnuVGCanvas::SVGDocument
	, public RemoteInterface::Context::ObjectSetListener<RISequence>
	, public RemoteInterface::GlobalControlObject::PlaybackStateListener
	, public KammoGUI::SensorEvent::Listener {
private:
	KammoGUI::GnuVGCanvas::SVGMatrix transform_m;

	// canvas coordinates for the actual LivePad part
	double doc_x1, doc_y1, doc_x2, doc_y2, doc_scale;

	int octave, scale_index;
	std::string scale_name;

	bool record, quantize, do_pitch_bend;
	std::string arp_pattern, controller, pitch_bend_controller;

	RISequence::ChordMode_t chord_mode;
	RISequence::ArpeggioDirection_t arp_direction;

	bool is_recording = false, is_playing = false;

	// last event data
	bool f_active[10];
	float l_ev_x[10], l_ev_y[10], l_ev_z;

	KammoGUI::GnuVGCanvas::ElementReference graphArea_element;
	KammoGUI::GnuVGCanvas::SVGRect graphArea_viewport;
	std::shared_ptr<RISequence> mseq;

	std::set<std::weak_ptr<RISequence>,
		 std::owner_less<std::weak_ptr<RISequence> > >msequencers; // all known sequencers

	GnuVGListView *listView;
	GnuVGScaleEditor *scale_editor;

	KammoGUI::GnuVGCanvas::ElementReference background_element;

	KammoGUI::GnuVGCanvas::ElementReference octaveUp_element;
	KammoGUI::GnuVGCanvas::ElementReference octaveDown_element;
	KammoGUI::GnuVGCanvas::ElementReference clear_element;
	KammoGUI::GnuVGCanvas::ElementReference recordGroup_element;
	KammoGUI::GnuVGCanvas::ElementReference quantize_element;

	KammoGUI::GnuVGCanvas::ElementReference selectMachine_element;
	KammoGUI::GnuVGCanvas::ElementReference selectArpPattern_element;
	KammoGUI::GnuVGCanvas::ElementReference toggleChord_element;
	KammoGUI::GnuVGCanvas::ElementReference togglePitchBend_element;
	KammoGUI::GnuVGCanvas::ElementReference toggleArpDirection_element;
	KammoGUI::GnuVGCanvas::ElementReference selectScale_element;
	KammoGUI::GnuVGCanvas::ElementReference selectController_element;
	KammoGUI::GnuVGCanvas::ElementReference selectMenu_element;

	void refresh_scale_key_names();

	void select_machine();
	void select_arp_pattern();
	void select_scale();
	void select_controller();
	void select_menu();

	void copy_to_loop();

	void refresh_record_indicator();
	void refresh_quantize_indicator();
	void refresh_scale_indicator();
	void refresh_controller_indicator();
	void refresh_machine_settings();

	void octave_up();
	void octave_down();
	void toggle_record();
	void toggle_quantize();
	void toggle_chord();
	void toggle_arp_direction();
	void toggle_pitch_bend();

	void switch_MachineSequencer(std::shared_ptr<RISequence> mseq);

	static void yes(void *ctx);
	static void no(void *ctx);
	void ask_clear_pad();

	static void button_on_event(KammoGUI::GnuVGCanvas::SVGDocument *source, KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				    const KammoGUI::MotionEvent &event);
	static void graphArea_on_event(SVGDocument *source, KammoGUI::GnuVGCanvas::ElementReference *e_ref,
				       const KammoGUI::MotionEvent &event);

public:
	GnuVGLivePad(KammoGUI::GnuVGCanvas *cnv);
	~GnuVGLivePad();

	static std::shared_ptr<GnuVGLivePad> create(KammoGUI::GnuVGCanvas *cnvs);

	void show();
	void hide();

	virtual void on_resize();
	virtual void on_render();

	virtual void object_registered(std::shared_ptr<RISequence> ri_seq) override;
	virtual void object_unregistered(std::shared_ptr<RISequence> ri_seq) override;

	// RemoteInterface::GlobalControlObject::PlaybackStateListener
	virtual void playback_state_changed(bool is_playing) override;
	virtual void recording_state_changed(bool is_recording) override;
	virtual void periodic_playback_update(int current_line) override { /* empty */ }

	virtual void on_sensor_event(std::shared_ptr<KammoGUI::SensorEvent> event) override;
};

#endif
