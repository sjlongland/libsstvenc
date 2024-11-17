/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include "sstv.h"
#include <assert.h>
#include <string.h>

#define SSTVENC_FREQ_VIS_BIT1	 (1100.0)
#define SSTVENC_FREQ_SYNC	 (1200.0)
#define SSTVENC_FREQ_VIS_BIT0	 (1300.0)
#define SSTVENC_FREQ_BLACK	 (1500.0)
#define SSTVENC_FREQ_VIS_START	 (1900.0)
#define SSTVENC_FREQ_WHITE	 (2300.0)
#define SSTVENC_FREQ_FSKID_BIT1	 (1900.0)
#define SSTVENC_FREQ_FSKID_BIT0	 (2100.0)

#define SSTVENC_PERIOD_VIS_START (300000u)
#define SSTVENC_PERIOD_VIS_SYNC	 (10000u)
#define SSTVENC_PERIOD_VIS_BIT	 (30000u)
#define SSTVENC_PERIOD_FSKID_BIT (22000u)

#define SSTVENC_VIS_BIT_START1	 (0)
#define SSTVENC_VIS_BIT_START2	 (1)
#define SSTVENC_VIS_BIT_START3	 (2)
#define SSTVENC_VIS_BIT_START4	 (3)
#define SSTVENC_VIS_BIT_DATA1	 (4)
#define SSTVENC_VIS_BIT_DATA2	 (5)
#define SSTVENC_VIS_BIT_DATA3	 (6)
#define SSTVENC_VIS_BIT_DATA4	 (7)
#define SSTVENC_VIS_BIT_DATA5	 (8)
#define SSTVENC_VIS_BIT_DATA6	 (9)
#define SSTVENC_VIS_BIT_DATA7	 (10)
#define SSTVENC_VIS_BIT_PARITY	 (11)
#define SSTVENC_VIS_BIT_STOP	 (12)
#define SSTVENC_VIS_BIT_END	 (13)

/* SSTV mode specifications -- Robot B/W modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_robotbw_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 7000000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Martin modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_martin_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 4862000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 572000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_martin_sep[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 572000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Wraase SC-2 modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_180_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 5522500},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_120_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 5522500},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1000000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_sep[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 500000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- the table */
const static struct sstvenc_mode sstvenc_sstv_modes[] = {
    /* Robot B/W modes */
    {
	.description	    = "Robot 8 B/W",
	.name		    = "R8BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {59900000, 0, 0},
	.width		    = 160,
	.height		    = 120,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y,
			     SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x02,
    },
    {
	.description	    = "Robot 12 B/W",
	.name		    = "R12BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {93000000, 0, 0},
	.width		    = 160,
	.height		    = 120,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y,
			     SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x86,
    },
    {
	.description	    = "Robot 24 B/W",
	.name		    = "R24BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {93000000, 0, 0},
	.width		    = 320,
	.height		    = 240,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y,
			     SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x0a,
    },
    /* Martin modes */
    {
	.description	    = "Martin M1",
	.name		    = "M1",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_martin_fp,
	.gap01		    = sstvenc_sstv_martin_sep,
	.gap12		    = sstvenc_sstv_martin_sep,
	.backporch	    = sstvenc_sstv_martin_sep,
	.finalseq	    = NULL,
	.scanline_period_ns = {146432000, 146432000, 146432000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G,
			     SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_R),
	.vis_code = 0x2c,
    },
    {
	.description	    = "Martin M2",
	.name		    = "M2",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_martin_fp,
	.gap01		    = sstvenc_sstv_martin_sep,
	.gap12		    = sstvenc_sstv_martin_sep,
	.backporch	    = sstvenc_sstv_martin_sep,
	.finalseq	    = NULL,
	.scanline_period_ns = {73216000, 73216000, 73216000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G,
			     SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_R),
	.vis_code = 0x28,
    },
    /* Wraase SC-2 modes */
    {
	.description	    = "Wraase SC-2 120",
	.name		    = "W2120",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_wraasesc2_120_fp,
	/* Separator is needed for W2120, but not W2180 in QSSTV and slowrx */
	.gap01		    = sstvenc_sstv_wraasesc2_sep,
	.gap12		    = sstvenc_sstv_wraasesc2_sep,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {155985000, 155985000, 155985000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R,
			     SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B),
	.vis_code = 0x3f,
    },
    {
	.description	    = "Wraase SC-2 180",
	.name		    = "W2180",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_wraasesc2_180_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {235000000, 235000000, 235000000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order
	= SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R,
			     SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B),
	.vis_code = 0x37,
    },
};

