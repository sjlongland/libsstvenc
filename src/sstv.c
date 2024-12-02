
/*!
 * @addtogroup sstv
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#ifdef _DEBUG_SSTV
#include <inttypes.h>
#include <stdio.h>
#endif

#include <assert.h>
#include <libsstvenc/sstv.h>
#include <libsstvenc/sstvfreq.h>

/*!
 * @defgroup sstv_vis_bit SSTV VIS header bits
 * @{
 */

/*! Start of VIS header bit 1: 1900Hz pulse for 300ms */
#define SSTVENC_VIS_BIT_START1			(0)

/*! Start of VIS header bit 2: 1200Hz pulse for 10ms */
#define SSTVENC_VIS_BIT_START2			(1)

/*! Start of VIS header bit 3: 1900Hz pulse for 300ms */
#define SSTVENC_VIS_BIT_START3			(2)

/*! Start of VIS header bit 2: 1200Hz pulse for 30ms */
#define SSTVENC_VIS_BIT_START4			(3)

/*!
 * Data bit 1, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA1			(4)

/*!
 * Data bit 2, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA2			(5)

/*!
 * Data bit 3, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA3			(6)

/*!
 * Data bit 4, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA4			(7)

/*!
 * Data bit 5, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA5			(8)

/*!
 * Data bit 6, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA6			(9)

/*!
 * Data bit 7, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_data_freq
 */
#define SSTVENC_VIS_BIT_DATA7			(10)

/*!
 * Parity bit, 30ms pulse with frequency set by
 * @ref sstvenc_encoder_vis_parity_freq
 */
#define SSTVENC_VIS_BIT_PARITY			(11)

/*!
 * VIS header STOP bit, 1200Hz for 30ms
 */
#define SSTVENC_VIS_BIT_STOP			(12)

/*!
 * VIS header state machine final state.
 */
#define SSTVENC_VIS_BIT_END			(13)

/*! @} */

/*!
 * @defgroup sstv_scan_seg SSTV scan line segments
 * @{
 */

/*!
 * Front-porch segment.  This is used for the initial sync pulse and any
 * front-porch pulses needed for the SSTV mode.  Pretty much all SSTV modes
 * define a front porch pulse sequence.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH (0)

/*!
 * Scan line channel 0, present in all SSTV modes.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH0	(1)

/*!
 * The gap between channels 0 and 1.  Optional.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP01	(2)

/*!
 * Scan line channel 1, present in all colour SSTV modes.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH1	(3)

/*!
 * The gap between channels 1 and 2.  Optional.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP12	(4)

/*!
 * Scan line channel 2, present in all colour SSTV modes.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH2	(5)

/*!
 * The gap between channels 2 and 3.  Optional.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_GAP23	(6)

/*!
 * Scan line channel 3, present in Robot36 and the PD modes.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_CH3	(7)

/*!
 * Sync pulses inserted at the end of a scan line prior to beginning the
 * front-porch for the next scan line.  Optional.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH	(8)

/*!
 * Dummy state to indicate the state machine has completed a scan line.
 */
#define SSTVENC_ENCODER_SCAN_SEGMENT_NEXT	(9)
/*! @} */

/*!
 * Transition the encoder to the next phase.  Used as a debugging attachment
 * point in development.
 *
 * @param[out]	enc	SSTV encoder context
 * @param[in]	phase	New phase, @ref sstv_phase
 */
static void sstvenc_encoder_new_phase(struct sstvenc_encoder* const enc,
				      uint8_t			    phase);

/*!
 * Begin a pulse sequence.
 *
 * @param[inout]	enc	SSTV encoder instance
 * @param[in]		seq	Pulse sequence, terminated with a 0ns "pulse".
 * @param[in]		on_done	Callback to run when the pulse sequence ends.
 */
static void sstvenc_encoder_begin_seq(struct sstvenc_encoder* const	  enc,
				      const struct sstvenc_encoder_pulse* seq,
				      sstvenc_encoder_callback* on_done);

