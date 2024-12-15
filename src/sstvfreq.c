/*!
 * @addtogroup sstvfreq
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/sstvfreq.h>

uint16_t sstvenc_level_freq(uint8_t level) {
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
 */
