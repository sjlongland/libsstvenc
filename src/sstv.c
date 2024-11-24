/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <libsstvenc/sstv.h>
#include <libsstvenc/sstvfreq.h>

#define SSTVENC_VIS_BIT_START1 (0)
#define SSTVENC_VIS_BIT_START2 (1)
#define SSTVENC_VIS_BIT_START3 (2)
#define SSTVENC_VIS_BIT_START4 (3)
#define SSTVENC_VIS_BIT_DATA1  (4)
#define SSTVENC_VIS_BIT_DATA2  (5)
#define SSTVENC_VIS_BIT_DATA3  (6)
#define SSTVENC_VIS_BIT_DATA4  (7)
#define SSTVENC_VIS_BIT_DATA5  (8)
#define SSTVENC_VIS_BIT_DATA6  (9)
#define SSTVENC_VIS_BIT_DATA7  (10)
#define SSTVENC_VIS_BIT_PARITY (11)
#define SSTVENC_VIS_BIT_STOP   (12)
#define SSTVENC_VIS_BIT_END    (13)

static void sstvenc_encoder_new_phase(struct sstvenc_encoder* const enc,
				      uint8_t			    phase) {
	enc->phase = phase;
}

void sstvenc_encoder_init(struct sstvenc_encoder* const enc,
			  const struct sstvenc_mode* mode, const char* fsk_id,
			  const uint8_t* framebuffer) {
	memset(enc, 0, sizeof(struct sstvenc_encoder));
	enc->mode	 = mode;
	enc->fsk_id	 = fsk_id;
	enc->framebuffer = framebuffer;
	enc->phase	 = SSTVENC_ENCODER_PHASE_INIT;
}

