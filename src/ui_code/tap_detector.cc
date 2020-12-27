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

#include "tap_detector.hh"

//#define __DO_SATAN_DEBUG
#include "satan_debug.hh"

bool TapDetector::analyze_events(const KammoGUI::MotionEvent &event) const {
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
		auto time_delta = timer.seconds_since_start();
		SATAN_DEBUG("event_delta: %f, seconds sine start: %f\n", event_delta, time_delta);
		if(event_delta < 15.0 && time_delta < 0.1) {
			return true;
		}
		break;
	}
	return false;
}
