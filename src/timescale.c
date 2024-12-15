/*!
 * @defgroup timescale Time-scale calculation routines
 * @{
 *
 * Conversion between discrete samples and real-time.  SSTV timings are given
 * in real-world seconds (actually, nanoseconds) as time units but we need to
 * know what that is in the number of discrete *samples*.  There is rounding
 * applied to handle fractional samples.
 *
 * Time periods down to nanosecond precision may be specified.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <libsstvenc/timescale.h>
#include <math.h>

uint64_t sstvenc_ts_unit_scale(uint8_t unit) {
	switch (unit) {
	case SSTVENC_TS_UNIT_SECONDS:
		return 1;
	case SSTVENC_TS_UNIT_MILLISECONDS:
		return 1000;
	case SSTVENC_TS_UNIT_MICROSECONDS:
		return 1000000;
	case SSTVENC_TS_UNIT_NANOSECONDS:
		return 1000000000;
	default:
		/* Not supported */
		assert(0);
	}
}

uint32_t sstvenc_ts_clamp_samples(uint64_t samples) {
	if (samples > UINT32_MAX) {
		return SSTVENC_TS_INFINITE;
	} else {
		return samples;
	}
}

uint32_t sstvenc_ts_unit_to_samples(double time, uint32_t sample_rate,
				    uint8_t unit) {
	if (time == INFINITY) {
		return SSTVENC_TS_INFINITE;
	} else {
		return sstvenc_ts_clamp_samples(
		    ((uint64_t)((time * sample_rate) + 0.5))
		    / sstvenc_ts_unit_scale(unit));
	}
}

double sstvenc_ts_samples_to_unit(uint32_t samples, uint32_t sample_rate,
				  uint8_t unit) {
	if (samples == SSTVENC_TS_INFINITE) {
		return INFINITY;
	} else {
		uint64_t n = samples * sstvenc_ts_unit_scale(unit);
		return ((double)n) / ((double)sample_rate);
	}
}

/*! @} */