static void sstvenc_encoder_begin_seq(struct sstvenc_encoder* const	  enc,
				      const struct sstvenc_encoder_pulse* seq,
				      sstvenc_encoder_callback* on_done) {
	enc->seq = seq;
	if ((!enc->seq) || (!enc->seq->duration_ns)) {
		/* Nothing to do */
		on_done(enc);
	} else {
		enc->seq_done_cb = on_done;
	}
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_seq_pulse(struct sstvenc_encoder* const enc) {
	if (enc->seq && enc->seq->duration_ns) {
		const struct sstvenc_encoder_pulse* pulse = enc->seq;
		enc->seq++;
		return pulse;
	} else if (enc->seq_done_cb) {
		sstvenc_encoder_callback* seq_done_cb = enc->seq_done_cb;
		enc->seq_done_cb		      = NULL;
		enc->seq			      = NULL;
		seq_done_cb(enc);
	} else {
		enc->seq_done_cb = NULL;
		enc->seq	 = NULL;
	}
	return NULL;
}

static void sstvenc_encoder_begin_vis(struct sstvenc_encoder* const enc) {
	enc->phase	  = SSTVENC_ENCODER_PHASE_VIS;
	enc->vars.vis.bit = SSTVENC_VIS_BIT_START1;
}

static uint32_t
sstvenc_encoder_vis_data_freq(struct sstvenc_encoder* const enc) {
	uint8_t bit = enc->vars.vis.bit - SSTVENC_VIS_BIT_DATA1;
	if (enc->mode->vis_code & (1 << bit)) {
		return SSTVENC_FREQ_VIS_BIT1;
	} else {
		return SSTVENC_FREQ_VIS_BIT0;
	}
}

static uint32_t
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

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_vis_pulse(struct sstvenc_encoder* const enc) {
	switch (enc->vars.vis.bit) {
	case SSTVENC_VIS_BIT_START1:
		enc->pulse.frequency   = SSTVENC_FREQ_VIS_START;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_START;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START2:
		enc->pulse.frequency   = SSTVENC_FREQ_SYNC;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_SYNC;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START3:
		enc->pulse.frequency   = SSTVENC_FREQ_VIS_START;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_START;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START4:
		enc->pulse.frequency   = SSTVENC_FREQ_SYNC;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_DATA1:
	case SSTVENC_VIS_BIT_DATA2:
	case SSTVENC_VIS_BIT_DATA3:
	case SSTVENC_VIS_BIT_DATA4:
	case SSTVENC_VIS_BIT_DATA5:
	case SSTVENC_VIS_BIT_DATA6:
	case SSTVENC_VIS_BIT_DATA7:
		enc->pulse.frequency   = sstvenc_encoder_vis_data_freq(enc);
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_PARITY:
		enc->pulse.frequency   = sstvenc_encoder_vis_parity_freq(enc);
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_STOP:
		enc->pulse.frequency   = SSTVENC_FREQ_SYNC;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	default:
		/* This is the end of the VIS header */
		return NULL;
	}
}

#define SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH (0)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH0	(1)
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP01	(2)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH1	(3)
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP12	(4)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH2	(5)
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP23	(6)
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH3	(7)
#define SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH	(8)
#define SSTVENC_ENCODER_SCAN_SEGMENT_NEXT	(9)

static void sstvenc_encoder_begin_image(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_begin_scanline(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_begin_channel(struct sstvenc_encoder* const enc,
					  uint8_t segment, uint8_t ch);
static void sstvenc_encoder_next_scan_seg(struct sstvenc_encoder* const enc,
					  uint8_t next_segment);

static void
sstvenc_encoder_on_initseq_done(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_begin_image(enc);
}

static void sstvenc_encoder_begin_fsk(struct sstvenc_encoder* const enc);
static void
sstvenc_encoder_on_finalseq_done(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_begin_fsk(enc);
}

static void sstvenc_encoder_begin_image(struct sstvenc_encoder* const enc) {
	enc->phase	 = SSTVENC_ENCODER_PHASE_SCAN;
	enc->vars.scan.y = 0;
	sstvenc_encoder_begin_scanline(enc);
}

static void
sstvenc_encoder_begin_frontporch(struct sstvenc_encoder* const enc);
static void
sstvenc_encoder_on_frontporch_done(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_begin_gap01(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_on_gap01_done(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_begin_gap12(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_on_gap12_done(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_begin_gap23(struct sstvenc_encoder* const enc);
static void sstvenc_encoder_on_gap23_done(struct sstvenc_encoder* const enc);
static void
sstvenc_encoder_begin_backporch(struct sstvenc_encoder* const enc);
static void
sstvenc_encoder_on_backporch_done(struct sstvenc_encoder* const enc);

static void
sstvenc_encoder_begin_scanline(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_begin_frontporch(enc);
}

static void sstvenc_encoder_begin_channel(struct sstvenc_encoder* const enc,
					  uint8_t segment, uint8_t ch) {
	sstvenc_encoder_next_scan_seg(enc, segment);
	enc->vars.scan.x = 0;
	enc->pulse.duration_ns
	    = (uint32_t)((((double)enc->mode->scanline_period_ns[ch])
			  / enc->mode->width)
			 + 0.5);
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_channel_pulse(struct sstvenc_encoder* const enc,
				   uint8_t			 ch) {

	if (enc->vars.scan.x >= enc->mode->width) {
		/* End of the channel */
		return NULL;
	}

	uint32_t idx = sstvenc_get_pixel_posn(enc->mode, enc->vars.scan.x,
					      enc->vars.scan.y);
	uint8_t	 value;

	switch (enc->mode->colour_space_order & SSTVENC_CSO_MASK_MODE) {
	case SSTVENC_CSO_MODE_YUV2: {
		const uint16_t row_length = 3 * enc->mode->width;
		assert(!(enc->vars.scan.y % 2));

		switch (
		    SSTVENC_MODE_GET_CH(ch, enc->mode->colour_space_order)) {
		case SSTVENC_CSO_CH_NONE:
			/* Channel not used */
			return NULL;
		case SSTVENC_CSO_CH_Y:
			value = enc->framebuffer[idx];
			break;
		case SSTVENC_CSO_CH_Y2:
			value = enc->framebuffer[idx + row_length];
			break;
		case SSTVENC_CSO_CH_U:
			value = (enc->framebuffer[idx + 1]
				 + enc->framebuffer[idx + row_length + 1])
				/ 2.0;
			break;
		case SSTVENC_CSO_CH_V:
			value = (enc->framebuffer[idx + 2]
				 + enc->framebuffer[idx + row_length + 2])
				/ 2.0;
			break;
		default:
			value = 0.0;
		}
		break;
	}
	default:
		switch (
		    SSTVENC_MODE_GET_CH(ch, enc->mode->colour_space_order)) {
		case SSTVENC_CSO_CH_NONE:
			/* Channel not used */
			return NULL;
		case SSTVENC_CSO_CH_Y:
		case SSTVENC_CSO_CH_R:
			value = enc->framebuffer[idx];
			break;
		case SSTVENC_CSO_CH_U:
		case SSTVENC_CSO_CH_G:
			value = enc->framebuffer[idx + 1];
			break;
		case SSTVENC_CSO_CH_V:
		case SSTVENC_CSO_CH_B:
			value = enc->framebuffer[idx + 2];
			break;
		default:
			value = 0.0;
		}
	}

	enc->pulse.frequency = sstvenc_level_freq(value);
	enc->vars.scan.x++;
	return &(enc->pulse);
}

static void
sstvenc_encoder_begin_frontporch(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(
	    enc, SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH);
	sstvenc_encoder_begin_seq(enc, enc->mode->frontporch,
				  sstvenc_encoder_on_frontporch_done);
}

static void
sstvenc_encoder_on_frontporch_done(struct sstvenc_encoder* const enc) {}

static void sstvenc_encoder_begin_gap01(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP01);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap01,
				  sstvenc_encoder_on_gap01_done);
}

static void sstvenc_encoder_on_gap01_done(struct sstvenc_encoder* const enc) {
}

static void sstvenc_encoder_begin_gap12(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP12);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap12,
				  sstvenc_encoder_on_gap12_done);
}

static void sstvenc_encoder_on_gap12_done(struct sstvenc_encoder* const enc) {
}

static void sstvenc_encoder_begin_gap23(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP23);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap23,
				  sstvenc_encoder_on_gap23_done);
}

static void sstvenc_encoder_on_gap23_done(struct sstvenc_encoder* const enc) {
}

static void
sstvenc_encoder_begin_backporch(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH);
	sstvenc_encoder_begin_seq(enc, enc->mode->backporch,
				  sstvenc_encoder_on_backporch_done);
}

static void
sstvenc_encoder_on_backporch_done(struct sstvenc_encoder* const enc) {}

static void sstvenc_encoder_next_scan_seg(struct sstvenc_encoder* const enc,
					  uint8_t next_segment) {
	enc->vars.scan.segment = next_segment;
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_image_pulse(struct sstvenc_encoder* const enc) {
	const struct sstvenc_encoder_pulse* pulse = NULL;
restart:
	switch (enc->vars.scan.segment) {
	case SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH0, 0);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH0:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 0);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap01(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP01:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH1, 1);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH1:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 1);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap12(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP12:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH2, 2);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH2:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 2);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap23(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP23:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_next_scan_seg(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH3);
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH3, 3);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH3:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 3);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_backporch(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
	default:
		sstvenc_encoder_next_scan_seg(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_NEXT);
		break;
	}

	/* If we reach here, that's the end of the scan line */
	enc->vars.scan.x = 0;
	switch (enc->mode->colour_space_order & SSTVENC_CSO_MASK_MODE) {
	case SSTVENC_CSO_MODE_YUV2:
		enc->vars.scan.y += 2;
		break;
	default:
		enc->vars.scan.y++;
	}

	if (enc->vars.scan.y < enc->mode->height) {
		/* Go again */
		sstvenc_encoder_begin_scanline(enc);
		goto restart;
	}

	/* That's it! */
	return NULL;
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
			enc->vars.fsk.byte   = 0;
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_ID:
		if (enc->vars.fsk.byte < enc->vars.fsk.seg_sz) {
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
			enc->vars.fsk.byte = 0;
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_TAIL:
		if (enc->vars.fsk.byte < enc->vars.fsk.seg_sz) {
			enc->vars.fsk.bv
			    = sstvenc_encoder_fsk_tail[enc->vars.fsk.byte];
			enc->vars.fsk.bit = 0;
			break;
		} else {
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_DONE;
			enc->vars.fsk.byte = 0;
		}
	}
}

