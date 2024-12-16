/*!
 * @addtogroup pulseshape
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/oscillator.h>
#include <libsstvenc/pulseshape.h>

void sstvenc_ps_reset_samples(struct sstvenc_pulseshape* const ps,
			      uint32_t			       hold_time) {
	ps->phase      = SSTVENC_PS_PHASE_INIT;
	ps->hold_sz    = hold_time;
	ps->sample_idx = 0;
}

void sstvenc_ps_reset(struct sstvenc_pulseshape* const ps, double hold_time,
		      uint8_t time_unit) {
	uint32_t samples = sstvenc_ts_unit_to_samples(
	    hold_time, ps->sample_rate, time_unit);
	sstvenc_ps_reset_samples(ps, samples);
}

void sstvenc_ps_init(struct sstvenc_pulseshape* const ps, double amplitude,
		     double rise_time, double hold_time, double fall_time,
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

void sstvenc_ps_advance(struct sstvenc_pulseshape* const ps) {
	if (ps->phase < SSTVENC_PS_PHASE_DONE) {
		ps->sample_idx = 0;
		ps->phase++;
	}
}

void sstvenc_ps_compute(struct sstvenc_pulseshape* const ps) {
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

size_t sstvenc_psosc_fill_buffer(struct sstvenc_pulseshape* const ps,
				 struct sstvenc_oscillator* const osc,
				 double* buffer, size_t buffer_sz) {
	size_t written_sz = 0;

	while ((buffer_sz > 0) && (ps->phase < SSTVENC_PS_PHASE_DONE)) {
		sstvenc_ps_compute(ps);
		osc->amplitude = ps->output;

		sstvenc_osc_compute(osc);
		buffer[0] = osc->output;

		buffer++;
		buffer_sz--;

		written_sz++;
	}

	return written_sz;
}

/*! @} */
