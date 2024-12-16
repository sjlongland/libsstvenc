/*!
 * @addtogroup sstv_mod
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/sstvfreq.h>
#include <libsstvenc/sstvmod.h>

void sstvenc_modulator_init(struct sstvenc_mod* const  mod,
			    const struct sstvenc_mode* mode,
			    const char* fsk_id, const uint8_t* framebuffer,
			    double rise_time, double fall_time,
			    uint32_t sample_rate, uint8_t time_unit) {
	/* Initialise the data structures */
	sstvenc_encoder_init(&(mod->enc), mode, fsk_id, framebuffer);
	sstvenc_osc_init(&(mod->osc), 1.0, SSTVENC_FREQ_SYNC, 0.0,
			 sample_rate);
	sstvenc_ps_init(&(mod->ps), 1.0, rise_time, INFINITY, fall_time,
			sample_rate, time_unit);

	/* Clear the state machine */
	mod->total_samples = 0;
	mod->total_ns	   = 0;
	mod->remaining	   = 0;
}

/*!
 * Compute the next sample whilst in the RISE phase of the pulse shaper state
 * machine.
 */
static void
sstvenc_modulator_next_rise_sample(struct sstvenc_mod* const mod) {
	sstvenc_ps_compute(&(mod->ps));
	mod->osc.amplitude = mod->ps.output;
	sstvenc_osc_compute(&(mod->osc));
}

/*!
 * Compute the next tone.
 */
static void sstvenc_modulator_next_tone(struct sstvenc_mod* const mod) {
	while (mod->enc.phase != SSTVENC_ENCODER_PHASE_DONE) {
		const struct sstvenc_encoder_pulse* pulse
		    = sstvenc_encoder_next_pulse(&(mod->enc));

		if (pulse) {
			/* Update the oscillator frequency */
			sstvenc_osc_set_frequency(&(mod->osc),
						  pulse->frequency);

			/* Figure out time duration in samples */
			mod->remaining = sstvenc_ts_unit_to_samples(
			    pulse->duration_ns, mod->osc.sample_rate,
			    SSTVENC_TS_UNIT_NANOSECONDS);

			/* Total up time and sample count */
			mod->total_samples += mod->remaining;
			mod->total_ns	   += pulse->duration_ns;

			/* Sanity check timing, adjust for any
			 * slippage */
			uint64_t expected_total_samples
			    = sstvenc_ts_unit_to_samples(
				mod->total_ns, mod->osc.sample_rate,
				SSTVENC_TS_UNIT_NANOSECONDS);
			if (expected_total_samples > mod->total_samples) {
				/*
				 * Rounding error has caused a slip,
				 * add samples to catch up.
				 */
				uint64_t diff = expected_total_samples
						- mod->total_samples;
				mod->remaining	   += diff;
				mod->total_samples += diff;
			}
			return;
		}
	}
}

/*!
 * Compute the next sample whilst in the HOLD phase of the pulse shaper state
 * machine.
 */
static void
sstvenc_modulator_next_hold_sample(struct sstvenc_mod* const mod) {
	sstvenc_ps_compute(&(mod->ps));
	mod->osc.amplitude = mod->ps.output;

	if (mod->enc.phase == SSTVENC_ENCODER_PHASE_DONE) {
		/* We're done */
		if (mod->ps.phase != SSTVENC_PS_PHASE_FALL) {
			sstvenc_ps_advance(&(mod->ps));
		}
		return;
	}

	if (mod->remaining == 0) {
		sstvenc_modulator_next_tone(mod);
	}

	if (mod->remaining > 0) {
		/* Compute the next oscillator output sample */
		sstvenc_osc_compute(&(mod->osc));
		mod->remaining--;
	}
}

/*!
 * Compute the next sample whilst in the FALL phase of the pulse shaper state
 * machine.
 */
static void
sstvenc_modulator_next_fall_sample(struct sstvenc_mod* const mod) {
	sstvenc_ps_compute(&(mod->ps));
	mod->osc.amplitude = mod->ps.output;
	sstvenc_osc_compute(&(mod->osc));
}

void sstvenc_modulator_compute(struct sstvenc_mod* const mod) {
	switch (mod->ps.phase) {
	case SSTVENC_PS_PHASE_INIT:
	case SSTVENC_PS_PHASE_RISE:
		/* Rising initial pulse */
		sstvenc_modulator_next_rise_sample(mod);
		break;
	case SSTVENC_PS_PHASE_HOLD:
		/* SSTV encoding state */
		sstvenc_modulator_next_hold_sample(mod);
		break;
	case SSTVENC_PS_PHASE_FALL:
		/* Falling final pulse */
		sstvenc_modulator_next_fall_sample(mod);
		break;
	default:
		/* We're done */
		mod->osc.output = 0.0;
		break;
	}
}

/*!
 * @}
 */
