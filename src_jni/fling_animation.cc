/*
 * vu|KNOB
 * (C) 2015 by Anton Persson
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

#include <iostream>

#include "fling_animation.hh"
#include "common.hh"

FlingAnimation::FlingAnimation(
	float speed, float _duration,
	std::function<void(float pixels_changed)> _callback) :
	KammoGUI::Animation(_duration), start_speed(speed), duration(_duration),
	current_speed(0.0f), deacc((start_speed < 0) ? (-FLING_DEACCELERATION) : (FLING_DEACCELERATION)),
	last_time(0.0f), callback(_callback) {}

void FlingAnimation::new_frame(float progress) {
	current_speed = start_speed - deacc * progress * duration;

	float time_diff = (progress * duration) - last_time;
	float pixels_change = time_diff * current_speed;

	callback(pixels_change);

	last_time = progress * duration;
}

void FlingAnimation::on_touch_event() {
	stop();
}
