#ifndef _SSTVENC_PULSESHAPE_H
#define _SSTVENC_PULSESHAPE_H

/*!
 * @defgroup pulseshape Pulse Shaper
 * @{
 *
 * The purpose of this module is to drive the amplitude of an oscillator to
 * ensure the waveform generated contains as few artefacts as possible.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/timescale.h>
#include <math.h>
#include <stdint.h>

/*!
 * @defgroup pulseshape_states Pulse Shaper States
 * @{
 */

/*!
 * Initial state.  No samples have been emitted yet.  @ref sstvenc_ps_compute
 * immediately moves the state machine to SSTVENC_PS_PHASE_RISE and handles
 * the machine accordingly.
 */
#define SSTVENC_PS_PHASE_INIT	 (0)

/*!
 * Rising slope.  The amplitude is being ramped up to maximum.
 *
 * The counter sstvenc_pulseshape#sample_idx is used to track the rise.  When
 * it reaches or exceeds sstvenc_pulseshape#rise_sz, the state machine resets
 * the counter and advances to SSTVENC_PS_PHASE_HOLD.
 */
#define SSTVENC_PS_PHASE_RISE	 (1)

/*!
 * Hold state.  We maintain the pulse amplitude at its maximum for the
 * duration of the pulse.
 *
 * The counter sstvenc_pulseshape#sample_idx is used to track the time period
 * spent at maximum amplitude.  If sstvenc_pulseshape#hold_sz is finite, the
 * counter is reset and the state machine advanced to SSTVENC_PS_PHASE_FALL
 * once it reaches or exceeds sstvenc_pulseshape#hold_sz.
 *
 * Otherwise, it will stay in this state indefinitely, code must call
 * @ref sstvenc_ps_advance manually to exit the hold state.
 */
#define SSTVENC_PS_PHASE_HOLD	 (2)

/*!
 * Falling slope.  The amplitude is being wound down to zero.
 *
 * The counter sstvenc_pulseshape#sample_idx is used to track the fall.  When
 * it reaches or exceeds sstvenc_pulseshape#fall_sz, the state machine resets
 * the counter and advances to SSTVENC_PS_PHASE_DONE.
 */
#define SSTVENC_PS_PHASE_FALL	 (3)

/*!
 * Pulse finished.  The state machine will reset
 * sstvenc_pulseshape#sample_idx to zero on entering this state and will
 * increment it each sample whilst keeping sstvenc_pulseshape#output at
 * zero.
 */
#define SSTVENC_PS_PHASE_DONE	 (4)
/*!
 * @}
 */

/*!
 * Hold time = infinite.
 */
#define SSTVENC_PS_HOLD_TIME_INF SSTVENC_TS_INFINITE

/*!
 * Pulse shaper data structure.  This should be initialised by calling
 * @ref sstvenc_ps_init.
 */
struct sstvenc_pulseshape {
	/*! Peak amplitude of the pulse */
	double	 amplitude;
	/*! The last computed output of the pulse shaper */
	double	 output;
	/*! Sample rate for the pulse in Hz */
	uint32_t sample_rate;
	/*! Sample index for the current phase. */
	uint32_t sample_idx;
	/*! Number of samples for the hold phase. */
	uint32_t hold_sz;
	/*! Number of samples for the rising pulse. */
	uint16_t rise_sz;
	/*! Number of samples for the falling pulse. */
	uint16_t fall_sz;
	/*! Current pulse shaper phase. */
	uint8_t	 phase;
};

/*!
 * Reset the pulse shape state machine with a new hold time given in samples,
 * but otherwise identical settings.
 *
 * @param[inout]	ps		Pulse shaper context being reset
 * @param[in]		hold_time	New hold time (given as number of
 * 					samples).
 */
static inline void
sstvenc_ps_reset_samples(struct sstvenc_pulseshape* const ps,
			 uint32_t			  hold_time) {
	ps->phase      = SSTVENC_PS_PHASE_INIT;
	ps->hold_sz    = hold_time;
	ps->sample_idx = 0;
}

/*!
 * Reset the pulse shape state machine with a new hold time, but otherwise
 * identical settings.
 *
 * @param[inout]	ps		Pulse shaper context being reset
 * @param[in]		hold_time	New hold time given in the time unit
 * 					specified by @a time_unit.
 * @param[in]		time_unit	Time unit used to measure @a
 * hold_time. Must be one of the values given in
 * 					@ref timescale_units
 */
