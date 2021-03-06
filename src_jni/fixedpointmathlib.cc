/* Autogenerated look-up-table for natural logarithm, log(x), for 0.0 < x < 1.0 */

#ifdef ANDROID
#include <android/log.h>
#endif

#include <stdint.h>
#include <stdio.h>

#define HAS_AUTOGEN_NAT_LOG_LUT

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif

#define small_ln_fp8p24(x) (ln_base_fx_table[(x & 0x00fffc00) >> 10] + mulfp8p24((x & 0x3ff), ln_step_fx_table[(x & 0x00fffc00) >> 10]))
#define neg_exp_fp8p24(A) (_fp8p24tmp = -A) ? ((_fp8p24tmp & 0xf8000000) ? ftofp8p24(0.0) : (pow_base_fx_table[(_fp8p24tmp & 0x07ffe000) >> 13] + mulfp8p24((_fp8p24tmp & 0x1fff), pow_step_fx_table[(_fp8p24tmp & 0x07ff1000) >> 13]))) : ftofp8p24(0.0) 

#include "fixedpointmath.h"
#include "fixedpointmathlib_ln_table.cc"
#include "fixedpointmathlib_exp_table.cc"

fp8p24_t _fp8p24tmp;

fp8p24_t satan_powfp8p24(fp8p24_t x, fp8p24_t y) {
//	fp8p24_t ln_d;
	fp8p24_t ln_r;
	fp8p24_t mul_r;

	if(x & 0x7f000000) {
		ln_r = lnfp8p24(x);
	} else {
		ln_r  = small_ln_fp8p24(x);
	}
	mul_r = mulfp8p24(y, ln_r);

	
	/*
	if(x > ftofp8p24(1.0)) {
		__android_log_print(ANDROID_LOG_INFO, "SATAN|NDK",
				    "Warning, x larger than one. (%f, %f) ln_r = %f, mul_r = %f, exp = %f",
				    fp8p24tof(x),
				    fp8p24tof(y),
				    fp8p24tof(ln_r),
				    fp8p24tof(mul_r),
				    fp8p24tof(expufp8p24(mul_r))
			);
	}
	*/
/*
	__android_log_print(ANDROID_LOG_INFO, "SATAN|NDK",
			    "satan_powfp8p24: %f, %f, %f, %f\n",
			    fp8p24tof(ln_d),
			    fp8p24tof(ln_r),
			    fp8p24tof(mul_r),
			    fp8p24tof(expufp8p24(mul_r))
		);
	fflush(0);
*/	
	if(mul_r & 0x80000000) {
		return neg_exp_fp8p24(mul_r);
	}
	return expufp8p24(mul_r);
}
	
