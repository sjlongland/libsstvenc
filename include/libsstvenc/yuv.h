#ifndef _SSTVENC_YUV_H
#define _SSTVENC_YUV_H

/*!
 * @addtogroup sstv
 * @{
 * @defgroup sstv_yuv RGB to YUV conversions (and vice versa).
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
uint8_t sstvenc_yuv_calc_y(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Return the U (red - luminance) component of a RGB colour.
 *
 * @param[in]	r	Red component in Q8 fixed-point
 * @param[in]	g	Green component in Q8 fixed-point
 * @param[in]	b	Blue component in Q8 fixed-point
 *
 * @returns	U component in Q8 fixed-point.
 */
uint8_t sstvenc_yuv_calc_u(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Return the V (blue - luminance) component of a RGB colour.
 *
 * @param[in]	r	Red component in Q8 fixed-point
 * @param[in]	g	Green component in Q8 fixed-point
 * @param[in]	b	Blue component in Q8 fixed-point
 *
 * @returns	V component in Q8 fixed-point.
 */
uint8_t sstvenc_yuv_calc_v(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Return the red component of a YUV colour.
 *
 * @param[in]	y	Y (monochrome) component in Q8 fixed-point
 * @param[in]	u	U (Y - R) component in Q8 fixed-point
 * @param[in]	v	V (Y - B) component in Q8 fixed-point
 *
 * @returns	Red component in Q8 fixed-point.
 */
uint8_t sstvenc_rgb_calc_r(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Return the green component of a YUV colour.
 *
 * @param[in]	y	Y (monochrome) component in Q8 fixed-point
 * @param[in]	u	U (Y - R) component in Q8 fixed-point
 * @param[in]	v	V (Y - B) component in Q8 fixed-point
 *
 * @returns	Green component in Q8 fixed-point.
 */
uint8_t sstvenc_rgb_calc_g(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Return the blue component of a YUV colour.
 *
 * @param[in]	y	Y (monochrome) component in Q8 fixed-point
 * @param[in]	u	U (Y - R) component in Q8 fixed-point
 * @param[in]	v	V (Y - B) component in Q8 fixed-point
 *
 * @returns	Blue component in Q8 fixed-point.
 */
uint8_t sstvenc_rgb_calc_b(uint8_t r, uint8_t g, uint8_t b);

/*!
 * Convert the given RGB framebuffer to monochrome (Y component only).
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be at
 * 			least one third of the size of @a src.  This can be
 * the same location as @a src -- after conversion the occupied size will be
 * one third of the original buffer and may be `realloc()`'d to that size.
 * @param[in]	src	Source framebuffer, which is assumed to be RGB
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_rgb_to_mono(uint8_t* dest, const uint8_t* src, uint16_t width,
			    uint16_t height);

/*!
 * Convert the given RGB framebuffer to YUV.
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be the
 * 			same size as @a src. This can be the same location as
 * 			@a src.
 * @param[in]	src	Source framebuffer, which is assumed to be RGB
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_rgb_to_yuv(uint8_t* dest, const uint8_t* src, uint16_t width,
			   uint16_t height);

/*!
 * Convert the given YUV framebuffer to RGB.
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be the
 * 			same size as @a src. This can be the same location as
 * @a src.
 * @param[in]	src	Source framebuffer, which is assumed to be YUV
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_yuv_to_rgb(uint8_t* dest, const uint8_t* src, uint16_t width,
			   uint16_t height);

/*!
 * Convert the given YUV framebuffer to monochrome (Y component only).
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be at
 * 			least one third of the size of @a src.  This can be
 * the same location as @a src -- after conversion the occupied size will be
 * one third of the original buffer and may be `realloc()`'d to that size.
 * @param[in]	src	Source framebuffer, which is assumed to be YUV
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_yuv_to_mono(uint8_t* dest, const uint8_t* src, uint16_t width,
			    uint16_t height);

/*!
 * Convert the given mono framebuffer to RGB.
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be triple
 * 			the size of @a src.  This can be the same location as
 * 			@a src -- so long as the buffer is sufficiently large.
 * @param[in]	src	Source framebuffer, which is assumed to be mono
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_mono_to_rgb(uint8_t* dest, const uint8_t* src, uint16_t width,
			    uint16_t height);

/*!
 * Convert the given mono framebuffer to YUV.
 *
 * @param[out]	dest	Destination framebuffer, which is assumed to be triple
 * 			the size as @a src. This can be the same location as
 * @a src if it is big enough.
 * @param[in]	src	Source framebuffer, which is assumed to be RGB
 * @param[in]	width	Width of the framebuffer in pixels
 * @param[in]	height	Height of the framebuffer in pixels
 */
void	sstvenc_mono_to_yuv(uint8_t* dest, const uint8_t* src, uint16_t width,
			    uint16_t height);

/*!
 * @}
 * @}
 */

#endif
