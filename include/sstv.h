#ifndef _SSTVENC_SSTV_H
#define _SSTVENC_SSTV_H

/*!
 * SSTV encoder class.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include "sstvmode.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define SSTVENC_ENCODER_PHASE_INIT     (0)
#define SSTVENC_ENCODER_PHASE_VIS      (1)
#define SSTVENC_ENCODER_PHASE_INITSEQ  (2)
#define SSTVENC_ENCODER_PHASE_SCAN     (3)
#define SSTVENC_ENCODER_PHASE_FINALSEQ (4)
#define SSTVENC_ENCODER_PHASE_FSK      (5)
#define SSTVENC_ENCODER_PHASE_DONE     (6)

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
 */
void sstvenc_encoder_init(struct sstvenc_encoder* const enc,
			  const struct sstvenc_mode* mode, const char* fsk_id,
			  const uint8_t* framebuffer);

/*!
 * Compute the next pulse to be emitted.
 */
const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_pulse(struct sstvenc_encoder* const enc);

#endif
