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

#define SSTVENC_PERIOD_VIS_START (0.300)
#define SSTVENC_PERIOD_VIS_SYNC	 (0.010)
#define SSTVENC_PERIOD_VIS_BIT	 (0.030)
#define SSTVENC_PERIOD_FSKID_BIT (0.022)

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
#define SSTVENC_VIS_BIT_DATA8	 (11)
#define SSTVENC_VIS_BIT_PARITY	 (12)
#define SSTVENC_VIS_BIT_STOP	 (13)
#define SSTVENC_VIS_BIT_END	 (14)

/* SSTV mode specifications -- Robot B/W modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_robotbw_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration = 0.007},
    {.frequency = 0, .duration = 0},
};

/* SSTV mode specifications -- the table */
#define SSTVENC_SSTV_MODES_NUM (2)
const static struct sstvenc_mode sstvenc_sstv_modes[SSTVENC_SSTV_MODES_NUM]
    = {
	{
	    .description     = "Robot 8 B/W",
	    .name	     = "R8BW",
	    .frontporch	     = sstvenc_sstv_robotbw_fp,
	    .gap01	     = NULL,
	    .gap12	     = NULL,
	    .backporch	     = NULL,
	    .scanline_period = {0.0599, 0.0, 0.0},
	    .width	     = 160,
	    .height	     = 120,
	    .colour_space_order
	    = SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y,
				 SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	    .vis_code = 0x02,
	},
	{
	    .description     = "Robot 24 B/W",
	    .name	     = "R24BW",
	    .frontporch	     = sstvenc_sstv_robotbw_fp,
	    .gap01	     = NULL,
	    .gap12	     = NULL,
	    .backporch	     = NULL,
	    .scanline_period = {0.093, 0.0, 0.0},
	    .width	     = 320,
	    .height	     = 240,
	    .colour_space_order
	    = SSTVENC_MODE_ORDER(SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y,
				 SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	    .vis_code = 0x0a,
	},
};

uint8_t sstvenc_get_mode_count() { return SSTVENC_SSTV_MODES_NUM; }

const struct sstvenc_mode* sstvenc_get_mode_by_idx(uint8_t idx) {
	if (idx < SSTVENC_SSTV_MODES_NUM) {
		return &sstvenc_sstv_modes[idx];
	} else {
		return NULL;
	}
}

const struct sstvenc_mode* sstvenc_get_mode_by_name(const char* name) {
	for (uint8_t idx = 0; idx < SSTVENC_SSTV_MODES_NUM; idx++) {
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

static void sstvenc_encoder_start_preamble(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_preamble(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_vis(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_vis(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_scan(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_scan(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_start_fsk(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_do_fsk(struct sstvenc_encoder* const enc);

static void sstvenc_encoder_preamble_next(struct sstvenc_encoder* const enc) {
	enc->vars.preamble.step++;
}

#define SSTVENC_ENCODER_TONE_GEN_INIT (0)
#define SSTVENC_ENCODER_TONE_GEN_RUN  (1)
#define SSTVENC_ENCODER_TONE_GEN_DONE (2)

static void sstvenc_encoder_start_tone(struct sstvenc_encoder* const enc,
				       double amplitude, double frequency,
				       double duration) {
	assert(enc->tone_state == SSTVENC_ENCODER_TONE_GEN_INIT);
	enc->sample_rem = ((uint16_t)((duration * enc->sample_rate) + 0.5));
	sstvenc_osc_init(&(enc->cw.osc), amplitude, frequency, 0.0,
			 enc->sample_rate);
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
	memset(&(enc->cw), 0, sizeof(struct sstvenc_cw_mod));
	enc->tone_state = SSTVENC_ENCODER_TONE_GEN_INIT;
}

static void
sstvenc_encoder_preamble_do_tone(struct sstvenc_encoder* const enc) {
	const struct sstvenc_preamble_step* step
	    = &(enc->preamble[enc->vars.preamble.step]);
	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(enc, step->amplitude,
					   step->frequency,
					   step->data.tone.duration);
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
				enc->sample_rate);
		/* Fall-thru */
	case SSTVENC_CW_MOD_STATE_NEXT_SYM:
	case SSTVENC_CW_MOD_STATE_MARK:
	case SSTVENC_CW_MOD_STATE_DITSPACE:
	case SSTVENC_CW_MOD_STATE_DAHSPACE:
		sstvenc_cw_compute(&(enc->cw));
		enc->output = enc->cw.output;
		break;
	case SSTVENC_CW_MOD_STATE_DONE:
		sstvenc_ps_reset(&(enc->cw.ps), INFINITY);
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
	enc->phase		      = SSTVENC_ENCODER_PHASE_PREAMBLE;
	enc->vars.preamble.step	      = 0;
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
	double frequency;
	double duration;
	if (enc->vars.vis.bit >= SSTVENC_VIS_BIT_END) {
		/* This is the end of the VIS header */
		sstvenc_encoder_start_scan(enc);
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		/* Next bit */
		switch (enc->vars.vis.bit) {
		case SSTVENC_VIS_BIT_START1:
			frequency = SSTVENC_FREQ_VIS_START;
			duration  = SSTVENC_PERIOD_VIS_START;
			break;
		case SSTVENC_VIS_BIT_START2:
			frequency = SSTVENC_FREQ_SYNC;
			duration  = SSTVENC_PERIOD_VIS_SYNC;
			break;
		case SSTVENC_VIS_BIT_START3:
			frequency = SSTVENC_FREQ_VIS_START;
			duration  = SSTVENC_PERIOD_VIS_START;
			break;
		case SSTVENC_VIS_BIT_START4:
			frequency = SSTVENC_FREQ_SYNC;
			duration  = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_DATA1:
		case SSTVENC_VIS_BIT_DATA2:
		case SSTVENC_VIS_BIT_DATA3:
		case SSTVENC_VIS_BIT_DATA4:
		case SSTVENC_VIS_BIT_DATA5:
		case SSTVENC_VIS_BIT_DATA6:
		case SSTVENC_VIS_BIT_DATA7:
		case SSTVENC_VIS_BIT_DATA8:
			frequency = sstvenc_encoder_vis_data_freq(enc);
			duration  = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_PARITY:
			frequency = sstvenc_encoder_vis_parity_freq(enc);
			duration  = SSTVENC_PERIOD_VIS_BIT;
			break;
		case SSTVENC_VIS_BIT_STOP:
			frequency = SSTVENC_FREQ_SYNC;
			duration  = SSTVENC_PERIOD_VIS_BIT;
			break;
		default:
			frequency = 0;
			duration  = 0;
			break;
		}
		sstvenc_encoder_start_tone(enc, enc->amplitude, frequency,
					   duration);
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
	enc->phase	  = SSTVENC_ENCODER_PHASE_VIS;
	enc->vars.vis.bit = 0;
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

static void
sstvenc_encoder_do_scan_seq(struct sstvenc_encoder* const	enc,
			    const struct sstvenc_encoder_pulse* seq,
			    uint8_t next_segment) {
	const struct sstvenc_encoder_pulse* step
	    = (seq) ? (&seq[enc->vars.scan.idx]) : NULL;
	if (!step || !step->duration) {
		/* No sequence, move to the next state */
		enc->vars.scan.x       = 0;
		enc->vars.scan.segment = next_segment;
		sstvenc_encoder_do_scan(enc);
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_tone(enc, enc->amplitude,
					   step->frequency, step->duration);
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
	sstvenc_encoder_start_tone(
	    enc, enc->amplitude,
	    sstvenc_level_freq(
		enc->framebuffer[sstvenc_get_pixel_posn(enc, enc->vars.scan.x,
							enc->vars.scan.y)
				 + ch]),
	    enc->mode->scanline_period[ch] / enc->mode->width);
}

static void sstvenc_encoder_do_scan_channel(struct sstvenc_encoder* const enc,
					    uint8_t ch) {
	if (enc->vars.scan.x >= enc->mode->width) {
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
		return;
	}

	switch (enc->tone_state) {
	case SSTVENC_ENCODER_TONE_GEN_INIT:
		sstvenc_encoder_start_scan_channel(enc, ch);
		/* Fall-thru */
	case SSTVENC_ENCODER_TONE_GEN_RUN:
		sstvenc_encoder_compute_tone(enc);
		break;
	case SSTVENC_ENCODER_TONE_GEN_DONE:
		sstvenc_encoder_finish_tone(enc);
		enc->vars.scan.x++;
		break;
	}
}

static void sstvenc_encoder_do_next_line(struct sstvenc_encoder* const enc) {
	enc->vars.scan.x       = 0;
	enc->vars.scan.segment = SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH;
	enc->vars.scan.y++;
	sstvenc_encoder_do_scan(enc);
}

static void sstvenc_encoder_do_scan(struct sstvenc_encoder* const enc) {
	if (enc->vars.scan.y >= enc->mode->height) {
		/* We are at the bottom of the image */
		sstvenc_encoder_start_fsk(enc);
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
	enc->phase	       = SSTVENC_ENCODER_PHASE_SCAN;
	enc->vars.scan.x       = 0;
	enc->vars.scan.y       = 0;
	enc->vars.scan.segment = 0;
	enc->vars.scan.idx     = 0;
	sstvenc_encoder_do_scan(enc);
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
	enc->phase	      = SSTVENC_ENCODER_PHASE_FSK;
	enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_BEGIN;
	enc->vars.fsk.byte    = 0;
	enc->vars.fsk.bit     = 0;
	sstvenc_encoder_fsk_load_next(enc);
	sstvenc_encoder_do_fsk(enc);
}

static void sstvenc_encoder_do_fsk(struct sstvenc_encoder* const enc) {
	if (enc->fsk_id) {
		double frequency;
		double duration;

		if (enc->vars.fsk.bit >= 6) {
			enc->vars.fsk.byte++;
			sstvenc_encoder_fsk_load_next(enc);
		}

		if (enc->vars.fsk.segment
		    >= SSTVENC_ENCODER_FSK_SEGMENT_DONE) {
			/* This is the end of the FSK ID */
			enc->phase = SSTVENC_ENCODER_PHASE_DONE;
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
			sstvenc_encoder_start_tone(enc, enc->amplitude,
						   frequency, duration);
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
						   1000.0, enc->slope_period);
			/* Fall-thru */
		case SSTVENC_ENCODER_TONE_GEN_RUN:
			sstvenc_encoder_compute_tone(enc);
			break;
		case SSTVENC_ENCODER_TONE_GEN_DONE:
			sstvenc_encoder_finish_tone(enc);
			enc->phase = SSTVENC_ENCODER_PHASE_DONE;
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
	case SSTVENC_ENCODER_PHASE_SCAN:
		sstvenc_encoder_do_scan(enc);
		break;
	case SSTVENC_ENCODER_PHASE_FSK:
		sstvenc_encoder_do_fsk(enc);
		break;
	default:
		break;
	}
}
