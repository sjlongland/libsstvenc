#ifndef _SSTVENC_TIMESCALE_H
#define _SSTVENC_TIMESCALE_H

/*!
 * Time-scale routines, for conversion between discrete samples, seconds and
 * milliseconds.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <stdint.h>

/*!
 * Infinite time scale.  We set this to the maximum number of samples, which
 * at 48kHz sample rates, equates to about a day.
 */
#define SSTVENC_TS_INFINITE	     UINT32_MAX

#define SSTVENC_TS_UNIT_SECONDS	     (0)
#define SSTVENC_TS_UNIT_MILLISECONDS (1)

static inline uint64_t sstvenc_ts_unit_scale(uint8_t unit) {
	switch (unit) {
	case SSTVENC_TS_UNIT_SECONDS:
		return 1;
	case SSTVENC_TS_UNIT_MILLISECONDS:
		return 1000;
	default:
		assert(0);
	}
}

static inline uint32_t sstvenc_ts_clamp_samples(uint64_t samples) {
	if (samples > UINT32_MAX) {
		return SSTVENC_TS_INFINITE;
	} else {
		return samples;
	}
}

static inline uint32_t
sstvenc_ts_unit_to_samples(double time, uint32_t sample_rate, uint8_t unit) {
	if (time == INFINITY) {
		return SSTVENC_TS_INFINITE;
	} else {
		return sstvenc_ts_clamp_samples(
		    ((uint64_t)(time * sample_rate))
		    / sstvenc_ts_unit_scale(unit));
	}
}

static inline double sstvenc_ts_samples_to_unit(uint32_t samples,
						uint32_t sample_rate,
						uint8_t	 unit) {
	if (samples == SSTVENC_TS_INFINITE) {
		return INFINITY;
	} else {
		uint64_t n = samples * sstvenc_ts_unit_scale(unit);
		return ((double)n) / ((double)sample_rate);
	}
}

#endif