static void sstvenc_encoder_begin_fsk(struct sstvenc_encoder* const enc) {
	if (enc->fsk_id) {
		sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_FSK);
		enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_BEGIN;
		enc->vars.fsk.byte    = 0;
		enc->vars.fsk.bit     = 0;
		sstvenc_encoder_fsk_load_next(enc);
	}
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_fsk_pulse(struct sstvenc_encoder* const enc) {
	if (enc->vars.fsk.bit >= 6) {
		enc->vars.fsk.byte++;
		sstvenc_encoder_fsk_load_next(enc);
	}

	if (enc->vars.fsk.segment >= SSTVENC_ENCODER_FSK_SEGMENT_DONE) {
		/* This is the end of the FSK ID */
		return NULL;
	}

	/* Next bit */
	if (enc->vars.fsk.bv & (1 << enc->vars.fsk.bit)) {
		enc->pulse.frequency = SSTVENC_FREQ_FSKID_BIT1;
	} else {
		enc->pulse.frequency = SSTVENC_FREQ_FSKID_BIT0;
	}
	enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_FSKID_BIT;
	enc->vars.fsk.bit++;

	return &(enc->pulse);
}

const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_pulse(struct sstvenc_encoder* const enc) {
	const struct sstvenc_encoder_pulse* pulse = NULL;
	switch (enc->phase) {
	case SSTVENC_ENCODER_PHASE_INIT:
		sstvenc_encoder_begin_vis(enc);
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_VIS:
		pulse = sstvenc_encoder_next_vis_pulse(enc);
		if (pulse) {
			break;
		} else {
			sstvenc_encoder_begin_seq(
			    enc, enc->mode->initseq,
			    sstvenc_encoder_on_initseq_done);
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_INITSEQ:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			break;
		} else {
			sstvenc_encoder_begin_image(enc);
		}
		/* Fall-thru */
		break;
	case SSTVENC_ENCODER_PHASE_SCAN:
		pulse = sstvenc_encoder_next_image_pulse(enc);
		if (pulse) {
			break;
		} else {
			sstvenc_encoder_begin_seq(
			    enc, enc->mode->finalseq,
			    sstvenc_encoder_on_finalseq_done);
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_FINALSEQ:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			break;
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_FSK:
		pulse = sstvenc_encoder_next_fsk_pulse(enc);
		if (pulse) {
			break;
		}
		/* Fall-thru */
	default:
		sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_DONE);
		break;
	}

	return pulse;
}
