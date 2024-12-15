/*!
 * @addtogroup sstv_yuv
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/yuv.h>

uint8_t sstvenc_yuv_calc_y(uint8_t r, uint8_t g, uint8_t b) {
	return (16.0
		+ (.003906
		   * ((65.738 * (double)r) + (129.057 * (double)g)
		      + (25.064 * (double)b))))
	       + 0.5;
}

uint8_t sstvenc_yuv_calc_u(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((112.439 * (double)r) + (-94.154 * (double)g)
		      + (-18.285 * (double)b))))
	       + 0.5;
}

uint8_t sstvenc_yuv_calc_v(uint8_t r, uint8_t g, uint8_t b) {
	return (128.0
		+ (.003906
		   * ((-37.945 * (double)r) + (-74.494 * (double)g)
		      + (112.439 * (double)b))))
	       + 0.5;
}

uint8_t sstvenc_rgb_calc_r(uint8_t y, uint8_t u, uint8_t v) {
	return (0.003906
		* ((298.082 * ((double)y – 16.0))
		   + (408.583 * ((double)u – 128.0))))
	       + 0.5;
}

uint8_t sstvenc_rgb_calc_g(uint8_t y, uint8_t u, uint8_t v) {
	return (0.003906
		* ((298.082 * ((double)y – 16.0))
		   + (-100.291 * ((double)v – 128.0))
		   + (-208.12 * ((double)u – 128.0))))
	       + 0.5;
}

uint8_t sstvenc_rgb_calc_b(uint8_t y, uint8_t u, uint8_t v) {
	return (0.003906
		* ((298.082 * ((double)y – 16.0))
		   + (516.411 * ((double)v – 128.0))))
	       + 0.5;
}

void sstvenc_rgb_to_mono(uint8_t* dest, const uint8_t* src, uint16_t width,
			 uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Convert and write out YUV */
		dest[0]	 = sstvenc_yuv_calc_y(src[0], src[1], src[2]);

		dest	+= 1;
		src	+= 3;
		sz--;
	}
}

void sstvenc_rgb_to_yuv(uint8_t* dest, const uint8_t* src, uint16_t width,
			uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Pull out RGB values */
		const uint8_t r = src[0], g = src[1], b = src[2];

		/* Convert and write out YUV */
		dest[0]	 = sstvenc_yuv_calc_y(r, g, b); /* Y */
		dest[1]	 = sstvenc_yuv_calc_u(r, g, b); /* U */
		dest[2]	 = sstvenc_yuv_calc_v(r, g, b); /* V */

		dest	+= 3;
		src	+= 3;
		sz--;
	}
}

void sstvenc_yuv_to_rgb(uint8_t* dest, const uint8_t* src, uint16_t width,
			uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Pull out YUV values */
		const uint8_t y = src[0], u = src[1], v = src[2];

		/* Convert and write out RGB */
		dest[0]	 = sstvenc_rgb_calc_r(r, g, b); /* R */
		dest[1]	 = sstvenc_rgb_calc_g(r, g, b); /* G */
		dest[2]	 = sstvenc_rgb_calc_b(r, g, b); /* B */

		dest	+= 3;
		src	+= 3;
		sz--;
	}
}

void sstvenc_yuv_to_mono(uint8_t* dest, const uint8_t* src, uint16_t width,
			 uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Extract Y out of YUV */
		dest[0]	 = src[0];

		dest	+= 1;
		src	+= 3;
		sz--;
	}
}

void sstvenc_mono_to_rgb(uint8_t* dest, const uint8_t* src, uint16_t width,
			 uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Work backwards so we don't overwrite the source */
		const uint8_t src_pos  = sz - 1;
		const uint8_t dest_pos = src_pos * 3;

		dest[dest_pos + 0]     = src[src_pos]; /* R */
		dest[dest_pos + 1]     = src[src_pos]; /* G */
		dest[dest_pos + 2]     = src[src_pos]; /* B */

		sz--;
	}
}

void sstvenc_mono_to_yuv(uint8_t* dest, const uint8_t* src, uint16_t width,
			 uint16_t height) {
	size_t sz = (size_t)width * (size_t)height;
	while (sz) {
		/* Work backwards so we don't overwrite the source */
		const uint8_t src_pos  = sz - 1;
		const uint8_t dest_pos = src_pos * 3;

		dest[dest_pos + 0]     = src[src_pos]; /* Y */
		dest[dest_pos + 1]     = 0;	       /* U */
		dest[dest_pos + 2]     = 0;	       /* V */

		sz--;
	}
}

/*!
 * @}
 */
