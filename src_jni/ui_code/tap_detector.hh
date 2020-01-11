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

#include <math.h>
#include <gnuVGcanvas.hh>
#include "time_measure.hh"

class TapDetector {
private:
	mutable TimeMeasure timer;
	mutable double event_start_x, event_start_y;

public:
	bool analyze_events(const KammoGUI::MotionEvent &event) const;
};

#endif
