#ifndef _SSTVENC_OSCILLATOR_H
#define _SSTVENC_OSCILLATOR_H

/*!
 * @defgroup oscillator Oscillator implementation
 * @{
 *
 * This is a simple module that can be used to produce a single sinusoid tone
 * at a given frequency for a given sample rate.
 *
 * The phase is computed at each sample step, and increments modulo 2-Pi.
 * Frequency, amplitude and phase can be modified at any time, they take
 * effect on the next call to @ref sstvenc_osc_compute.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>

/*!
 * Oscillator data structure.  This must remain allocated for the lifetime of
 * the sinusoid.
 *
 * Use @ref sstvenc_osc_init to initialise this structure.
 */
struct sstvenc_oscillator {
	/*!
	 * The amplitude of the sinusoid: range 0-1.0.
	 *
	 * This may be adjusted to ampiltude-modulate the oscillator.  The
	 * new amplitude takes effect on the next call to
	 * @ref sstvenc_osc_compute.
	 */
	double	 amplitude;
	/*!
	 * Phase offset in radians: range 0-2*M_PI.
	 *
	 * This may be adjusted to phase-modulate the oscillator.  The new
	 * phase offset takes effect on the next call to
	 * @ref sstvenc_osc_compute.
	 */
	double	 offset;
	/*!
	 * The last computed output of the sinusoid.  The audio output should
	 * be read from this field.
	 */
	double	 output;
	/*!
	 * Sample rate for the sinusoid in Hz.  Must not be changed after
	 * initialisation.
	 */
	uint32_t sample_rate;
	/*!
	 * Fixed-point phase of the sinusoid oscillator.  Do not manipulate
	 * directly, use @ref sstvenc_osc_init or @ref sstvenc_osc_compute to
	 * update.  If just re-starting a sine with identical settings, you
	 * *may* set this to 0.
	 */
	uint32_t phase;
	/*!
	 * Fixed-point phase increment each iteration.  Use
	 * @ref sstvenc_osc_set_frequency to adjust this or
	 * @ref sstvenc_osc_get_frequency to convert back to a frequency in
	 * Hz.
	 */
	uint32_t phase_inc;
};

/*!
 * Get the oscillator frequency in Hertz.
 *
 * @param[in]		osc		Oscillator context.
 *
 * @returns		The oscillator frequency in Hertz.
 */
double sstvenc_osc_get_frequency(const struct sstvenc_oscillator* const osc);

/*!
 * Set the oscillator frequency in Hertz.
 *
 * @param[inout]	osc		Oscillator context being updated.
 * @param[in]		frequency	The frequency in Hertz.  This *MUST*
 * be at least 0Hz and less than the nyquist frequency (half the sample rate).
 */
void   sstvenc_osc_set_frequency(struct sstvenc_oscillator* const osc,
				 double				  frequency);

/*!
 * Initialise an oscillator with the given amplitude, frequency and phase
 * offset.  Use this when starting a new sinusoid.
 *
 * @param[inout]	osc		Oscillator context being initialised.
 *
 * @param[in]		amplitude	Starting oscillator amplitude
 * 					(range 0.0-1.0).
 *
 * @param[in]		frequency	Starting frequency of the oscillator
 * 					(range 0.0 - ½ of @a sample_rate).
 *
 * @param[in]		offset		Starting phase offset in radians.
 *
 * @param[in]		sample_rate	Output sample rate in hertz.
 */
void sstvenc_osc_init(struct sstvenc_oscillator* const osc, double amplitude,
		      double frequency, double offset, uint32_t sample_rate);

/*!
 * Compute the next sinusoid value and store it in the output field.
 * A no-op if the sample rate is set to zero.
 *
 * The next sample is made available in sstvenc_oscillator#output.
 *
 * @param[inout]	osc		Oscillator context being computed.
 */
void sstvenc_osc_compute(struct sstvenc_oscillator* const osc);

/*! @} */

#endif
