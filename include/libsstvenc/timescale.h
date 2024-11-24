#ifndef _SSTVENC_TIMESCALE_H
#define _SSTVENC_TIMESCALE_H

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
#include <stdint.h>

/*!
 * Infinite time scale.  We set this to the maximum number of samples, which
 * at 48kHz sample rates, equates to about a day.
 */
#define SSTVENC_TS_INFINITE	     UINT32_MAX

/*!
 * @defgroup timescale_units Time-scale units
 * @{
 *
 * The supported time-scale units.
 */
#define SSTVENC_TS_UNIT_SECONDS	     (0) /*!< Seconds */
#define SSTVENC_TS_UNIT_MILLISECONDS (1) /*!< Milliseconds */
#define SSTVENC_TS_UNIT_MICROSECONDS (2) /*!< Microseconds */
#define SSTVENC_TS_UNIT_NANOSECONDS  (3) /*!< Nanoseconds */
/*!
 * @}
 */

/*!
 * Obtain the scaling factor to convert 1 second of time to the given unit.
 *
 * @param[in]	unit	The time scale of interest.  @ref timescale_units
 *
 * @returns	The number of specified time units in one second.
 */
static inline uint64_t sstvenc_ts_unit_scale(uint8_t unit) {
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

/*!
 * Clamp the given number of samples to a safe maximum.  If the value
 * is greater than that supported by a `uint32_t` data type, it is clamped
 * to @ref SSTVENC_TS_INFINITE.
 *
 * @param[in]	samples		Unclamped number of samples.
 *
 * @returns	Clamped number of samples.
 */
static inline uint32_t sstvenc_ts_clamp_samples(uint64_t samples) {
	if (samples > UINT32_MAX) {
		return SSTVENC_TS_INFINITE;
	} else {
		return samples;
	}
}

/*!
 * Convert the given time period to the number of units.
 *
 * @param[in]	time		The number of time units being converted.
 * @param[in]	sample_rate	The sample rate in hertz being used for the
 * 				discrete timebase.
 * @param[in]	unit		The time unit being measured, see
 * 				@ref timescale_units
 *
 * @returns	The number of samples needed for that time period.
 *
 * @retval	SSTVENC_TS_INFINITE	Time period is too long to represent.
 */
static inline uint32_t
sstvenc_ts_unit_to_samples(double time, uint32_t sample_rate, uint8_t unit) {
	if (time == INFINITY) {
		return SSTVENC_TS_INFINITE;
	} else {
		return sstvenc_ts_clamp_samples(
		    ((uint64_t)((time * sample_rate) + 0.5))
		    / sstvenc_ts_unit_scale(unit));
	}
}

/*!
 * Convert the given number of samples to a time period in the specified unit.
 *
 * @param[in]	samples		The number of samples being measured.
 * @param[in]	sample_rate	The sample rate in hertz being used for the
 * 				discrete timebase.
 * @param[in]	unit		The time unit desired, see
 * 				@ref timescale_units
 *
 * @returns	The time period represented by the given number of samples
 * 		in the given time unit.
 *
 * @retval	INFINITY	Infinite number of samples is represented.
 */
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

/*! @} */

#endif
