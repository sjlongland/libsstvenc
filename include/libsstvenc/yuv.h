#ifndef _SSTVENC_YUV_H
#define _SSTVENC_YUV_H

/*!
 * RGB to YUV conversions.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>

static inline uint8_t sstvenc_yuv_calc_y(uint8_t r, uint8_t g, uint8_t b) {
	return (16.0
		+ (.003906
		   * ((65.738 * (double)r) + (129.057 * (double)g)
		      + (25.064 * (double)b))))
	       + 0.5;
}

static inline uint8_t sstvenc_yuv_calc_u(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((112.439 * (double)r) + (-94.154 * (double)g)
		      + (-18.285 * (double)b))))
	       + 0.5;
}

static inline uint8_t sstvenc_yuv_calc_v(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((-37.945 * (double)r) + (-74.494 * (double)g)
		      + (112.439 * (double)b))))
	       + 0.5;
}

#endif
