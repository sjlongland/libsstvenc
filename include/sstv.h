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

#define SSTVENC_PERIOD_VIS_START       (300000u)
#define SSTVENC_PERIOD_VIS_SYNC	       (10000u)
#define SSTVENC_PERIOD_VIS_BIT	       (30000u)

#define SSTVENC_PERIOD_FSKID_BIT       (22000u)

/*!
 * @defgroup sstv_colour_space_order Colour Space/Order bitmap
 * @addtogroup sstv_colour_space_order
 * @{
 * Colour space and order definitions.  A lot of SSTV modes may be
 * characterised by what colour space they use (monochrome, RGB or YUV)
 * and what order the channels are defined in.
 *
 * For flexibility, a bitmap has been defined using a `uint16_t`.
 */
#define SSTVENC_CSO_BIT_MODE	       (12)
#define SSTVENC_CSO_BIT_C(n)	       ((n) * 3)

/*! Bit mask for the colour space mode bits */
#define SSTVENC_CSO_MASK_MODE	       (0170000)

/*! Bit mask for the Nth colour channel */
#define SSTVENC_CSO_MASK_C(n)	       (07 << ((n) * 3))

/*! SSTV mode is monochrome (1 channel, no colour) */
#define SSTVENC_CSO_MODE_MONO	       (0 << SSTVENC_CSO_BIT_MODE)

/*! SSTV mode is colour using the RGB colourspace */
#define SSTVENC_CSO_MODE_RGB	       (1 << SSTVENC_CSO_BIT_MODE)

/*! SSTV mode is colour using the YUV (aka YCrCb) colourspace */
#define SSTVENC_CSO_MODE_YUV	       (2 << SSTVENC_CSO_BIT_MODE)

/*!
 * YUV colour space averaged over two rows.  Used in PD modes:
 *
 * - Y is the lumance on the even row
 * - U and V are averaged over both rows
 * - Y2 is the luminance of the odd row.
 */
#define SSTVENC_CSO_MODE_YUV2	       (3 << SSTVENC_CSO_BIT_MODE)

#define SSTVENC_CSO_CH_NONE	       (0) /*!< Channel not used */
#define SSTVENC_CSO_CH_Y	       (1) /*!< Y channel (luminance) */
#define SSTVENC_CSO_CH_U	       (2) /*!< U or Cr channel (Y - red) */
#define SSTVENC_CSO_CH_V	       (3) /*!< V or Cb channel (Y - blue) */
#define SSTVENC_CSO_CH_R	       (4) /*!< R channel (red) */
#define SSTVENC_CSO_CH_G	       (5) /*!< G channel (green) */
#define SSTVENC_CSO_CH_B	       (6) /*!< B channel (blue) */
#define SSTVENC_CSO_CH_Y2	       (7) /*!< Y channel of next row */

#define SSTVENC_MODE_GET_CH(n, mode)                                         \
	(((mode) & SSTVENC_CSO_MASK_C(n)) >> SSTVENC_CSO_BIT_C(n))

#define SSTVENC_MODE_ORDER(cs, c0, c1, c2, c3)                               \
	(((cs) & SSTVENC_CSO_MASK_MODE)                                      \
	 | (((c0) << SSTVENC_CSO_BIT_C(0)) & SSTVENC_CSO_MASK_C(0))          \
	 | (((c1) << SSTVENC_CSO_BIT_C(1)) & SSTVENC_CSO_MASK_C(1))          \
	 | (((c2) << SSTVENC_CSO_BIT_C(2)) & SSTVENC_CSO_MASK_C(2))          \
	 | (((c3) << SSTVENC_CSO_BIT_C(3)) & SSTVENC_CSO_MASK_C(3)))

/*!
 * @}
 */

/*!
 * Convenience structure, encodes the duration and frequency of a pulse.
 */
struct sstvenc_encoder_pulse {
	uint32_t frequency;
	uint32_t duration_ns;
};

/*!
 * Description of a SSTV mode.  This encodes all of the specifications of a
 * given mode.
 */
struct sstvenc_mode {
	/*! Human-readable description of a mode, e.g. Martin M1 */
	const char*			    description;
	/*! Short-hand name of a SSTV mode, e.g. M1 */
	const char*			    name;
	/*!
	 * Initial sequence pulses prior to first scan line.
	 */
	const struct sstvenc_encoder_pulse* initseq;
	/*!
	 * Front-porch sync pulses that happen before channel 0 is sent.  An
	 * array terminated with a 0Hz 0sec "pulse".  May be NULL if there are
	 * no start-of-scan pulses before channel 0.
	 */
	const struct sstvenc_encoder_pulse* frontporch;
	/*!
	 * Sync pulses between channel 0 and channel 1.  May be NULL if
	 * there are no pulses between channels 0 and 1.
	 */
	const struct sstvenc_encoder_pulse* gap01;
	/*!
	 * Sync pulses between channel 1 and channel 2.  May be NULL if
	 * there are no pulses between channels 1 and 2.
	 */
	const struct sstvenc_encoder_pulse* gap12;
	/*!
	 * Sync pulses between channel 2 and channel 3.  May be NULL if
	 * there are no pulses between channels 2 and 3.
	 */
	const struct sstvenc_encoder_pulse* gap23;
	/*!
	 * Back-porch sync pulses after channel 2 (or after channel 0 for
	 * mono SSTV modes).
	 */
	const struct sstvenc_encoder_pulse* backporch;
	/*!
	 * Final sequence pulses following last scan line.
	 */
	const struct sstvenc_encoder_pulse* finalseq;
	/*!
	 * Scanline periods for each of the three channels.  For mono modes,
	 * only the first is used.
	 */
	uint32_t			    scanline_period_ns[4];