/*!
 * Emit the next pulse in the pulse sequence.  If there are no more pulses,
 * call the call-back function (if defined).
 *
 * @param[inout]	enc	SSTV encoder instance
 *
 * @returns	Next pulse in the sequence
 *
 * @retval	NULL	No more pulses in the sequence.
 */
static const struct sstvenc_encoder_pulse*
	    sstvenc_encoder_next_seq_pulse(struct sstvenc_encoder* const enc);

/*!
 * Start sending the VIS header.
 *
 * @param[inout]	enc	SSTV encoder instance
 */
static void sstvenc_encoder_begin_vis(struct sstvenc_encoder* const enc);

/*!
 * Compute the frequency for the next VIS data bit.
 *
 * @param[inout]	enc	SSTV encoder instance
 *
 * @returns	SSTV VIS header data bit frequency in Hz.
 */
static uint32_t
sstvenc_encoder_vis_data_freq(struct sstvenc_encoder* const enc);

/*!
 * Compute the frequency for the VIS parity bit.  The 8th bit of the VIS
 * header is used to invert the parity for modes that require it.
 *
 * @param[inout]	enc	SSTV encoder instance
 *
 * @returns	SSTV VIS header parity bit frequency in Hz.
 */
static uint32_t
sstvenc_encoder_vis_parity_freq(struct sstvenc_encoder* const enc);

/*!
 * Determine and emit the next VIS header pulse to be sent.
 *
 * @param[inout]	enc	SSTV encoder instance
 *
 * @returns	Pointer to SSTV VIS header pulse frequency (in Hz) and
 * duration (in ns)
 *
 * @retval	NULL	End of VIS header.
 */
