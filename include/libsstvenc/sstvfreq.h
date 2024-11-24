#ifndef _SSTVENC_SSTVFREQ_H
#define _SSTVENC_SSTVFREQ_H

/*!
 * @addtogroup sstv
 * @{
 * @defgroup sstvfreq SSTV frequency specifications.
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>

#define SSTVENC_FREQ_VIS_BIT1	(1100)
#define SSTVENC_FREQ_SYNC	(1200)
#define SSTVENC_FREQ_VIS_BIT0	(1300)
#define SSTVENC_FREQ_BLACK	(1500)
#define SSTVENC_FREQ_VIS_START	(1900)
#define SSTVENC_FREQ_WHITE	(2300)

#define SSTVENC_FREQ_FSKID_BIT1 (1900)
#define SSTVENC_FREQ_FSKID_BIT0 (2100)

/*!
 * Compute the frequency that corresponds to the given signal level
 * given in unsigned Q8 fixed-point.
 *
 * @param[in]	level	Signal level in Q8 fixed-point.
 *
 * @returns	Output frequency in hertz.
 */
static inline uint16_t sstvenc_level_freq(uint8_t level) {
	double flevel = level / ((float)UINT8_MAX);
	if (flevel >= 1.0) {
		return SSTVENC_FREQ_WHITE;
	} else if (flevel <= 0.0) {
		return SSTVENC_FREQ_BLACK;
	} else {
		return (uint16_t)(SSTVENC_FREQ_BLACK
				  + (flevel
				     * (double)(SSTVENC_FREQ_WHITE
						- SSTVENC_FREQ_BLACK))
				  + 0.5);
	}
}

/*!
 * @}
 * @}
 */

#endif