	/*!
	 * Width of the SSTV image sent in pixels.
	 */
	uint16_t			    width;
	/*!
	 * Height of the SSTV image sent in pixels.
	 */
	uint16_t			    height;
	/*!
	 * The colour space mode and order used.  This is a bitmap, see
	 * @ref SSTVENC_MODE_ORDER
	 */
	uint16_t			    colour_space_order;
	/*!
	 * The VIS code sent at the start of the SSTV transmission.
	 */
	uint8_t				    vis_code;
};

/*!
 * Return the number of SSTV modes defined.
 */
uint8_t			   sstvenc_get_mode_count();

/*!
 * Return the Nth SSTV mode.  Returns NULL if we're off the end of the array.
 */
const struct sstvenc_mode* sstvenc_get_mode_by_idx(uint8_t idx);

/*!
 * Return the SSTV mode whose name matches this.  Returns NULL if no such
 * matching mode is found.
 */
const struct sstvenc_mode* sstvenc_get_mode_by_name(const char* name);

/*!
 * Compute the transmission time of a given pulse sequence.
 */
static inline uint64_t
sstvenc_pulseseq_get_txtime(const struct sstvenc_encoder_pulse* seq) {
	uint64_t txtime = 0;

	while ((seq != NULL) && (seq->duration_ns > 0)) {
		txtime += seq->duration_ns;
		seq++;
	}

	return txtime;
}

/*!
 * Compute the transmission time of the specified mode in nanoseconds.
 */
static inline uint64_t
sstvenc_mode_get_txtime(const struct sstvenc_mode* const mode,
			const char*			 fsk_id) {
	uint64_t txtime	 = 0;

	/* Compute each scan line */
	txtime		+= sstvenc_pulseseq_get_txtime(mode->frontporch);

	for (uint8_t ch = 0; ch < 4; ch++) {
		if (SSTVENC_MODE_GET_CH(ch, mode->colour_space_order)
		    != SSTVENC_CSO_CH_NONE) {
			/* There's a scan line here */
			txtime += mode->scanline_period_ns[ch];

			switch (ch) {
			case 0:
				txtime += sstvenc_pulseseq_get_txtime(
				    mode->gap01);
				break;
			case 1:
				txtime += sstvenc_pulseseq_get_txtime(
				    mode->gap12);
				break;
			case 2:
				txtime += sstvenc_pulseseq_get_txtime(
				    mode->gap23);
				break;
			default:
				break;
			}
		}
	}

	txtime += sstvenc_pulseseq_get_txtime(mode->backporch);

	/* This is repeated for each scan line */
	switch (mode->colour_space_order & SSTVENC_CSO_MASK_MODE) {
	case SSTVENC_CSO_MODE_YUV2:
		/* Each scan line is two image lines */
		txtime *= mode->height / 2;
		break;
	default:
		/* Each scan line is an image line */
		txtime *= mode->height;
		break;
	}

	/*
	 * VIS header:
	 */
	txtime += 1000
		  * (SSTVENC_PERIOD_VIS_START	    /* START bit 1 */
		     + SSTVENC_PERIOD_VIS_SYNC	    /* START bit 2 */
		     + SSTVENC_PERIOD_VIS_START	    /* START bit 3 */
		     + SSTVENC_PERIOD_VIS_BIT	    /* START bit 4 */
		     + (7 * SSTVENC_PERIOD_VIS_BIT) /* data bits */
		     + SSTVENC_PERIOD_VIS_BIT	    /* PARITY bit */
		     + SSTVENC_PERIOD_VIS_BIT	    /* STOP bit */
		  );

	/* Before and after, we have sequences of pulses */
	txtime += sstvenc_pulseseq_get_txtime(mode->initseq);
	txtime += sstvenc_pulseseq_get_txtime(mode->finalseq);

	if (fsk_id) {
		/* Add the duration of the FSK ID */
		txtime += 1000
			  * ((SSTVENC_PERIOD_FSKID_BIT
			      * 12) /* 12 bits preamble */
			     + (SSTVENC_PERIOD_FSKID_BIT * strlen(fsk_id)
				* 6) /* 6 bits/char ID */
			     + (SSTVENC_PERIOD_FSKID_BIT
				* 6) /* 6 bits trailer */
			  );
	}

	return txtime;
}

/*!
 * Return the size of the framebuffer needed for this mode in bytes.
 */
static inline size_t
sstvenc_mode_get_fb_sz(const struct sstvenc_mode* const mode) {
	size_t sz = mode->width * mode->height * sizeof(uint8_t);

	if ((mode->colour_space_order & SSTVENC_CSO_MASK_MODE)
	    != SSTVENC_CSO_MODE_MONO) {
		/* 3 channels needed */
		sz *= 3;
	}

	return sz;
}

/*!
 * Fetch the offset into the framebuffer for the given pixel.
 */
static inline uint32_t
sstvenc_get_pixel_posn(const struct sstvenc_mode* const mode, uint16_t x,
		       uint16_t y) {
	assert(x < mode->width);
	assert(y < mode->height);

	uint32_t idx  = y * mode->width;
	idx	     += x;

	if ((mode->colour_space_order & SSTVENC_CSO_MASK_MODE)
	    != SSTVENC_CSO_MODE_MONO) {
		/* These are 3-colour tuples */
		idx *= 3;
	}

	return idx;
}

/*
 * Preamble sequence step types.
 */
#define SSTVENC_PREAMBLE_TYPE_END  (0) /*!< End of preamble */
#define SSTVENC_PREAMBLE_TYPE_TONE (1) /*!< Emit a tone or silence */
#define SSTVENC_PREAMBLE_TYPE_CW   (2) /*!< Emit a CW message */

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