static inline void sstvenc_ps_reset(struct sstvenc_pulseshape* const ps,
				    double hold_time, uint8_t time_unit) {
	uint32_t samples = sstvenc_ts_unit_to_samples(
	    hold_time, ps->sample_rate, time_unit);
	sstvenc_ps_reset_samples(ps, samples);
}

/*!
 * Initialise a pulse shaper.
 *
 * @param[out]	ps		The pulse shaper being initialised
 * @param[in]	amplitude	The peak amplitude for the pulse shaper
 * @param[in]	rise_time	The rise time in seconds, use 0 to disable
 * 				rise.
 * @param[in]	hold_time	The hold time in seconds, use INFINITY for
 * 				an indefinite hold.
 * @param[in]	fall_time	The fall time in seconds, use 0 to disable
 * 				fall.
 * @param[in]	sample_rate	The sample rate in Hz.
 * @param[in]	time_unit	The time unit used to measure @a rise_time,
 * 				@a hold_time and @a fall_time.
 */
static inline void sstvenc_ps_init(struct sstvenc_pulseshape* const ps,
				   double amplitude, double rise_time,
				   double hold_time, double fall_time,
				   uint32_t sample_rate, uint8_t time_unit) {
	uint32_t samples;

	ps->amplitude	= amplitude;
	ps->sample_rate = sample_rate;
	ps->output	= 0.0;

	samples
	    = sstvenc_ts_unit_to_samples(rise_time, sample_rate, time_unit);
	if (samples > UINT16_MAX) {
		ps->rise_sz = UINT16_MAX;
	} else {
		ps->rise_sz = samples;
	}

	samples
	    = sstvenc_ts_unit_to_samples(fall_time, sample_rate, time_unit);
	if (samples > UINT16_MAX) {
		ps->fall_sz = UINT16_MAX;
	} else {
		ps->fall_sz = samples;
	}

	sstvenc_ps_reset(ps, hold_time, time_unit);
}

/*!
 * Advance the pulse shaper to the next phase, regardless of whether it is
 * finished with the present one.  A no-op at the "done" phase.
 *
 * @param[out]	ps		The pulse shaper being advanced.
 */
static inline void sstvenc_ps_advance(struct sstvenc_pulseshape* const ps) {
	if (ps->phase < SSTVENC_PS_PHASE_DONE) {
		ps->sample_idx = 0;
		ps->phase++;
	}
}

/*!
 * Compute the next pulse shaper value and store it in the output field.
 * A no-op if the sample rate is set to zero or the pulse shaper is done.
 *
 * The output envelope amplitude can be read from sstvenc_pulseshape#output.
 *
 * @param[out]	ps		The pulse shaper being computed.
 */
static inline void sstvenc_ps_compute(struct sstvenc_pulseshape* const ps) {
	ps->sample_idx++;

	switch (ps->phase) {
	case SSTVENC_PS_PHASE_INIT:
		ps->phase++;
		/* Fall-thru */
	case SSTVENC_PS_PHASE_RISE:
		if (ps->rise_sz) {
			/* Compute the raised sine amplitude */
			ps->output = ps->amplitude
				     * (1.0
					- cos((ps->sample_idx * M_PI)
					      / (2 * ((double)ps->rise_sz))));
			if (ps->output > ps->amplitude) {
				ps->output = ps->amplitude;
			}
		}
		if (ps->sample_idx > ps->rise_sz) {
			/* Next phase */
			sstvenc_ps_advance(ps);
		}
		break;
	case SSTVENC_PS_PHASE_HOLD:
		ps->output = ps->amplitude;
		if ((ps->hold_sz != SSTVENC_PS_HOLD_TIME_INF)
		    && (ps->sample_idx >= ps->hold_sz)) {
			/* Next phase */
			sstvenc_ps_advance(ps);
		}
		break;
	case SSTVENC_PS_PHASE_FALL:
		if (ps->fall_sz) {
			/* Compute the raised sine amplitude */
			ps->output
			    = ps->amplitude
			      * (1.0
				 - cos(((ps->fall_sz - ps->sample_idx) * M_PI)
				       / (2 * ((double)ps->fall_sz))));

			if (ps->output > ps->amplitude) {
				ps->output = ps->amplitude;
			}
		}
		if (ps->sample_idx >= ps->fall_sz) {
			/* Next phase */
			sstvenc_ps_advance(ps);
		}
		break;
	case SSTVENC_PS_PHASE_DONE:
	default:
		ps->output = 0.0;
		break;
	}
}

/*! @} */

#endif
