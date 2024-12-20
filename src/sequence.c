/*!
 * @addtogroup sequence
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <libsstvenc/sequence.h>

void sstvenc_sequencer_step_set_timescale(
    struct sstvenc_sequencer_step* const step, uint8_t time_unit,
    _Bool convert) {
	step->type		= SSTVENC_SEQ_STEP_TYPE_SET_TS_UNIT;
	step->args.ts.time_unit = time_unit;
	step->args.ts.convert	= convert;
}

/*!
 * Configure a step that manipulates a register.
 * @param[out]		step		Sequencer step
 * @param[in]		type		Instruction type
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		New register value
 */
static void
sstvenc_sequencer_step_update_reg(struct sstvenc_sequencer_step* const step,
				  uint8_t type, uint8_t reg, double value) {
	step->type	     = type;
	step->args.reg.reg   = reg;
	step->args.reg.value = value;
}

void sstvenc_sequencer_step_set_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_SET_REGISTER, reg, value);
}

void sstvenc_sequencer_step_inc_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_INC_REGISTER, reg, value);
}

void sstvenc_sequencer_step_dec_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_DEC_REGISTER, reg, value);
}

void sstvenc_sequencer_step_mul_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_MUL_REGISTER, reg, value);
}

void sstvenc_sequencer_step_div_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_DIV_REGISTER, reg, value);
}

void sstvenc_sequencer_step_idec_reg(
    struct sstvenc_sequencer_step* const step, uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_IDEC_REGISTER, reg, value);
}

void sstvenc_sequencer_step_idiv_reg(
    struct sstvenc_sequencer_step* const step, uint8_t reg, double value) {
	sstvenc_sequencer_step_update_reg(
	    step, SSTVENC_SEQ_STEP_TYPE_IDIV_REGISTER, reg, value);
}

/*!
 * Configure a sequencer step that sets up an operation for a given duration.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		duration	Operation duration
 */
static void
sstvenc_sequencer_step_duration(struct sstvenc_sequencer_step* const step,
				uint8_t type, double duration) {
	step->type		     = type;
	step->args.duration.duration = duration;
}

void sstvenc_sequencer_step_silence(struct sstvenc_sequencer_step* const step,
				    double duration) {
	sstvenc_sequencer_step_duration(
	    step, SSTVENC_SEQ_STEP_TYPE_EMIT_SILENCE, duration);
}

void sstvenc_sequencer_step_tone(struct sstvenc_sequencer_step* const step,
				 double duration, uint8_t slopes) {
	sstvenc_sequencer_step_duration(step, SSTVENC_SEQ_STEP_TYPE_EMIT_TONE,
					duration);
	step->args.duration.slopes = slopes;
}

void sstvenc_sequencer_step_cw(struct sstvenc_sequencer_step* const step,
			       const char*			    text) {
	step->type	   = SSTVENC_SEQ_STEP_TYPE_EMIT_CW;
	step->args.cw.text = text;
}

void sstvenc_sequencer_step_image(struct sstvenc_sequencer_step* const step,
				  const struct sstvenc_mode* const     mode,
				  const uint8_t* framebuffer,
				  const char*	 fsk_id) {
	step->type		     = SSTVENC_SEQ_STEP_TYPE_EMIT_IMAGE;
	step->args.image.mode	     = mode;
	step->args.image.framebuffer = framebuffer;
	step->args.image.fsk_id	     = fsk_id;
}

void sstvenc_sequencer_step_audio(
    struct sstvenc_sequencer_step* const  step,
    struct sstvenc_sequencer_ausrc* const ausrc) {
	step->type	     = SSTVENC_SEQ_STEP_TYPE_EMIT_AUDIO;
	step->args.audio.src = ausrc;
}

void sstvenc_sequencer_step_end(struct sstvenc_sequencer_step* const step) {
	step->type = SSTVENC_SEQ_STEP_TYPE_END;
}