uint8_t sstvenc_get_mode_count() {
	return sizeof(sstvenc_sstv_modes) / sizeof(sstvenc_sstv_modes[0]);
}

const struct sstvenc_mode* sstvenc_get_mode_by_idx(uint8_t idx) {
	if (idx < sstvenc_get_mode_count()) {
		return &sstvenc_sstv_modes[idx];
	} else {
		return NULL;
	}
}

const struct sstvenc_mode* sstvenc_get_mode_by_name(const char* name) {
	for (uint8_t idx = 0; idx < sstvenc_get_mode_count(); idx++) {
		if (!strcmp(sstvenc_sstv_modes[idx].name, name)) {
			return &sstvenc_sstv_modes[idx];
		}
	}
	return NULL;
}

static double sstvenc_level_freq(double level) {
	if (level >= 1.0) {
		return SSTVENC_FREQ_WHITE;
	} else if (level <= 0.0) {
		return SSTVENC_FREQ_BLACK;
	} else {
		return SSTVENC_FREQ_BLACK
		       + (level * (SSTVENC_FREQ_WHITE - SSTVENC_FREQ_BLACK));
	}
}

void sstvenc_encoder_init(struct sstvenc_encoder* const	      enc,
			  const struct sstvenc_mode*	      mode,
			  const struct sstvenc_preamble_step* preamble,
			  const char* fsk_id, const double* framebuffer,
			  double amplitude, double slope_period,
			  uint32_t sample_rate) {
	memset(enc, 0, sizeof(struct sstvenc_encoder));
	enc->mode	  = mode;
	enc->preamble	  = preamble;
	enc->fsk_id	  = fsk_id;
	enc->framebuffer  = framebuffer;
	enc->amplitude	  = amplitude;
	enc->slope_period = slope_period;
	enc->sample_rate  = sample_rate;
	enc->phase	  = SSTVENC_ENCODER_PHASE_INIT;
}

static void sstvenc_encoder_new_phase(struct sstvenc_encoder* const enc,
				      uint8_t			    phase) {
	enc->phase = phase;
}

