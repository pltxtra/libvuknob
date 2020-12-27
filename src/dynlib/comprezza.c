/*
 * SATAN, Signal Applications To Any Network
 * Copyright (C) 2003 by Anton Persson & Johan Thim
 * Copyright (C) 2005 by Anton Persson
 * Copyright (C) 2006 by Anton Persson & Ted Björling
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

/***
 * Compression algorithm based on compressor.cpp
 * as found in 'qaac - CLI QuickTime AAC/ALAC encoder'
 * on github.
 *
 * qaac is in the Public Domain.
 *
 ***/

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#error "CAN'T FIND config.h"
#endif

#include <math.h>
#include <fixedpointmath.h>
#include "dynlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI M_PI

USE_SATANS_MATH

#include "libfilter.c"

typedef struct _XpData {
	// settings
	float dry;
	float threshold_float, knee_width, ratio;
	float attack, release; // milliseconds
	float in_gain, out_gain; // dB

	// calculated values
        FTYPE threshold, threshold_lo, threshold_hi, slope, knee_factor;
	FTYPE smooth_a, smooth_r;

	struct delayLineMono *left_delay;
	struct delayLineMono *right_delay;

	float freq;

} XpData;

void *init(MachineTable *mt, const char *name) {
	/* Allocate and initiate instance data here */
	XpData *data = (XpData *)malloc(sizeof(XpData));
	memset(data, 0, sizeof(XpData));

	// default parameters
	data->dry = 0.0;
	data->threshold_float = -4.5f;
	data->knee_width = 0.4f;
	data->ratio = 2.25f;
	data->attack = 1.5f;
	data->release = 2.25f;

	SETUP_SATANS_MATH(mt);

	data->left_delay = create_delayLineMono(30);
	data->right_delay = create_delayLineMono(30);

	/* return pointer to instance data */
	return (void *)data;
}

void *get_controller_ptr(MachineTable *mt, void *data,
			 const char *name,
			 const char *group) {
	XpData *xd = (XpData *)data;

	if(strcmp("comp", name) == 0)
		return &(xd->threshold);

	if(strcmp("width", name) == 0)
		return &(xd->knee_width);

	if(strcmp("ratio", name) == 0)
		return &(xd->ratio);

	if(strcmp("attack", name) == 0)
		return &(xd->attack);

	if(strcmp("decay", name) == 0)
		return &(xd->release);

	if(strcmp("dry", name) == 0)
		return &(xd->dry);

	if(strcmp("input_gain", name) == 0)
		return &(xd->in_gain);

	if(strcmp("output_gain", name) == 0)
		return &(xd->out_gain);

	return NULL;
}

void reset(MachineTable *mt, void *data) {
	return; /* nothing to do... */
}

static inline FTYPE calc_gain(XpData *xd, FTYPE dB) {
        if (dB < xd->threshold_lo)
		return ftoFTYPE(0.0f);
        else if (dB > xd->threshold_hi)
		return mulFTYPE(xd->slope, (dB - xd->threshold));
        else {
		FTYPE delta = dB - xd->threshold_lo;
		delta = mulFTYPE(delta, delta);
		return mulFTYPE(delta, xd->knee_factor);
        }
}

static inline FTYPE smooth(XpData *xd, FTYPE x, FTYPE alpha_a, FTYPE alpha_r) {
	FTYPE r = mulFTYPE(alpha_r, xd->smooth_r) + mulFTYPE(itoFTYPE(1) - alpha_r, x);
	xd->smooth_r = x < r ? x : r;

	xd->smooth_a = mulFTYPE(alpha_a, xd->smooth_a) + mulFTYPE(itoFTYPE(1) - alpha_a, xd->smooth_r);

	return xd->smooth_a;
}

void execute(MachineTable *mt, void *data) {
	XpData *xd = (XpData *)data;

      	SignalPointer *s = mt->get_input_signal(mt, "Stereo");
	SignalPointer *os = mt->get_output_signal(mt, "Stereo");

	if(os == NULL)
		return;

	FTYPE *ou = mt->get_signal_buffer(os);
	int ol = mt->get_signal_samples(os);
	int oc = mt->get_signal_channels(os);

	if(s == NULL) {
		// just clear output, then return
		int t, c;
		for(t = 0; t < ol; t++) {
			for(c = 0; c < oc; c++) {
				ou[t * oc + c] = itoFTYPE(0);
			}
		}
		return;
	}


	FTYPE *in = mt->get_signal_buffer(s);
	int ic = mt->get_signal_channels(s);

	xd->freq = mt->get_signal_frequency(os);

	xd->slope = ftoFTYPE((1.0f - xd->ratio) / xd->ratio);

	xd->knee_factor = divFTYPE(xd->slope, ftoFTYPE(xd->knee_width * 2.0f));

	xd->threshold = ftoFTYPE(xd->threshold_float);
	xd->threshold_lo = xd->threshold - ftoFTYPE(xd->knee_width / 2.0f);
	xd->threshold_hi = xd->threshold + ftoFTYPE(xd->knee_width / 2.0f);

	FTYPE alpha_a =
		ftoFTYPE(
			xd->attack > 0.0f ? expf(-1.0f / (xd->attack * xd->freq / 1000.0f)) : 0.0f
			);
	FTYPE alpha_r =
		ftoFTYPE(
			xd->release > 0.0 ? expf(-1.0f / (xd->release * xd->freq / 1000.0f)) : 0.0f
			);

	FTYPE ing = DB2SAM(ftoFTYPE(xd->in_gain));
	FTYPE outg = DB2SAM(ftoFTYPE(xd->out_gain));

	int i;

	for(i = 0; i < ol; i++) {
		// get the two current samples
		FTYPE left, right;

		left = mulFTYPE(ing, in[i * ic + 0]);
		right = mulFTYPE(ing, in[i * ic + 1]);

		// put into delay line
		delayLineMonoPut(xd->left_delay, left);
		delayLineMonoPut(xd->right_delay, right);

		// get amplitude at current position
		FTYPE amp_l = ABS_FTYPE(left);
		FTYPE amp_r = ABS_FTYPE(right);
		FTYPE amp = amp_l > amp_r ? amp_l : amp_r;

		// convert to dB
		amp = SAM2DB(amp);

		// calculate the compressor gain
		FTYPE gain = calc_gain(xd, amp);

		// smooth the gain and convert from dB
		gain = DB2SAM(smooth(xd, gain, alpha_a, alpha_r));

		// get data from delay line
		left = delayLineMonoGet(xd->left_delay);
		right = delayLineMonoGet(xd->right_delay);

		// apply compressor gain
		ou[i * oc + 0] =
			mulFTYPE(ftoFTYPE(1.0) - xd->dry,
				 mulFTYPE(gain, left)
				)
			+
			mulFTYPE(xd->dry, left)
			;
		ou[i * oc + 1] =
			mulFTYPE(ftoFTYPE(1.0) - xd->dry,
				 mulFTYPE(gain, right)
				)
			+
			mulFTYPE(xd->dry, right)
			;

		// apply output gain
		ou[i * oc + 0] *= outg;
		ou[i * oc + 1] *= outg;
	}
}

void delete(void *data) {
	if(data) {
		XpData *xd = (XpData *)data;
		if(xd->left_delay) delayLineMonoFree(xd->left_delay);
		if(xd->right_delay) delayLineMonoFree(xd->right_delay);

		/* free instance data here */
		free(data);
	}
}