static void
sstvenc_sequencer_reset_internal(struct sstvenc_sequencer* const seq) {
	seq->step			      = 0;
	seq->state			      = SSTVENC_SEQ_STATE_INIT;
	seq->regs[SSTVENC_SEQ_REG_AMPLITUDE]  = 1.0;
	seq->regs[SSTVENC_SEQ_REG_FREQUENCY]  = 800.0;
	seq->regs[SSTVENC_SEQ_REG_PHASE]      = 0.0;
	seq->regs[SSTVENC_SEQ_REG_PULSE_RISE] = 0.002;
	seq->regs[SSTVENC_SEQ_REG_PULSE_FALL] = 0.002;
	seq->regs[SSTVENC_SEQ_REG_DIT_PERIOD] = 0.05;
	seq->time_unit			      = SSTVENC_TS_UNIT_SECONDS;
}

void sstvenc_sequencer_init(struct sstvenc_sequencer* const	 seq,
			    const struct sstvenc_sequencer_step* steps,
			    sstvenc_sequencer_event_cb*		 event_cb,
			    const void* event_cb_ctx, uint32_t sample_rate) {
	seq->err	  = 0;
	seq->steps	  = steps;
	seq->event_cb	  = event_cb;
	seq->event_cb_ctx = event_cb_ctx;
	sstvenc_sequencer_reset_internal(seq);
}

/*!
 * Advance to the next step in the sequence.  Optionally call the callback
 * routine if it is defined.
 */
static void sstvenc_sequencer_next_step(struct sstvenc_sequencer* const seq,
					_Bool notify) {
	seq->step++;

	if (notify && seq->event_cb) {
		seq->event_cb(seq);
	}
}

/*!
 * Enter a new state in the state machine.  Optionally call the callback
 * routine if it is defined.
 */
static void sstvenc_sequencer_next_state(struct sstvenc_sequencer* const seq,
					 uint8_t state, _Bool notify) {
	if (seq->state != state) {
		seq->state = state;

		if (notify && seq->event_cb) {
			seq->event_cb(seq);
		}
	}
}

/*!
 * Abort the state machine with an error.
 */
static void sstvenc_sequencer_abort(struct sstvenc_sequencer* const seq,
				    int				    err) {
	seq->state = SSTVENC_SEQ_STATE_DONE;
	seq->err   = err;

	if (seq->event_cb) {
		seq->event_cb(seq);
	}
}

/*!
 * Execute a SET_TS instruction.
 */
static void sstvenc_sequencer_exec_set_ts(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	/* Perform conversions if asked */
	if (step->args.ts.convert) {
		const uint64_t old_scale
		    = sstvenc_ts_unit_scale(seq->time_unit);
		const uint64_t new_scale
		    = sstvenc_ts_unit_scale(step->args.ts.time_unit);
		double scale = (double)new_scale / (double)old_scale;

		for (uint8_t reg = 0; reg < SSTVENC_SEQ_NUM_REGS; reg++) {
			seq->regs[reg] *= scale;
		}
	}

	/* Apply new unit setting */
	seq->time_unit = step->args.ts.time_unit;

	/* Step is complete */
	sstvenc_sequencer_next_step(seq, true);
}

/*!
 * Execute a register manipulation instruction.
 */
static void sstvenc_sequencer_exec_update_reg(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	if (step->args.reg.reg < SSTVENC_SEQ_NUM_REGS) {
		/* Get a pointer to the register for convenience */
		double* const value = &(seq->regs[step->args.reg.reg]);

		switch (step->type) {
		case SSTVENC_SEQ_STEP_TYPE_SET_REGISTER:
			*value = step->args.reg.value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_INC_REGISTER:
			*value += step->args.reg.value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_DEC_REGISTER:
			*value -= step->args.reg.value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_MUL_REGISTER:
			*value *= step->args.reg.value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_DIV_REGISTER:
			*value /= step->args.reg.value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_IDEC_REGISTER:
			*value = step->args.reg.value - *value;
			break;
		case SSTVENC_SEQ_STEP_TYPE_IDIV_REGISTER:
			*value = step->args.reg.value / *value;
			break;
		}
	}

	/* Step is complete */
	sstvenc_sequencer_next_step(seq, true);
}

/*!
 * Initialise the state machine for a run of silence.
 */
