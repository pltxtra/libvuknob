/*
 * VuKNOB
 * Copyright (C) 2019 by Anton Persson
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

#ifndef VUKNOB_TAP_DETECTOR
#define VUKNOB_TAP_DETECTOR

#include <gnuVGcanvas.hh>
#include "time_measure.hh"

class TapDetector {
private:
	TimeMeasure timer;
	double event_start_x, event_start_y;

public:
	bool analyze_events(const KammoGUI::MotionEvent &event) {
		auto event_current_x = event.get_x();
		auto event_current_y = event.get_y();
		switch(event.get_action()) {
		case KammoGUI::MotionEvent::ACTION_CANCEL:
		case KammoGUI::MotionEvent::ACTION_OUTSIDE:
		case KammoGUI::MotionEvent::ACTION_POINTER_DOWN:
		case KammoGUI::MotionEvent::ACTION_POINTER_UP:
		case KammoGUI::MotionEvent::ACTION_MOVE:
			break;
		case KammoGUI::MotionEvent::ACTION_DOWN:
			event_start_x = event_current_x;
			event_start_y = event_current_y;
			timer.reset();
			break;
		case KammoGUI::MotionEvent::ACTION_UP:
			auto event_delta_x = event_current_x - event_start_x;
			auto event_delta_y = event_current_y - event_start_y;
			auto event_delta = sqrt(event_delta_x * event_delta_x + event_delta_y * event_delta_y);

			if(event_delta < 5.0 && timer.seconds_since_start() < 0.5) {
				return true;
			}
			break;
		}
		return false;
	}
};

#endif