static const struct sstvenc_encoder_pulse*
	    sstvenc_encoder_next_vis_pulse(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine for the beginning of the image scan.
 *
 * @param[inout]	enc	SSTV encoder instance
 */
static void sstvenc_encoder_begin_image(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine for the beginning of a single scan line.
 *
 * @param[inout]	enc	SSTV encoder instance
 */
static void sstvenc_encoder_begin_scanline(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine for transmitting the given channel of the current
 * scan line.
 *
 * @param[inout]	enc	SSTV encoder instance
 * @param[in]		ch	SSTV scan line channel being emitted
 * 				(0-3 inclusive)
 */
static void sstvenc_encoder_begin_channel(struct sstvenc_encoder* const enc,
					  uint8_t segment, uint8_t ch);

/*!
 * Initialise the state for the next scan segment.  Used as a debugging
 * attachment point in development.
 *
 * @param[inout]	enc		SSTV encoder instance
 * @param[in]		next_segment	The next scan line segment.
 * 					@ref sstv_scan_seg
 */
static void sstvenc_encoder_next_scan_seg(struct sstvenc_encoder* const enc,
					  uint8_t next_segment);

/*!
 * End of initialisation pulse sequence.  This signals the beginning of the
 * image scan.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void
sstvenc_encoder_on_initseq_done(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine for sending the FSK ID.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void sstvenc_encoder_begin_fsk(struct sstvenc_encoder* const enc);

/*!
 * Handle the end of the end-of-image pulse sequence.  This triggers a
 * transition to sending the FSK ID.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void
sstvenc_encoder_on_finalseq_done(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine to begin sending the front-porch sync pulses
 * at the beginning of the scan line.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void
sstvenc_encoder_begin_frontporch(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine to begin sending the sync pulses that separate
 * channels 0 and 1.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void sstvenc_encoder_begin_gap01(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine to begin sending the sync pulses that separate
 * channels 1 and 2.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void sstvenc_encoder_begin_gap12(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine to begin sending the sync pulses that separate
 * channels 2 and 3.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void sstvenc_encoder_begin_gap23(struct sstvenc_encoder* const enc);

/*!
 * Set up the state machine to begin sending the back-porch sync pulses
 * at the end of the scan line.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void
sstvenc_encoder_begin_backporch(struct sstvenc_encoder* const enc);

/*!
 * Compute the frequency of the next pulse for the current pixel in the
 * indicated scan line.
 *
 * @param[inout]	enc		SSTV encoder instance
 * @param[in]		ch		Scan line channel (0-3 inclusive)
 *
 * @returns	Pointer to SSTV pixel pulse.
 *
 * @retval	NULL	End of scan segment.
 */
static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_channel_pulse(struct sstvenc_encoder* const enc,
				   uint8_t			 ch);

/*!
 * Compute the frequency of the next pulse for the image according to the
 * state machine.
 *
 * @param[inout]	enc		SSTV encoder instance
 *
 * @returns	Pointer to SSTV image pulse.
 *
 * @retval	NULL	End of image scan.
 */
static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_image_pulse(struct sstvenc_encoder* const enc);

/*!
 * Determine the value of the next (6-bit) FSK byte to transmit and load it
 * into the state machine.
 *
 * @param[inout]	enc		SSTV encoder instance
 */
static void sstvenc_encoder_fsk_load_next(struct sstvenc_encoder* const enc);

/*!
 * Compute the frequency of the next pulse for the FSK ID according to the
 * state machine.
 *
 * @param[inout]	enc		SSTV encoder instance
 *
 * @returns	Pointer to FSK pulse.
 *
 * @retval	NULL	End of FSK ID.
 */
static const struct sstvenc_encoder_pulse*
	    sstvenc_encoder_next_fsk_pulse(struct sstvenc_encoder* const enc);

static void sstvenc_encoder_new_phase(struct sstvenc_encoder* const enc,
				      uint8_t			    phase) {
#ifdef _DEBUG_SSTV
	printf("%s: entering phase 0x%02x\n", __func__, phase);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: empty sequence\n", __func__);
#endif
		if (on_done) {
#ifdef _DEBUG_SSTV
			printf("%s: calling on_done callback\n", __func__);
#endif
			/* Nothing to do */
			on_done(enc);
		}
	} else {
#ifdef _DEBUG_SSTV
		printf("%s: begin sequence %p\n", __func__, (void*)seq);
#endif
		enc->seq_done_cb = on_done;
	}
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_seq_pulse(struct sstvenc_encoder* const enc) {
	if (enc->seq && enc->seq->duration_ns) {
		const struct sstvenc_encoder_pulse* pulse = enc->seq;
		enc->seq++;
#ifdef _DEBUG_SSTV
		printf("%s: returning pulse at %p\n", __func__, (void*)pulse);
#endif
		return pulse;
	} else if (enc->seq_done_cb) {
#ifdef _DEBUG_SSTV
		printf("%s: end of pulse sequence, calling callback\n",
		       __func__, (void*)(enc->seq_done_cb));
#endif
		sstvenc_encoder_callback* seq_done_cb = enc->seq_done_cb;
		enc->seq_done_cb		      = NULL;
		enc->seq			      = NULL;
		seq_done_cb(enc);
	} else {
#ifdef _DEBUG_SSTV
		printf("%s: end of pulse sequence, no callback\n", __func__);
#endif
		enc->seq_done_cb = NULL;
		enc->seq	 = NULL;
	}
	return NULL;
}

static void sstvenc_encoder_begin_vis(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin sending VIS header\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: VIS header start bit 1\n", __func__);
#endif
		enc->pulse.frequency   = SSTVENC_FREQ_VIS_START;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_START;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START2:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header start bit 2\n", __func__);
#endif
		enc->pulse.frequency   = SSTVENC_FREQ_SYNC;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_SYNC;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START3:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header start bit 3\n", __func__);
#endif
		enc->pulse.frequency   = SSTVENC_FREQ_VIS_START;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_START;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_START4:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header start bit 4\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: VIS header data bit\n", __func__);
#endif
		enc->pulse.frequency   = sstvenc_encoder_vis_data_freq(enc);
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_PARITY:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header parity bit\n", __func__);
#endif
		enc->pulse.frequency   = sstvenc_encoder_vis_parity_freq(enc);
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	case SSTVENC_VIS_BIT_STOP:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header stop bit\n", __func__);
#endif
		enc->pulse.frequency   = SSTVENC_FREQ_SYNC;
		enc->pulse.duration_ns = 1000 * SSTVENC_PERIOD_VIS_BIT;
		enc->vars.vis.bit++;
		return &(enc->pulse);
	default:
#ifdef _DEBUG_SSTV
		printf("%s: VIS header is finished\n", __func__);
#endif
		/* This is the end of the VIS header */
		return NULL;
	}
}

static void
sstvenc_encoder_on_initseq_done(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: initial sequence done\n", __func__);
#endif
	sstvenc_encoder_begin_image(enc);
}

static void sstvenc_encoder_begin_image(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin image scan\n", __func__);
#endif
	enc->phase	 = SSTVENC_ENCODER_PHASE_SCAN;
	enc->vars.scan.y = 0;
	sstvenc_encoder_begin_scanline(enc);
}

static void
sstvenc_encoder_begin_scanline(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_begin_frontporch(enc);
}

static void sstvenc_encoder_begin_channel(struct sstvenc_encoder* const enc,
					  uint8_t segment, uint8_t ch) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u channel %u\n", __func__, enc->vars.scan.y,
	       ch);
#endif
	sstvenc_encoder_next_scan_seg(enc, segment);
	enc->vars.scan.x = 0;
	enc->pulse.duration_ns
	    = (uint32_t)((((double)enc->mode->scanline_period_ns[ch])
			  / enc->mode->width)
			 + 0.5);
#ifdef _DEBUG_SSTV
	printf("%s: %" PRIu32 " ns per pixel\n", __func__,
	       enc->pulse.duration_ns);
#endif
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
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u front porch\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_next_scan_seg(
	    enc, SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH);
	sstvenc_encoder_begin_seq(enc, enc->mode->frontporch, NULL);
}