static void sstvenc_sequencer_begin_silence(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_BEGIN_SILENCE,
				     true);

	if (step->args.duration.duration == INFINITY) {
		sstvenc_sequencer_next_state(
		    seq, SSTVENC_SEQ_STATE_GEN_INF_SILENCE, true);
	} else {
		seq->vars.silence.remaining = sstvenc_ts_unit_to_samples(
		    step->args.duration.duration, seq->sample_rate,
		    seq->time_unit);
		sstvenc_sequencer_next_state(
		    seq, SSTVENC_SEQ_STATE_GEN_SILENCE, true);
	}
}

static void sstvenc_sequencer_begin_tone(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	_Bool init_osc = seq->state != SSTVENC_SEQ_STATE_END_TONE;
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_BEGIN_SILENCE,
				     true);

	sstvenc_ps_init(
	    &(seq->vars.tone.ps), seq->regs[SSTVENC_SEQ_REG_AMPLITUDE],
	    (step->args.duration.slopes & SSTVENC_SEQ_SLOPE_RISING)
		? seq->regs[SSTVENC_SEQ_REG_PULSE_RISE]
		: 0.0,
	    step->args.duration.duration,
	    (step->args.duration.slopes & SSTVENC_SEQ_SLOPE_FALLING)
		? seq->regs[SSTVENC_SEQ_REG_PULSE_FALL]
		: 0.0,
	    seq->sample_rate, seq->time_unit);

	if (init_osc) {
		sstvenc_osc_init(&(seq->vars.tone.osc), 0.0,
				 seq->regs[SSTVENC_SEQ_REG_FREQUENCY],
				 seq->regs[SSTVENC_SEQ_REG_PHASE],
				 seq->sample_rate);
	} else {
		sstvenc_osc_set_frequency(
		    &(seq->vars.tone.osc),
		    seq->regs[SSTVENC_SEQ_REG_FREQUENCY]);
		seq->vars.tone.osc.offset = seq->regs[SSTVENC_SEQ_REG_PHASE];
	}

	if (step->args.duration.duration == INFINITY) {
		sstvenc_sequencer_next_state(
		    seq, SSTVENC_SEQ_STATE_GEN_INF_TONE, true);
	} else {
		sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_GEN_TONE,
					     true);
	}
}

static void
sstvenc_sequencer_begin_cw(struct sstvenc_sequencer* const	      seq,
			   const struct sstvenc_sequencer_step* const step) {
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_BEGIN_CW, true);

	sstvenc_cw_init(&(seq->vars.cw), step->args.cw.text,
			seq->regs[SSTVENC_SEQ_REG_AMPLITUDE],
			seq->regs[SSTVENC_SEQ_REG_FREQUENCY],
			seq->regs[SSTVENC_SEQ_REG_DIT_PERIOD],
			seq->regs[SSTVENC_SEQ_REG_PULSE_RISE],
			seq->sample_rate, seq->time_unit);

	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_GEN_CW, true);
}

static void sstvenc_sequencer_begin_image(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_BEGIN_IMAGE,
				     true);

	sstvenc_modulator_init(&(seq->vars.sstv), step->args.image.mode,
			       step->args.image.fsk_id,
			       step->args.image.framebuffer,
			       seq->regs[SSTVENC_SEQ_REG_DIT_PERIOD],
			       seq->regs[SSTVENC_SEQ_REG_PULSE_RISE],
			       seq->sample_rate, seq->time_unit);
	seq->vars.sstv.ps.amplitude = seq->regs[SSTVENC_SEQ_REG_AMPLITUDE];

	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_GEN_IMAGE, true);
}

static void sstvenc_sequencer_begin_audio(
    struct sstvenc_sequencer* const	       seq,
    const struct sstvenc_sequencer_step* const step) {
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_BEGIN_AUDIO,
				     true);

	assert(step->args.audio.src->iface != NULL);

	if (step->args.audio.src->iface->init) {
		int res
		    = step->args.audio.src->iface->init(step->args.audio.src);
		if (res < 0) {
			sstvenc_sequencer_abort(seq, -res);
			return;
		}
	}

	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_GEN_AUDIO, true);
}

/*!
 * Read the next audio sample from the audio source.
 */
