#ifndef _SSTVENC_SSTVMOD_H
#define _SSTVENC_SSTVMOD_H

/*!
 * @addtogroup sstv
 * @defgroup sstv_mod SSTV modulator
 * @{
 *
 * This uses the SSTV encoder defined in @ref sstv and combines it with an
 * oscillator from @ref oscillator and a pulse shaper from @ref pulseshape.
 *
 * Additional logic is supplied to handle time-quantisation jitter that arises
 * due to the discrete sampling intervals.
 *
 * Like the @ref sstv component this is built on, the state machine state is
 * determined by observing sstvenc_mod#ps, in particular, the `phase` member
 * and comparing that to @ref pulseshape_states.  When the value matches
 * @ref SSTVENC_PS_PHASE_DONE, no more meaningful samples will be
 * emitted.
 *
 * The output is returned by the oscillator instance: sstvenc_mod#osc via its
 * `output` member.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/oscillator.h>
#include <libsstvenc/pulseshape.h>
#include <libsstvenc/sstv.h>
#include <stdint.h>

/*!
 * SSTV modulator data structure.
 */
struct sstvenc_mod {
	/*! SSTV encoder state machine */
	struct sstvenc_encoder	  enc;
	/*! Frequency modulation oscillator */
	struct sstvenc_oscillator osc;
	/*! Pulse shaper */
	struct sstvenc_pulseshape ps;
	/*! Total audio samples emitted */
	uint64_t		  total_samples;
	/*! Total time period in nanoseconds emitted */
	uint64_t		  total_ns;
	/*! Remaining number of samples needed to correct timing */
	uint32_t		  remaining;
};

/*!
 * Initialise the SSTV modulator with the given parameters.
 *
 * @param[inout]	mod		SSTV modulator context to initialise
 * @param[in]		mode		SSTV mode to encode
 * @param[in]		fsk_id		FSK ID to send at the end, NULL to
 * 					disable.
 * @param[in]		framebuffer	Framebuffer data representing the
 * 					image.
 * @param[in]		rise_time	Carrier rise time, set to 0 to
 * 					disable.
 * @param[in]		fall_time	Carrier fall time, set to 0 to
 * 					disable.
 * @param[in]		sample_rate	Sample rate in Hz
 * @param[in]		time_unit	Time unit used to measure @a rise_time
 * 					and @a fall_time.
 */
void   sstvenc_modulator_init(struct sstvenc_mod* const	 mod,
			      const struct sstvenc_mode* mode,
			      const char* fsk_id, const uint8_t* framebuffer,
			      double rise_time, double fall_time,
			      uint32_t sample_rate, uint8_t time_unit);

/*!
 * Compute the next audio sample to be emitted from the modulator.
 */
void   sstvenc_modulator_compute(struct sstvenc_mod* const mod);

/*!
 * Fill the given buffer with audio samples from the SSTV modulator.  Stop if
 * we run out of buffer space or if the SSTV state machine finishes.  Return
 * the number of samples generated.
 *
 * @param[inout]	mod		SSTV modulator state machine to pull
 * 					samples from.
 * @param[out]		buffer		Audio buffer to write samples to.
 * @param[in]		buffer_sz	Size of the audio buffer in samples.
 *
 * @returns		Number of samples written to @a buffer
 */
size_t sstvenc_modulator_fill_buffer(struct sstvenc_mod* const mod,
				     double* buffer, size_t buffer_sz);

/*!
 * @}
 * @}
 */

#endif
