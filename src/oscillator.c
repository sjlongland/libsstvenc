/*!
 * @addtogroup oscillator
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <libsstvenc/oscillator.h>
#include <math.h>
#include <stdint.h>

/*!
 * Fixed-point phase bit allocation.  For a uint32_t, this gives us a range
 * of 0-7.999 with a precision of ~1.86 nano-radians.
 */
#define SSTVENC_OSC_PHASE_FRAC_BITS (29)

/*!
 * Fixed-point scaling factor, computed from the number of bits.
 */
#define SSTVENC_OSC_PHASE_FRAC_SCALE                                         \
	((double)(1 << SSTVENC_OSC_PHASE_FRAC_BITS))

double sstvenc_osc_get_frequency(const struct sstvenc_oscillator* const osc) {
	return ((double)(((uint64_t)osc->phase_inc) * osc->sample_rate))
	       / (2 * M_PI * SSTVENC_OSC_PHASE_FRAC_SCALE);
}

void sstvenc_osc_set_frequency(struct sstvenc_oscillator* const osc,
			       double				frequency) {
	assert(frequency >= 0);
	assert(frequency < (osc->sample_rate / 2));

	osc->phase_inc = (2 * M_PI * frequency * SSTVENC_OSC_PHASE_FRAC_SCALE)
			 / osc->sample_rate;
}

void sstvenc_osc_init(struct sstvenc_oscillator* const osc, double amplitude,
		      double frequency, double offset, uint32_t sample_rate) {
	osc->amplitude	 = amplitude;
	osc->offset	 = offset;
	osc->output	 = 0.0;
	osc->sample_rate = sample_rate;
	osc->phase	 = 0;
	sstvenc_osc_set_frequency(osc, frequency);
}

void sstvenc_osc_compute(struct sstvenc_oscillator* const osc) {
	if (osc->sample_rate) {
		/* Compute output */
		osc->output = osc->amplitude
			      * sin(osc->offset
				    + (((double)osc->phase)
				       / SSTVENC_OSC_PHASE_FRAC_SCALE));

		/* Increment phase, modulo 2Pi */
		osc->phase += osc->phase_inc;
		osc->phase
		    %= (uint32_t)(2 * M_PI * SSTVENC_OSC_PHASE_FRAC_SCALE);
	}
}

/*! @} */
