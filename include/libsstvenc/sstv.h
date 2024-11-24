#ifndef _SSTVENC_SSTV_H
#define _SSTVENC_SSTV_H

/*!
 * @defgroup sstv SSTV encoder
 * @{
 *
 * This is an asynchronous SSTV encoder that emits frequency/duration pairs
 * to be used with an oscillator (see @ref oscillator) to produce actual audio
 * samples.
 *
 * Depending on the SSTV mode it will expect a framebuffer with one of the
 * following forms:
 *
 * - for modes with colour-space SSTVENC_CSO_MODE_MONO: an array of `uint8_t`s
 *   measuring sstvenc_mode#width times sstvenc_mode#height.  Each element
 *   represents the pixel's monochromatic brightness level.
 *
 * - for modes with colour-space SSTVENC_CSO_MODE_RGB: an array of `uint8_t`s
 *   measuring sstvenc_mode#width times sstvenc_mode#height times 3.
 *   Every 3 elements represents a pixel's RGB colour in the order: red,
 * green, blue.
 *
 * - for modes with colour-spaces SSTVENC_CSO_MODE_YUV or
 *   SSTVENC_CSO_MODE_YUV2: an array of `uint8_t`s measuring
 * sstvenc_mode#width times sstvenc_mode#height times 3.  Every 3 elements
 * represents a pixel's YUV colour in the order: Y (luminance), U (R-Y), V
 * (B-Y).
 *
 * The routines in @ref yuv may be useful for converting between RGB and YUV
 * or monochrome modes in your application.
 *
 * Calling code initialises a context by calling @ref sstvenc_encoder_init
 * then repeatedly calling @ref sstvenc_encoder_next_pulse to compute each
 * SSTV image pulse.  Calling code may conclude the state machine is finished
 * when sstvenc_encoder#phase reaches SSTVENC_ENCODER_PHASE_DONE or when
 * @ref sstvenc_encoder_next_pulse returns NULL.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <libsstvenc/sstvmode.h>
#include <stdint.h>
#include <string.h>

/*!
 * @defgroup sstv_phase SSTV Encoder Phases
 * @{
 * These values are used in sstv_encoder#phase
 */

/*! Initialisation phase.  No samples emitted yet */
#define SSTVENC_ENCODER_PHASE_INIT     (0)

/*! Transmission of the VIS header */
#define SSTVENC_ENCODER_PHASE_VIS      (1)

/*! Transmission of the mode initial pulse sequence */
#define SSTVENC_ENCODER_PHASE_INITSEQ  (2)

/*!
 * Transmission of scan lines.  The progress can be tracked by
 * observing the co-ordinates at
 * sstvenc_encoder_phase_scan_data#x
 * and sstvenc_encoder_phase_scan_data#y.
 */
#define SSTVENC_ENCODER_PHASE_SCAN     (3)

/*! Transmission of the mode final pulse sequence */
#define SSTVENC_ENCODER_PHASE_FINALSEQ (4)

/*! Transmission of the FSK ID */
#define SSTVENC_ENCODER_PHASE_FSK      (5)

/*! End of SSTV transmission */
#define SSTVENC_ENCODER_PHASE_DONE     (6)

/*!
 * @}
 */

/* Forward declaration */
struct sstvenc_encoder;

/*!
 * Callback routine for SSTV encoder events.
 */
typedef void sstvenc_encoder_callback(struct sstvenc_encoder* const enc);

/*!
 * SSTV encoder data structure.  This encodes the state of the encoder and
 * all the necessary oscillator and pulse shaper structures.
 */
struct sstvenc_encoder {
	/*! The SSTV mode being used for encoding. */
	const struct sstvenc_mode*	    mode;

	/*! The FSK-ID to be appended to the end.  NULL for no FSK ID */
	const char*			    fsk_id;

	/*!
	 * The framebuffer holding the image to be sent.  The image is assumed
	 * to be in the correct format for the SSTV mode, i.e. correct
	 * dimensions, channel count and colour space.
	 *
	 * For colour SSTV modes, the values are interleaved for each channel,
	 * so c0, c1, c2; for the first pixel, then c0, c1, c2 for the second,
	 * etc.  Use @ref SSTVENC_MODE_GET_CH to determine what channel is
	 * what.
	 */
	const uint8_t*			    framebuffer;

	/*! The current pulse sequence being emitted */
	const struct sstvenc_encoder_pulse* seq;

	/*! What to do if the sequence is done? */
	sstvenc_encoder_callback*	    seq_done_cb;

	/*! The current pulse being emitted */
	struct sstvenc_encoder_pulse	    pulse;

	union sstvenc_encoder_phase_data {
		struct sstvenc_encoder_phase_vis_data {
			/*! The current bit being sent */
			uint8_t bit;
		} vis;

		struct sstvenc_encoder_phase_scan_data {
			/*!
			 * The current image X position being scanned
			 */
			uint16_t x;

			/*!
			 * The current image Y position being scanned
			 */
			uint16_t y;

			/*!
			 * Scanline segment being emitted
			 */
			uint8_t	 segment;
		} scan;

		struct sstvenc_encoder_phase_fsk_data {
			/*!
			 * The current byte segment being sent
			 */
			uint8_t segment;

			/*!
			 * The current segment length
			 */
			uint8_t seg_sz;

			/*!
			 * The current byte position being sent
			 */
			uint8_t byte;

			/*!
			 * The current byte value being sent
			 */
			uint8_t bv;

			/*!
			 * The current bit of the byte being sent
			 */
			uint8_t bit;
		} fsk;
	} vars;

	/*!
	 * The transmission phase we are in.
	 */
	uint8_t phase;
};

/*!
 * Initialise the SSTV encoder with the given parameters.
 *
 * @param[inout]	enc		SSTV encoder context to initialise
 * @param[in]		mode		SSTV mode to encode
 * @param[in]		fsk_id		FSK ID to send at the end, NULL to
 * 					disable.
 * @param[in]		framebuffer	Framebuffer data representing the
 * 					image.
 */
void sstvenc_encoder_init(struct sstvenc_encoder* const enc,
			  const struct sstvenc_mode* mode, const char* fsk_id,
			  const uint8_t* framebuffer);

/*!
 * Compute the next pulse to be emitted.  This value returns NULL when there
 * are no more pulses to transmit.
 */
const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_pulse(struct sstvenc_encoder* const enc);

#endif