static void
sstvenc_sequencer_next_ausrc_sample(struct sstvenc_sequencer* const seq) {
	const struct sstvenc_sequencer_step* step = &(seq->steps[seq->step]);
	int				     res;
	assert(step->args.audio.src->iface != NULL);
	assert(step->args.audio.src->iface->next != NULL);

	res = step->args.audio.src->iface->next(step->args.audio.src,
						&(seq->output));
	if (res < 0) {
		/* Read failed */
		sstvenc_sequencer_abort(seq, -res);
	} else if (res == 0) {
		/* We are finished reading, close the audio source */
		if (step->args.audio.src->iface->close) {
			res = step->args.audio.src->iface->close(
			    step->args.audio.src);
			if (res < 0) {
				sstvenc_sequencer_abort(seq, -res);
			}
		}
	}
}

static void sstvenc_sequencer_end(struct sstvenc_sequencer* const seq) {
	sstvenc_sequencer_next_state(seq, SSTVENC_SEQ_STATE_DONE, true);
}

/*!
 * Execute the step pointed to by sstvenc_sequencer#step.
 */
static void sstvenc_sequencer_exec_step(struct sstvenc_sequencer* const seq) {
	const struct sstvenc_sequencer_step* step = &(seq->steps[seq->step]);
	switch (step->type) {
	case SSTVENC_SEQ_STEP_TYPE_SET_TS_UNIT:
		sstvenc_sequencer_exec_set_ts(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_SET_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_INC_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_DEC_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_MUL_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_DIV_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_IDEC_REGISTER:
	case SSTVENC_SEQ_STEP_TYPE_IDIV_REGISTER:
		sstvenc_sequencer_exec_update_reg(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_EMIT_SILENCE:
		sstvenc_sequencer_begin_silence(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_EMIT_TONE:
		sstvenc_sequencer_begin_tone(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_EMIT_CW:
		sstvenc_sequencer_begin_cw(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_EMIT_IMAGE:
		sstvenc_sequencer_begin_image(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_EMIT_AUDIO:
		sstvenc_sequencer_begin_audio(seq, step);
		break;
	case SSTVENC_SEQ_STEP_TYPE_END:
	default:
		sstvenc_sequencer_end(seq);
		break;
	}
}

static int
sstvenc_sequencer_reset_ausrc(struct sstvenc_sequencer_ausrc* ausrc) {
	int reset_res = 0;
	int close_res = 0;

	assert(ausrc->iface != NULL);
	if (ausrc->iface->reset) {
		reset_res = ausrc->iface->reset(ausrc);
		if (reset_res == 0) {
			return 0;
		}
	}

	/* Still here?  Then resetting failed or isn't implemented */

	if (ausrc->iface->close) {
		close_res = ausrc->iface->close(ausrc);
		if (close_res == 0) {
			/* Close worked, leave it at that. */
			return 0;
		}
	}

	/* We could not reset or close */
	if (reset_res) {
		return reset_res;
	} else {
		return close_res;
	}
}

void sstvenc_sequencer_reset(struct sstvenc_sequencer* const seq) {
	/* Assume everything prior to seq->step has been manipulated */
	uint16_t failed_idx = UINT16_MAX;
	int	 failed_err = 0;
	int	 reset_res  = 0;

	for (uint16_t idx = 0; idx < seq->step; idx++) {
		const struct sstvenc_sequencer_step* step
		    = &(seq->steps[idx]);
		switch (step->type) {
		case SSTVENC_SEQ_STEP_TYPE_EMIT_AUDIO:
			reset_res = sstvenc_sequencer_reset_ausrc(
			    step->args.audio.src);
			if (reset_res < 0) {
				/*
				 * Reset failed!  Make a note of where
				 * if we haven't already done so.
				 */
				if (failed_idx == UINT16_MAX) {
					failed_idx = idx;
					failed_err = -reset_res;
				}
			}
			break;
		default:
			/* Nothing to do */
			break;
		}
	}

	if (failed_idx != UINT16_MAX) {
		/*
		 * We were unable to reset at least one step.  Report the
		 * first one we saw here.
		 */
		seq->step = failed_idx;
		sstvenc_sequencer_abort(seq, failed_err);
	} else {
		/* All clear, reset the sequencer itself */
		sstvenc_sequencer_reset_internal(seq);
	}
}

void sstvenc_sequencer_advance(struct sstvenc_sequencer* const seq) {
	switch (seq->state) {
	case SSTVENC_SEQ_STATE_GEN_INF_SILENCE:
		sstvenc_sequencer_next_state(
		    seq, SSTVENC_SEQ_STATE_END_SILENCE, false);
		sstvenc_sequencer_next_step(seq, true);
		break;
	case SSTVENC_SEQ_STATE_GEN_INF_TONE:
		sstvenc_ps_advance(&(seq->vars.tone.ps));
		break;
	default:
		return;
	}
}

void sstvenc_sequencer_compute(struct sstvenc_sequencer* const seq) {
retry:
	switch (seq->state) {
	case SSTVENC_SEQ_STATE_INIT:
	case SSTVENC_SEQ_STATE_END_SILENCE:
	case SSTVENC_SEQ_STATE_END_TONE:
	case SSTVENC_SEQ_STATE_END_CW:
	case SSTVENC_SEQ_STATE_END_IMAGE:
	case SSTVENC_SEQ_STATE_END_AUDIO:
		sstvenc_sequencer_exec_step(seq);
		goto retry;
		break;
	case SSTVENC_SEQ_STATE_BEGIN_SILENCE:
	case SSTVENC_SEQ_STATE_GEN_SILENCE:
	case SSTVENC_SEQ_STATE_GEN_INF_SILENCE:
		seq->output = 0.0;
		if (seq->vars.silence.remaining > 0) {
			seq->vars.silence.remaining--;
		} else {
			sstvenc_sequencer_next_state(
			    seq, SSTVENC_SEQ_STATE_END_SILENCE, true);
			goto retry;
		}
		break;
	case SSTVENC_SEQ_STATE_BEGIN_TONE:
	case SSTVENC_SEQ_STATE_GEN_TONE:
	case SSTVENC_SEQ_STATE_GEN_INF_TONE:
		sstvenc_ps_compute(&(seq->vars.tone.ps));
		seq->vars.tone.osc.amplitude = seq->vars.tone.ps.output;
		sstvenc_osc_compute(&(seq->vars.tone.osc));
		seq->output = seq->vars.tone.osc.output;

		if (seq->vars.tone.ps.phase >= SSTVENC_PS_PHASE_DONE) {
			sstvenc_sequencer_next_state(
			    seq, SSTVENC_SEQ_STATE_END_TONE, true);
			goto retry;
		}
		break;
	case SSTVENC_SEQ_STATE_BEGIN_CW:
	case SSTVENC_SEQ_STATE_GEN_CW:
		sstvenc_cw_compute(&(seq->vars.cw));
		seq->output = seq->vars.cw.output;

		if (seq->vars.cw.state >= SSTVENC_CW_MOD_STATE_DONE) {
			sstvenc_sequencer_next_state(
			    seq, SSTVENC_SEQ_STATE_END_CW, true);
			goto retry;
		}
		break;
	case SSTVENC_SEQ_STATE_BEGIN_IMAGE:
	case SSTVENC_SEQ_STATE_GEN_IMAGE:
		sstvenc_modulator_compute(&(seq->vars.sstv));
		seq->output = seq->vars.sstv.osc.output;

		if (seq->vars.sstv.ps.phase >= SSTVENC_PS_PHASE_DONE) {
			sstvenc_sequencer_next_state(
			    seq, SSTVENC_SEQ_STATE_END_IMAGE, true);
			goto retry;
		}
		break;
	case SSTVENC_SEQ_STATE_BEGIN_AUDIO:
	case SSTVENC_SEQ_STATE_GEN_AUDIO:
		sstvenc_sequencer_next_ausrc_sample(seq);
		break;
	case SSTVENC_SEQ_STATE_DONE:
	default:
		break;
	}
}

size_t sstvenc_sequencer_fill_buffer(struct sstvenc_sequencer* const seq,
				     double* buffer, size_t buffer_sz) {
	size_t written_sz = 0;

	while ((buffer_sz > 0) && (seq->state < SSTVENC_SEQ_STATE_DONE)) {
		sstvenc_sequencer_compute(seq);

		buffer[0] = seq->output;
		buffer++;
		buffer_sz--;

		written_sz++;
	}

	return written_sz;
}
