#ifndef _SSTVENC_YUV_H
#define _SSTVENC_YUV_H

/*!
 * @addtogroup sstv
 * @{
 * @defgroup sstv_yuv RGB to YUV conversions.
 * @{
 *
 * Functions for extracting the Y, U or V components of a RGB colour.
 * The equations in this module came from JL Barber (N7CXI)'s presentation
 * at the Dayton SSSTV forum, 2000-05-20.
 *
 * http://www.barberdsp.com/downloads/Dayton%20Paper.pdf
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>

/*!
 * Return the Y (luminance) component of a RGB colour.  This routine is
 * useful for converting colour to monochrome as well as RGB to YUV.
 *
 * @param[in]	r	Red component in Q8 fixed-point
 * @param[in]	g	Green component in Q8 fixed-point
 * @param[in]	b	Blue component in Q8 fixed-point
 *
 * @returns	Y component in Q8 fixed-point.
 */
static inline uint8_t sstvenc_yuv_calc_y(uint8_t r, uint8_t g, uint8_t b) {
	return (16.0
		+ (.003906
		   * ((65.738 * (double)r) + (129.057 * (double)g)
		      + (25.064 * (double)b))))
	       + 0.5;
}

/*!
 * Return the U (red - luminance) component of a RGB colour.
 *
 * @param[in]	r	Red component in Q8 fixed-point
 * @param[in]	g	Green component in Q8 fixed-point
 * @param[in]	b	Blue component in Q8 fixed-point
 *
 * @returns	U component in Q8 fixed-point.
 */
static inline uint8_t sstvenc_yuv_calc_u(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((112.439 * (double)r) + (-94.154 * (double)g)
		      + (-18.285 * (double)b))))
	       + 0.5;
}

/*!
 * Return the V (blue - luminance) component of a RGB colour.
 *
 * @param[in]	r	Red component in Q8 fixed-point
 * @param[in]	g	Green component in Q8 fixed-point
 * @param[in]	b	Blue component in Q8 fixed-point
 *
 * @returns	V component in Q8 fixed-point.
 */
static inline uint8_t sstvenc_yuv_calc_v(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((-37.945 * (double)r) + (-74.494 * (double)g)
		      + (112.439 * (double)b))))
	       + 0.5;
}

/*!
 * @}
 * @}
 */

#endif