static void sstvenc_encoder_begin_gap01(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u gap 0/1\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP01);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap01, NULL);
}

static void sstvenc_encoder_begin_gap12(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u gap 1/2\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP12);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap12, NULL);
}

static void sstvenc_encoder_begin_gap23(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u gap 2/3\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_GAP23);
	sstvenc_encoder_begin_seq(enc, enc->mode->gap23, NULL);
}

static void
sstvenc_encoder_begin_backporch(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: begin row %u back porch\n", __func__, enc->vars.scan.y);
#endif
	sstvenc_encoder_next_scan_seg(enc,
				      SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH);
	sstvenc_encoder_begin_seq(enc, enc->mode->backporch, NULL);
}

static void sstvenc_encoder_next_scan_seg(struct sstvenc_encoder* const enc,
					  uint8_t next_segment) {
#ifdef _DEBUG_SSTV
	printf("%s: entering segment 0x%02x\n", __func__, next_segment);
#endif
	enc->vars.scan.segment = next_segment;
}

static void
sstvenc_encoder_on_finalseq_done(struct sstvenc_encoder* const enc) {
#ifdef _DEBUG_SSTV
	printf("%s: end of final image sequence\n", __func__);
#endif
	sstvenc_encoder_begin_fsk(enc);
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_image_pulse(struct sstvenc_encoder* const enc) {
	const struct sstvenc_encoder_pulse* pulse = NULL;
restart:
	switch (enc->vars.scan.segment) {
	case SSTVENC_ENCODER_SCAN_SEGMENT_FRONTPORCH:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
#ifdef _DEBUG_SSTV
		printf("%s: front porch pulse = %p (*, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH0, 0);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH0:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 0);
#ifdef _DEBUG_SSTV
		printf("%s: ch0 pulse = %p (%u, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.x, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap01(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP01:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
#ifdef _DEBUG_SSTV
		printf("%s: gap 0/1 pulse = %p (*, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH1, 1);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH1:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 1);
#ifdef _DEBUG_SSTV
		printf("%s: ch1 pulse = %p (%u, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.x, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap12(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP12:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
#ifdef _DEBUG_SSTV
		printf("%s: gap 1/2 pulse = %p (*, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_channel(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_CH2, 2);
	case SSTVENC_ENCODER_SCAN_SEGMENT_CH2:
		pulse = sstvenc_encoder_next_channel_pulse(enc, 2);
#ifdef _DEBUG_SSTV
		printf("%s: ch2 pulse = %p (%u, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.x, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_gap23(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_GAP23:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
#ifdef _DEBUG_SSTV
		printf("%s: gap 2/3 pulse = %p (*, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.y);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: ch3 pulse = %p (%u, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.x, enc->vars.scan.y);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
		sstvenc_encoder_begin_backporch(enc);
	case SSTVENC_ENCODER_SCAN_SEGMENT_BACKPORCH:
#ifdef _DEBUG_SSTV
		printf("%s: back porch pulse = %p (*, %u)\n", __func__,
		       (void*)pulse, enc->vars.scan.y);
#endif
		pulse = sstvenc_encoder_next_seq_pulse(enc);
#ifdef _DEBUG_SSTV
		printf("%s: back porch pulse = %p\n", __func__, (void*)pulse);
#endif
		if (pulse) {
			return pulse;
		}
		/* Fall-thru */
	default:
#ifdef _DEBUG_SSTV
		printf("%s: end of scan line\n", __func__);
#endif
		sstvenc_encoder_next_scan_seg(
		    enc, SSTVENC_ENCODER_SCAN_SEGMENT_NEXT);
		break;
	}

	/* If we reach here, that's the end of the scan line */
	enc->vars.scan.x = 0;
	switch (enc->mode->colour_space_order & SSTVENC_CSO_MASK_MODE) {
	case SSTVENC_CSO_MODE_YUV2:
#ifdef _DEBUG_SSTV
		printf("%s: YUV2 increment two rows\n", __func__);
#endif
		enc->vars.scan.y += 2;
		break;
	default:
#ifdef _DEBUG_SSTV
		printf("%s: increment single row\n", __func__);
#endif
		enc->vars.scan.y++;
	}

	if (enc->vars.scan.y < enc->mode->height) {
		/* Go again */
#ifdef _DEBUG_SSTV
		printf("%s: repeat for row %u\n", __func__, enc->vars.scan.y);
#endif
		sstvenc_encoder_begin_scanline(enc);
		goto restart;
	}

	/* That's it! */
#ifdef _DEBUG_SSTV
	printf("%s: end of image\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: load FSK preamble\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
			printf("%s: preamble byte 0x%02x\n", __func__,
			       enc->vars.fsk.bv);
#endif
			break;
		} else {
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_ID;
			enc->vars.fsk.seg_sz = strlen(enc->fsk_id);
			enc->vars.fsk.byte   = 0;
#ifdef _DEBUG_SSTV
			printf("%s: end of preamble, load FSK ID byte 0\n",
			       __func__);
#endif
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_ID:
		if (enc->vars.fsk.byte < enc->vars.fsk.seg_sz) {
#ifdef _DEBUG_SSTV
			printf("%s: FSK ID byte %u = 0x%02x\n", __func__,
			       enc->vars.fsk.byte, enc->vars.fsk.bv);
#endif
			enc->vars.fsk.bv
			    = (uint8_t)(enc->fsk_id[enc->vars.fsk.byte])
			      - 0x20;
			enc->vars.fsk.bit = 0;
			break;
		} else {
#ifdef _DEBUG_SSTV
			printf("%s: end of FSK ID, load FSK TAIL byte 0\n",
			       __func__);
#endif
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_TAIL;
			enc->vars.fsk.seg_sz
			    = sizeof(sstvenc_encoder_fsk_tail);
			enc->vars.fsk.byte = 0;
		}
		/* Fall-thru */
	case SSTVENC_ENCODER_FSK_SEGMENT_TAIL:
		if (enc->vars.fsk.byte < enc->vars.fsk.seg_sz) {
#ifdef _DEBUG_SSTV
			printf("%s: FSK TAIL byte %u = 0x%02x\n", __func__,
			       enc->vars.fsk.byte, enc->vars.fsk.bv);
#endif
			enc->vars.fsk.bv
			    = sstvenc_encoder_fsk_tail[enc->vars.fsk.byte];
			enc->vars.fsk.bit = 0;
			break;
		} else {
#ifdef _DEBUG_SSTV
			printf("%s: end of FSK TAIL\n", __func__);
#endif
			enc->vars.fsk.segment
			    = SSTVENC_ENCODER_FSK_SEGMENT_DONE;
			enc->vars.fsk.byte = 0;
		}
	}
}

static void sstvenc_encoder_begin_fsk(struct sstvenc_encoder* const enc) {
	sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_FSK);
	enc->vars.fsk.byte = 0;
	enc->vars.fsk.bit  = 0;

	if (enc->fsk_id) {
#ifdef _DEBUG_SSTV
		printf("%s: initialise FSK state\n", __func__);
#endif
		enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_BEGIN;
		sstvenc_encoder_fsk_load_next(enc);
	} else {
#ifdef _DEBUG_SSTV
		printf("%s: no FSK defined\n", __func__);
#endif
		enc->vars.fsk.segment = SSTVENC_ENCODER_FSK_SEGMENT_DONE;
	}
}

static const struct sstvenc_encoder_pulse*
sstvenc_encoder_next_fsk_pulse(struct sstvenc_encoder* const enc) {
	if (enc->vars.fsk.bit >= 6) {
#ifdef _DEBUG_SSTV
		printf("%s: end of FSK byte\n", __func__);
#endif
		enc->vars.fsk.byte++;
		sstvenc_encoder_fsk_load_next(enc);
	}

	if (enc->vars.fsk.segment >= SSTVENC_ENCODER_FSK_SEGMENT_DONE) {
		/* This is the end of the FSK ID */
#ifdef _DEBUG_SSTV
		printf("%s: end of FSK\n", __func__);
#endif
		return NULL;
	}

	/* Next bit */
	if (enc->vars.fsk.bv & (1 << enc->vars.fsk.bit)) {
#ifdef _DEBUG_SSTV
		printf("%s: FSK bit value 1\n", __func__);
#endif
		enc->pulse.frequency = SSTVENC_FREQ_FSKID_BIT1;
	} else {
#ifdef _DEBUG_SSTV
		printf("%s: FSK bit value 0\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: initialise encoder\n", __func__);
#endif
		sstvenc_encoder_begin_vis(enc);
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_VIS:
#ifdef _DEBUG_SSTV
		printf("%s: next VIS pulse\n", __func__);
#endif
		pulse = sstvenc_encoder_next_vis_pulse(enc);
		if (pulse) {
			break;
		} else {
			sstvenc_encoder_begin_seq(
			    enc, enc->mode->initseq,
			    sstvenc_encoder_on_initseq_done);
		}
		/* Fall-thru */
#ifdef _DEBUG_SSTV
		printf("%s: end of VIS\n", __func__);
#endif
	case SSTVENC_ENCODER_PHASE_INITSEQ:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			break;
		} else {
			sstvenc_encoder_begin_image(enc);
		}
		/* Fall-thru */
#ifdef _DEBUG_SSTV
		printf("%s: end of init sequence\n", __func__);
#endif
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
#ifdef _DEBUG_SSTV
		printf("%s: end of image scan\n", __func__);
#endif
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_FINALSEQ:
		pulse = sstvenc_encoder_next_seq_pulse(enc);
		if (pulse) {
			break;
		}
#ifdef _DEBUG_SSTV
		printf("%s: end of final sequence\n", __func__);
#endif
		/* Fall-thru */
	case SSTVENC_ENCODER_PHASE_FSK:
		pulse = sstvenc_encoder_next_fsk_pulse(enc);
		if (pulse) {
			break;
		}
		/* Fall-thru */
#ifdef _DEBUG_SSTV
		printf("%s: end of FSK ID\n", __func__);
#endif
	default:
		sstvenc_encoder_new_phase(enc, SSTVENC_ENCODER_PHASE_DONE);
		break;
	}

	return pulse;
}

/*!
 * @}
 */
