#ifndef _SSTVENC_SSTV_H
#define _SSTVENC_SSTV_H

/*!
 * SSTV encoder class.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include "cw.h"
#include "sstvmode.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define SSTVENC_ENCODER_PHASE_INIT     (0)
#define SSTVENC_ENCODER_PHASE_PREAMBLE (1)
#define SSTVENC_ENCODER_PHASE_VIS      (2)
#define SSTVENC_ENCODER_PHASE_INITSEQ  (3)
#define SSTVENC_ENCODER_PHASE_SCAN     (4)
#define SSTVENC_ENCODER_PHASE_FINALSEQ (5)
#define SSTVENC_ENCODER_PHASE_FSK      (6)
#define SSTVENC_ENCODER_PHASE_DONE     (7)

/*
 * Preamble sequence step types.
 */
#define SSTVENC_PREAMBLE_TYPE_END      (0) /*!< End of preamble */
#define SSTVENC_PREAMBLE_TYPE_TONE     (1) /*!< Emit a tone or silence */
#define SSTVENC_PREAMBLE_TYPE_CW       (2) /*!< Emit a CW message */

struct sstvenc_preamble_step {
	/*! Amplitude of this tone or CW message.  Set to 0 for silence. */
	double amplitude;

	/*! Frequency of the tone or CW message. */
	double frequency;

	/*! Additional settings based on the type */
	union sstvenc_preamble_step_data {
		/*! Settings for `type == SSTVENC_PREAMBLE_TYPE_TONE` */
		struct sstvenc_preamble_tone_step_data {
			/*! Duration of the tone in seconds */
			double duration;
		} tone;

		/*! Settings for `type == SSTVENC_PREAMBLE_TYPE_CW` */
		struct sstvenc_preamble_cw_step_data {
			/*! Duration of a dit in seconds */
			double	    dit_period;
			/*! The text to be transmitted */
			const char* text;
			/*! Time unit */
			uint8_t	    time_unit;
		} cw;
	} data;

	/* Preamble step type */
	uint8_t type;
};

#define SSTVENC_PREAMBLE_STEP_TONE(amplitude, frequency, duration)           \
	{ \
	.type = SSTVENC_PREAMBLE_TYPE_TONE, \
	.amplitude = (amplitude), \
	.frequency = (frequency), \
	.data = { \
		.tone = { \
			.duration = (duration),\
		}, \
	}, \
}

#define SSTVENC_PREAMBLE_STEP_CW(amplitude, frequency, dit_period,           \
				 time_unit, text)                            \
	{ \
	.type = SSTVENC_PREAMBLE_TYPE_CW, \
	.amplitude = (amplitude), \
	.frequency = (frequency), \
	.data = { \
		.cw = { \
			.dit_period = (dit_period), \
			.text = (text), \
			.time_unit = (time_unit),\
		}, \
	}, \
}

#define SSTVENC_PREAMBLE_STEP_END                                            \
	{                                                                    \
	    .type      = SSTVENC_PREAMBLE_TYPE_END,                          \
	    .amplitude = (0.0),                                              \
	    .frequency = (0.0),                                              \
	}

/*!
 * SSTV encoder data structure.  This encodes the state of the encoder and
 * all the necessary oscillator and pulse shaper structures.
 */
struct sstvenc_encoder {
	/*!
	 * CW modulator, we also use its oscillator and pulse shaper
	 * for the SSTV transmission.
	 */
	struct sstvenc_cw_mod		    cw;

	/*! The SSTV mode being used for encoding. */
	const struct sstvenc_mode*	    mode;

	/*! The preamble steps, ended with @ref SSTVENC_PREAMBLE_STEP_END */
	const struct sstvenc_preamble_step* preamble;

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

	/*! Output sample */
	double				    output;

	/*! Amplitude of the SSTV transmission */
	double				    amplitude;

	/*! Pulse shaping filter slope transition period in seconds. */
	double				    slope_period;

	/*! Sample rate in Hz */
	uint32_t			    sample_rate;

	/*!
	 * Sample index for drift tracking purposes.  Incremented each time
	 * output is updated.
	 */
	uint32_t			    sample_idx;

	/*! Samples remaining for current tone */
	uint16_t			    sample_rem;

	union sstvenc_encoder_phase_data {
		struct sstvenc_encoder_phase_preamble_data {
			/*! The current step being performed */
			uint8_t step;
		} preamble;

		struct sstvenc_encoder_phase_vis_data {
			/*! The current bit being sent */
			uint8_t bit;
		} vis;

		struct sstvenc_encoder_phase_initseq_data {
			/*!
			 * Initial tone sequence index
			 */
			uint8_t idx;
		} initseq;

		struct sstvenc_encoder_phase_scan_data {
			/*!
			 * The current image Y position being scanned
			 */
			uint16_t y;

			/*!
			 * Scanline segment being emitted
			 */
			uint8_t	 segment;

			/*!
			 * Scanline tone sequence index
			 */
			uint8_t	 idx;
		} scan;

		struct sstvenc_encoder_phase_finalseq_data {
			/*!
			 * Final tone sequence index
			 */
			uint8_t idx;
		} finalseq;

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
	 * Tone generator state.
	 */
	uint8_t tone_state;

	/*!
	 * The transmission phase we are in.
	 */
	uint8_t phase;
};

/*!
 * Initialise the SSTV encoder with the given parameters.
 */
void sstvenc_encoder_init(struct sstvenc_encoder* const	      enc,
			  const struct sstvenc_mode*	      mode,
			  const struct sstvenc_preamble_step* preamble,
			  const char* fsk_id, const uint8_t* framebuffer,
			  double amplitude, double slope_period,
			  uint32_t sample_rate);

/*!
 * Compute the next audio sample from the SSTV encoder.
 */
void sstvenc_encoder_compute(struct sstvenc_encoder* const enc);

#endif
