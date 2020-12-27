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

#ifndef FLING_ANIMATION_CLASS
#define FLING_ANIMATION_CLASS

#include <kamogui.hh>
#include <functional>

class FlingAnimation : public KammoGUI::Animation {
private:
	float start_speed, duration, current_speed, deacc, last_time;

	std::function<void(float pixels_changed)> callback;

public:
	FlingAnimation(float speed, float duration,
		       std::function<void(float pixels_changed)> callback);
	virtual void new_frame(float progress);
	virtual void on_touch_event();
};

#endif