static void sstvenc_encoder_start_preamble(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_preamble(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_vis(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_vis(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_initseq(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_initseq(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_scan(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_scan(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_finalseq(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_finalseq(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_fsk(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_fsk(struct sstvenc_encoder* const enc);

static void sstvenc_encoder_preamble_next(struct sstvenc_encoder* const enc) {
	enc->vars.preamble.step++;
}

#define SSTVENC_ENCODER_TONE_GEN_INIT (0)
#define SSTVENC_ENCODER_TONE_GEN_RUN  (1)
#define SSTVENC_ENCODER_TONE_GEN_DONE (2)

static void sstvenc_encoder_start_tone(struct sstvenc_encoder* const enc,
				       double amplitude, uint32_t frequency,
				       uint32_t duration, uint8_t time_unit) {
	assert(enc->tone_state == SSTVENC_ENCODER_TONE_GEN_INIT);
	enc->cw.osc.amplitude = amplitude;
	sstvenc_osc_set_frequency(&(enc->cw.osc), frequency);
	enc->sample_rem = sstvenc_ts_unit_to_samples(
	    duration, enc->sample_rate, time_unit);
	enc->tone_state = SSTVENC_ENCODER_TONE_GEN_RUN;
}

static void sstvenc_encoder_compute_tone(struct sstvenc_encoder* const enc) {
	assert(enc->tone_state == SSTVENC_ENCODER_TONE_GEN_RUN);
	if (enc->sample_rem > 0) {
		sstvenc_osc_compute(&(enc->cw.osc));
		enc->output = enc->cw.osc.output;
		enc->sample_rem--;
	}

	if (!enc->sample_rem) {
		enc->tone_state = SSTVENC_ENCODER_TONE_GEN_DONE;
	}
}

static void sstvenc_encoder_finish_tone(struct sstvenc_encoder* const enc) {
	assert(enc->tone_state == SSTVENC_ENCODER_TONE_GEN_DONE);
	assert(enc->sample_rem == 0);
	enc->tone_state = SSTVENC_ENCODER_TONE_GEN_INIT;
}

static void
sstvenc_encoder_preamble_do_tone(struct sstvenc_encoder* const enc) {
	const struct sstvenc_preamble_step* step
	    = &(enc->preamble[enc->vars.preamble.step]);
	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(
		    enc, step->amplitude, step->frequency,
		    step->data.tone.duration, SSTVENC_TS_UNIT_SECONDS);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		sstvenc_encoder_preamble_next(enc);
		break;
	}
}

static void
sstvenc_encoder_preamble_do_cw(struct sstvenc_encoder* const enc) {
	const struct sstvenc_preamble_step* step
	    = &(enc->preamble[enc->vars.preamble.step]);
	switch (enc->cw.state) {
	case SSTVENC_CW_MOD_STATE_INIT:
		/* We're starting a CW message */
		sstvenc_cw_init(&(enc->cw), step->data.cw.text,
				step->amplitude, step->frequency,
				step->data.cw.dit_period, enc->slope_period,
				enc->sample_rate, step->data.cw.time_unit);
		/* Fall-thru */
	case SSTVENC_CW_MOD_STATE_NEXT_SYM:
	case SSTVENC_CW_MOD_STATE_MARK:
	case SSTVENC_CW_MOD_STATE_DITSPACE:
	case SSTVENC_CW_MOD_STATE_DAHSPACE:
		sstvenc_cw_compute(&(enc->cw));
		enc->output = enc->cw.output;
		break;
	case SSTVENC_CW_MOD_STATE_DONE:
		sstvenc_ps_reset(&(enc->cw.ps), INFINITY,
				 SSTVENC_TS_UNIT_SECONDS);
		sstvenc_encoder_preamble_next(enc);
		break;
	}
}

static void sstvenc_encoder_do_preamble(struct sstvenc_encoder* const enc) {
	switch (enc->preamble[enc->vars.preamble.step].type) {
	case SSTVENC_PREAMBLE_TYPE_END:
		sstvenc_encoder_start_vis(enc);
		break;
	case SSTVENC_PREAMBLE_TYPE_TONE:
		sstvenc_encoder_preamble_do_tone(enc);
		break;
	case SSTVENC_PREAMBLE_TYPE_CW:
		sstvenc_encoder_preamble_do_cw(enc);
		break;
	default:
		sstvenc_encoder_preamble_next(enc);
		break;
	}
}

static void
sstvenc_encoder_start_preamble(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_PREAMBLE);
	enc->vars.preamble.step = 0;
	sstvenc_encoder_do_preamble(enc);
}

static double
sstvenc_encoder_vis_data_freq(struct sstvenc_encoder* const enc) {
	uint8_t bit = enc->vars.vis.bit - SSTVENC_VIS_BIT_DATA1;
	if (enc->mode->vis_code & (1 << bit)) {
		return SSTVENC_FREQ_VIS_BIT1;
	} else {
		return SSTVENC_FREQ_VIS_BIT0;
	}
}

static double
sstvenc_encoder_vis_parity_freq(struct sstvenc_encoder* const enc) {
	uint8_t ones = 0;
	for (uint8_t i = 0; i < 8; i++) {
		if (enc->mode->vis_code & (1 << i)) {
			ones++;
		}
	}

	if ((ones % 2) == 1) {
		return SSTVENC_FREQ_VIS_BIT1;
	} else {
		return SSTVENC_FREQ_VIS_BIT0;
	}
}

static void sstvenc_encoder_do_vis(struct sstvenc_encoder* const enc) {
	double	 frequency;
	uint32_t duration_us;
	if (enc->vars.vis.bit >= SSTVENC_VIS_BIT_END) {
		/* This is the end of the VIS header */
		if (enc->mode->initseq) {
			/* There is an initial sequence before the scan */
			sstvenc_encoder_start_initseq(enc);
		} else {
			/* No initial sequence, jump to scan */
			sstvenc_encoder_start_scan(enc);
		}
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		/* Next bit */
		switch (enc->vars.vis.bit) {
		case SSTVENC_VIS_BIT_START1:
			frequency   = SSTVENC_FREQ_VIS_START;
			duration_us = SSTVENC_PERIOD_VIS_START;
			break;
		case SSTVENC_VIS_BIT_START2:
			frequency   = SSTVENC_FREQ_SYNC;
			duration_us = SSTVENC_PERIOD_VIS_SYNC;
			break;
		case SSTVENC_VIS_BIT_START3:
			frequency   = SSTVENC_FREQ_VIS_START;
			duration_us = SSTVENC_PERIOD_VIS_START;
			break;
		case SSTVENC_VIS_BIT_START4:
			frequency   = SSTVENC_FREQ_SYNC;
			duration_us = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_DATA1:
		case SSTVENC_VIS_BIT_DATA2:
		case SSTVENC_VIS_BIT_DATA3:
		case SSTVENC_VIS_BIT_DATA4:
		case SSTVENC_VIS_BIT_DATA5:
		case SSTVENC_VIS_BIT_DATA6:
		case SSTVENC_VIS_BIT_DATA7:
			frequency   = sstvenc_encoder_vis_data_freq(enc);
			duration_us = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_PARITY:
			frequency   = sstvenc_encoder_vis_parity_freq(enc);
			duration_us = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_STOP:
			frequency   = SSTVENC_FREQ_SYNC;
			duration_us = SSTVENC_PERIOD_VIS_BIT;
			break;
		default:
			frequency   = 0;
			duration_us = 0;
			break;
		}
		sstvenc_encoder_start_tone(enc, enc->amplitude, frequency,
					   duration_us,
					   SSTVENC_TS_UNIT_MICROSECONDS);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		enc->vars.vis.bit++;
		break;
	}
}

static void sstvenc_encoder_start_vis(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_VIS);
	enc->vars.vis.bit = 0;
	sstvenc_osc_init(&(enc->cw.osc), enc->amplitude,
			 SSTVENC_FREQ_VIS_START, 0.0, enc->sample_rate);
	sstvenc_encoder_do_vis(enc);
}

#define SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH (0)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH0	(1)
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP01	(2)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH1	(3)
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP12	(4)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH2	(5)
#define SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH	(6)
#define SSTVENC_ENCODER_SCAN_SEGMENT_NEXT	(7)

static void sstvenc_encoder_next_scan_seg(struct sstvenc_encoder* const enc,
					  uint8_t next_segment) {
	enc->vars.scan.segment = next_segment;
	enc->vars.scan.idx     = 0;
}

static void sstvenc_encoder_start_initseq(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_INITSEQ);
	enc->vars.initseq.idx = 0;
	sstvenc_encoder_do_initseq(enc);
}

static void sstvenc_encoder_do_initseq(struct sstvenc_encoder* const enc) {
	const struct sstvenc_encoder_pulse* step
	    = (enc->mode->initseq)
		  ? (&enc->mode->initseq[enc->vars.initseq.idx])
		  : NULL;
	if (!step || !step->duration_ns) {
		/* No sequence, move to the image transmission step */
		sstvenc_encoder_start_scan(enc);
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(enc, enc->amplitude,
					   step->frequency, step->duration_ns,
					   SSTVENC_TS_UNIT_NANOSECONDS);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		enc->vars.initseq.idx++;
		break;
	}
}

static void
sstvenc_encoder_do_scan_seq(struct sstvenc_encoder* const	enc,
			    const struct sstvenc_encoder_pulse* seq,
			    uint8_t next_segment) {
	const struct sstvenc_encoder_pulse* step
	    = (seq) ? (&seq[enc->vars.scan.idx]) : NULL;
	if (!step || !step->duration_ns) {
		/* No sequence, move to the next state */
		sstvenc_encoder_next_scan_seg(enc, next_segment);
		sstvenc_encoder_do_scan(enc);
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(enc, enc->amplitude,
					   step->frequency, step->duration_ns,
					   SSTVENC_TS_UNIT_NANOSECONDS);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		enc->vars.scan.idx++;
		break;
	}
}

static void
sstvenc_encoder_start_scan_channel(struct sstvenc_encoder* const enc,
				   uint8_t			 ch) {
	sstvenc_encoder_start_tone(enc, enc->amplitude, 0,
				   enc->mode->scanline_period_ns[ch],
				   SSTVENC_TS_UNIT_NANOSECONDS);
}

static double
sstvenc_encoder_get_pixel_freq(struct sstvenc_encoder* const enc,
			       uint8_t			     ch) {
	/*
	 * sample_rem is the number of samples before we hit the end of the
	 * scan line, thus is the inverse X axis, scaled.
	 *
	 * scanline_period_ns is in 1ns increments.
	 * sample_rem is in sample rate units.  If we use
	 * sstvenc_ts_samples_to_unit, we can convert that to a fraction of
	 * the scan line, with 0.0 being the right-hand edge.
	 */
	double x_rem_frac
	    = sstvenc_ts_samples_to_unit(enc->sample_rem, enc->sample_rate,
					 SSTVENC_TS_UNIT_NANOSECONDS)
	      / enc->mode->scanline_period_ns[ch];
	uint16_t x   = (uint16_t)(enc->mode->width * (1.0 - x_rem_frac));

	uint32_t idx = sstvenc_get_pixel_posn(enc->mode, x, enc->vars.scan.y);

	return sstvenc_level_freq(enc->framebuffer[idx + ch]);
}

static void sstvenc_encoder_do_scan_channel(struct sstvenc_encoder* const enc,
					    uint8_t ch) {
	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_scan_channel(enc, ch);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_osc_set_frequency(
		    &(enc->cw.osc), sstvenc_encoder_get_pixel_freq(enc, ch));
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);

		/* We have reached the right-side of the image */
		switch (enc->mode->colour_space_order
			& SSTVENC_CSO_MASK_MODE) {
		case SSTVENC_CSO_MODE_MONO:
			/* Next we do the back porch */
			enc->vars.scan.segment
			    = SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH;
			break;
		default:
			switch (ch) {
			case 0:
				enc->vars.scan.segment
				    = SSTVENC_ENCODER_SCAN_SEGMENT_GAP01;
				break;
			case 1:
				enc->vars.scan.segment
				    = SSTVENC_ENCODER_SCAN_SEGMENT_GAP12;
				break;
			default:
				enc->vars.scan.segment
				    = SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH;
				break;
			}
		}

		sstvenc_encoder_do_scan(enc);
		break;
	}
}

static void sstvenc_encoder_do_next_line(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(
	    enc, SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH);
	enc->vars.scan.y++;
	sstvenc_encoder_do_scan(enc);
}

static void sstvenc_encoder_do_scan(struct sstvenc_encoder* const enc) {
	if (enc->vars.scan.y >= enc->mode->height) {
		/* We are at the bottom of the image */
		if (enc->mode->finalseq) {
			/* There's a trail sequence we need to send */
			sstvenc_encoder_start_finalseq(enc);
		} else {
			/* Image data is finished, onto the FSK ID */
			sstvenc_encoder_start_fsk(enc);
		}
		return;
	}

	switch (enc->vars.scan.segment) {
	case SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH:
		sstvenc_encoder_do_scan_seq(enc, enc->mode->frontporch,
					    SSTVENC_ENCODER_SCAN_SEGMENT_CH0);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH0:
		sstvenc_encoder_do_scan_channel(enc, 0);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP01:
		sstvenc_encoder_do_scan_seq(enc, enc->mode->gap01,
					    SSTVENC_ENCODER_SCAN_SEGMENT_CH1);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH1:
		sstvenc_encoder_do_scan_channel(enc, 1);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP12:
		sstvenc_encoder_do_scan_seq(enc, enc->mode->gap12,
					    SSTVENC_ENCODER_SCAN_SEGMENT_CH2);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH2:
		sstvenc_encoder_do_scan_channel(enc, 2);
		break;
	case SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH:
		sstvenc_encoder_do_scan_seq(
		    enc, enc->mode->backporch,
		    SSTVENC_ENCODER_SCAN_SEGMENT_NEXT);
		break;
	default:
		sstvenc_encoder_do_next_line(enc);
		break;
	}
}

static void sstvenc_encoder_start_scan(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_SCAN);
	enc->vars.scan.y = 0;
	sstvenc_encoder_next_scan_seg(
	    enc, SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH);
	sstvenc_encoder_do_scan(enc);
}

static void
sstvenc_encoder_start_finalseq(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_FINALSEQ);
	enc->vars.finalseq.idx = 0;
	sstvenc_encoder_do_finalseq(enc);
}

static void sstvenc_encoder_do_finalseq(struct sstvenc_encoder* const enc) {
	const struct sstvenc_encoder_pulse* step
	    = (enc->mode->finalseq)
		  ? (&enc->mode->finalseq[enc->vars.finalseq.idx])
		  : NULL;
	if (!step || !step->duration_ns) {
		/* No sequence, move to the FSK step */
		sstvenc_encoder_start_fsk(enc);
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(enc, enc->amplitude,
					   step->frequency, step->duration_ns,
					   SSTVENC_TS_UNIT_NANOSECONDS);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		enc->vars.finalseq.idx++;
		break;
	}
}

#define SSTVENC_ENCODER_FSK_SEGMENT_BEGIN    (0)
#define SSTVENC_ENCODER_FSK_SEGMENT_PREAMBLE (1)
#define SSTVENC_ENCODER_FSK_SEGMENT_ID	     (2)
#define SSTVENC_ENCODER_FSK_SEGMENT_TAIL     (3)
#define SSTVENC_ENCODER_FSK_SEGMENT_DONE     (4)
const static uint8_t sstvenc_encoder_fsk_preamble[] = {0x20, 0x2a};
const static uint8_t sstvenc_encoder_fsk_tail[]	    = {0x01};

static void sstvenc_encoder_fsk_load_next(struct sstvenc_encoder* const enc) {
	switch (enc->vars.fsk.segment) {
	case SSTVENC_ENCODER_FSK_SEGMENT_BEGIN:
		enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_PREAMBLE;
		enc->vars.fsk.byte    = 0;
		enc->vars.fsk.seg_sz  = sizeof(sstvenc_encoder_fsk_preamble);
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_PREAMBLE:
		if (enc->vars.fsk.byte < enc->vars.fsk.seg_sz) {
			enc->vars.fsk.bv
			    = sstvenc_encoder_fsk_preamble[enc->vars.fsk
							       .byte];
			enc->vars.fsk.bit = 0;
			break;
		} else {
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_ID;
			enc->vars.fsk.seg_sz = strlen(enc->fsk_id);
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_ID:
		if (enc->vars.fsk.byte < strlen(enc->fsk_id)) {
			enc->vars.fsk.bv
			    = (uint8_t)(enc->fsk_id[enc->vars.fsk.byte])
			      - 0x20;
			enc->vars.fsk.bit = 0;
			break;
		} else {
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_TAIL;
			enc->vars.fsk.seg_sz
			    = sizeof(sstvenc_encoder_fsk_tail);
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_TAIL:
		if (enc->vars.fsk.byte < strlen(enc->fsk_id)) {
			enc->vars.fsk.bv
			    = (uint8_t)(enc->fsk_id[enc->vars.fsk.byte])
			      - 0x20;
			enc->vars.fsk.bit = 0;
			break;
		} else {
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_DONE;
		}
	}
}

static void sstvenc_encoder_start_fsk(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_FSK);
	enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_BEGIN;
	enc->vars.fsk.byte    = 0;
	enc->vars.fsk.bit     = 0;
	sstvenc_encoder_fsk_load_next(enc);
	sstvenc_encoder_do_fsk(enc);
}

static void sstvenc_encoder_do_fsk(struct sstvenc_encoder* const enc) {
	if (enc->fsk_id) {
		double frequency;
		double duration_us;

		if (enc->vars.fsk.bit >= 6) {
			enc->vars.fsk.byte++;
			sstvenc_encoder_fsk_load_next(enc);
		}

		if (enc->vars.fsk.segment
		    >= SSTVENC_ENCODER_FSK_SEGMENT_DONE) {
			/* This is the end of the FSK ID */
			sstvenc_encoder_new_phase(enc,
						  SSTVENC_ENCODER_PHASE_DONE);
			return;
		}

		switch (enc->tone_state) {
		case SSTVENC_ENCODER_TONE_GEN_INIT:
			/* Next bit */
			if (enc->vars.fsk.bv & (1 << enc->vars.fsk.bit)) {
				frequency = SSTVENC_FREQ_FSKID_BIT1;
			} else {
				frequency = SSTVENC_FREQ_FSKID_BIT0;
			}
			sstvenc_encoder_start_tone(
			    enc, enc->amplitude, frequency, duration_us,
			    SSTVENC_TS_UNIT_MICROSECONDS);
			/* Fall-thru */
		case SSTVENC_ENCODER_TONE_GEN_RUN:
			sstvenc_encoder_compute_tone(enc);
			break;
		case SSTVENC_ENCODER_TONE_GEN_DONE:
			sstvenc_encoder_finish_tone(enc);
			enc->vars.fsk.bit++;
			break;
		}
	} else {
		switch (enc->tone_state) {
		case SSTVENC_ENCODER_TONE_GEN_INIT:
			/* Just start a 1kHz tone to quickly tail-off */
			sstvenc_encoder_start_tone(enc, enc->amplitude,
						   1000.0, enc->slope_period,
						   SSTVENC_TS_UNIT_SECONDS);
			/* Fall-thru */
		case SSTVENC_ENCODER_TONE_GEN_RUN:
			sstvenc_encoder_compute_tone(enc);
			break;
		case SSTVENC_ENCODER_TONE_GEN_DONE:
			sstvenc_encoder_finish_tone(enc);
			sstvenc_encoder_new_phase(enc,
						  SSTVENC_ENCODER_PHASE_DONE);
			break;
		}
	}
}

void sstvenc_encoder_compute(struct sstvenc_encoder* const enc) {
	switch (enc->phase) {
	case SSTVENC_ENCODER_PHASE_INIT:
		if (enc->preamble) {
			sstvenc_encoder_start_preamble(enc);
		} else {
			sstvenc_encoder_start_vis(enc);
		}
		break;
	case SSTVENC_ENCODER_PHASE_PREAMBLE:
		sstvenc_encoder_do_preamble(enc);
		break;
	case SSTVENC_ENCODER_PHASE_VIS:
		sstvenc_encoder_do_vis(enc);
		break;
	case SSTVENC_ENCODER_PHASE_INITSEQ:
		sstvenc_encoder_do_initseq(enc);
		break;
	case SSTVENC_ENCODER_PHASE_SCAN:
		sstvenc_encoder_do_scan(enc);
		break;
	case SSTVENC_ENCODER_PHASE_FINALSEQ:
		sstvenc_encoder_do_finalseq(enc);
		break;
	case SSTVENC_ENCODER_PHASE_FSK:
		sstvenc_encoder_do_fsk(enc);
		break;
	default:
		break;
	}
}
