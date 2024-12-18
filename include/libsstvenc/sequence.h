#ifndef _SSTVENC_SEQUENCE_H
#define _SSTVENC_SEQUENCE_H

/*!
 * @defgroup sequence Transmission Sequencer
 * @{
 *
 * This module aids in combining a SSTV (or in fact, multiple if desired) with
 * arbitrary tone generation and the CW modulator to enable adding of CW IDs
 * to meet regulatory requirements and to enable triggering of VOX circuits or
 * SSTV repeaters.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/cw.h>
#include <libsstvenc/sstvmod.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*!
 * @defgroup sequence_states Transmission sequencer states
 * @{
 *
 * The sequencer state records information on which data structure is
 * currently active and thus provides a hint to the state machine as to when a
 * re-initialisation is necessary.
 */

/*!
 * Sequencer is in the initial state.  No meaningful audio samples will be
 * generated at this point.  The first step of the sequence is loaded and
 * the state machine should enter one of these states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_INIT		    (0x00)

/*!
 * Sequencer is about to begin emitting silence.  A duration in samples will
 * be computed and that amount counted out.  Next valid state is
 * @ref SSTVENC_SEQ_STATE_GEN_SILENCE if the duration is finite, otherwise
 * we go to state @ref SSTVENC_SEQ_STATE_GEN_INF_SILENCE.
 */
#define SSTVENC_SEQ_STATE_BEGIN_SILENCE	    (0x10)

/*!
 * Sequencer is emitting silence.  Zero samples are being transmitted at this
 * point.  We exit this state when the state counter reaches zero, at which
 * point we enter state @ref SSTVENC_SEQ_STATE_END_SILENCE.
 */
#define SSTVENC_SEQ_STATE_GEN_SILENCE	    (0x17)

/*!
 * Sequencer is emitting silence for an indefinite time period.  Zero samples
 * are being transmitted at this point.  We exit this state when an external
 * signal triggers us to move to state @ref SSTVENC_SEQ_STATE_END_SILENCE.
 */
#define SSTVENC_SEQ_STATE_GEN_INF_SILENCE   (0x18)

/*!
 * Sequencer has finished emitting silence, the counter is now zero.  Possible
 * follow-on states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE (again)
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_END_SILENCE	    (0x1f)

/*!
 * Sequencer is about to begin emitting tones.  Initialise the oscillator and
 * pulse shaper.  The pulse shaper will drive the oscillator's amplitude and
 * the oscillator's output will be the sequencer output.  Follow-on state is
 * @ref SSTVENC_SEQ_STATE_GEN_TONE or @ref SSTVENC_SEQ_STATE_GEN_INF_TONE.
 */
#define SSTVENC_SEQ_STATE_BEGIN_TONE	    (0x20)

/*!
 * Sequencer is emitting a tone.  Samples should be read from the oscillator
 * output which will be modulated by the pulse shaper.  We exit this state
 * when the pulse shaper state machine reaches the "DONE" state, at which
 * point we enter state @ref SSTVENC_SEQ_STATE_END_TONE.
 */
#define SSTVENC_SEQ_STATE_GEN_TONE	    (0x27)

/*!
 * Sequencer is emitting a tone for an indefinite period of time.  Samples
 * should be read from the oscillator output which will be modulated by the
 * pulse shaper.  We exit this state when the pulse shaper state machine
 * reaches the "DONE" state, at which point we enter state @ref
 * SSTVENC_SEQ_STATE_END_TONE.
 */
#define SSTVENC_SEQ_STATE_GEN_INF_TONE	    (0x28)

/*!
 * Sequencer has finished emitting the tone.  Possible follow-on states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE (again)
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_END_TONE	    (0x2f)

/*!
 * Sequencer is about to begin emitting CW text.  The CW state machine will be
 * initialised with the new configuration.  Follow-on state is
 * @ref SSTVENC_SEQ_STATE_GEN_CW.
 */
#define SSTVENC_SEQ_STATE_BEGIN_CW	    (0x30)

/*!
 * Sequencer is emitting CW text.  We exit this state when the CW state
 * machine reaches the "DONE" state, at which point we transition to the
 * @ref SSTVENC_SEQ_STATE_END_CW state.
 */
#define SSTVENC_SEQ_STATE_GEN_CW	    (0x37)

/*!
 * Sequencer has finished sending CW.  Possible follow-on states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW (again)
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_END_CW	    (0x3f)

/*!
 * Sequencer is about to begin emitting a SSTV image.  The SSTV modulator
 * state machine will be configured with the framebuffer and mode specified.
 * Follow-on state is @ref SSTVENC_SEQ_STATE_GEN_IMAGE.
 */
#define SSTVENC_SEQ_STATE_BEGIN_IMAGE	    (0x40)

/*!
 * Sequencer is emitting the SSTV image.  We remain in this state until the
 * SSTV modulator has completed its transmission, at which point we transition
 * to the @ref SSTVENC_SEQ_STATE_END_IMAGE state.
 */
#define SSTVENC_SEQ_STATE_GEN_IMAGE	    (0x47)

/*!
 * SSTV transmission has completed.  Follow-on states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE (again)
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_END_IMAGE	    (0x4f)

/*!
 * Sequencer is about to begin emitting an audio recording.  Follow-on state
 * is @ref SSTVENC_SEQ_STATE_GEN_AUDIO unless initialisation fails, at which
 * point the sequencer will jump to state @ref SSTVENC_SEQ_STATE_DONE and
 * set sstvenc_sequencer#err.
 */
#define SSTVENC_SEQ_STATE_BEGIN_AUDIO	    (0xe0)

/*!
 * Sequencer is emitting the audio recording.  We remain in this state until
 * we run out of samples from the decoder, at which point we transition
 * to the @ref SSTVENC_SEQ_STATE_END_AUDIO state.
 */
#define SSTVENC_SEQ_STATE_GEN_AUDIO	    (0xe7)

/*!
 * Audio transmission has completed.  Follow-on states:
 *
 * - @ref SSTVENC_SEQ_STATE_BEGIN_SILENCE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_TONE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_CW
 * - @ref SSTVENC_SEQ_STATE_BEGIN_IMAGE
 * - @ref SSTVENC_SEQ_STATE_BEGIN_AUDIO (again)
 * - @ref SSTVENC_SEQ_STATE_DONE
 */
#define SSTVENC_SEQ_STATE_END_AUDIO	    (0xef)

/*!
 * The sequence is complete.  No more samples are provided.
 * If the sequence finished with an error, the error code is given by
 * sstvenc_sequencer#err.
 */
#define SSTVENC_SEQ_STATE_DONE		    (0xff)

/*!
 * @}
 */

/*!
 * @defgroup sequence_step_type Sequencer step types.
 * @{
 *
 * A sequencer step represents a simple instruction.  The sequence of
 * instructions is executed in order, with no branching or looping from start
 * to finish.  Each step is a unioned data structure, the step type defines
 * which union member stores the parameter for the current step.
 */

/*!
 * End of sequence step.  This is used to indicate the end of the sequence.
 * No parameters are used.
 */
#define SSTVENC_SEQ_STEP_TYPE_END	    (0x00)

/*!
 * Set the time scale unit.
 */
#define SSTVENC_SEQ_STEP_TYPE_SET_TS_UNIT   (0x10)

/*!
 * Set a register value to an absolute value.  (i.e. `reg = value`)
 */
#define SSTVENC_SEQ_STEP_TYPE_SET_REGISTER  (0x20)

/*!
 * Increment a register value by the given value.  (i.e. `reg = reg + value`)
 */
#define SSTVENC_SEQ_STEP_TYPE_INC_REGISTER  (0x22)

/*!
 * Decrement a register value by the given value.  (i.e. `reg = reg - value`)
 */
#define SSTVENC_SEQ_STEP_TYPE_DEC_REGISTER  (0x23)

/*!
 * Multiply a register value by the given value.  (i.e. `reg = reg * value`)
 */
#define SSTVENC_SEQ_STEP_TYPE_MUL_REGISTER  (0x24)

/*!
 * Divide a register value by the given value.  (i.e. `reg = reg / value`)
 */
#define SSTVENC_SEQ_STEP_TYPE_DIV_REGISTER  (0x25)

/*!
 * Decrement a register value from the given value.  (i.e. `reg = value -
 * reg`)
 */
#define SSTVENC_SEQ_STEP_TYPE_IDEC_REGISTER (0x2b)

/*!
 * Divide a register value into the given value.  (i.e. `reg = value / reg`)
 */
#define SSTVENC_SEQ_STEP_TYPE_IDIV_REGISTER (0x2d)

/*!
 * Emit silence
 */
#define SSTVENC_SEQ_STEP_TYPE_EMIT_SILENCE  (0x30)

/*!
 * Emit tone
 */
#define SSTVENC_SEQ_STEP_TYPE_EMIT_TONE	    (0x40)

/*!
 * Emit CW text
 */
#define SSTVENC_SEQ_STEP_TYPE_EMIT_CW	    (0x50)

/*!
 * Emit image
 */
#define SSTVENC_SEQ_STEP_TYPE_EMIT_IMAGE    (0x60)

/*!
 * Emit audio recording
 */
#define SSTVENC_SEQ_STEP_TYPE_EMIT_AUDIO    (0x70)

/*!
 * @}
 */

/*!
 * @defgroup sequence_regs Sequencer registers.
 * @{
 */

/*!
 * Carrier Amplitude [0.0, 1.0]
 */
#define SSTVENC_SEQ_REG_AMPLITUDE	    (0)

/*!
 * Carrier Frequency in Hertz [0.0, ½ of sstvenc_sequencer#sample_rate]
 */
#define SSTVENC_SEQ_REG_FREQUENCY	    (1)

/*!
 * Carrier Phase Offset in radians
 */
#define SSTVENC_SEQ_REG_PHASE		    (2)

/*!
 * Pulse Rise Time, unit determined by sstvenc_sequencer#time_unit.
 */
#define SSTVENC_SEQ_REG_PULSE_RISE	    (3)

/*!
 * Pulse Fall Time, unit determined by sstvenc_sequencer#time_unit.
 */
#define SSTVENC_SEQ_REG_PULSE_FALL	    (4)

/*!
 * CW Dit Period, unit determined by sstvenc_sequencer#time_unit.
 */
#define SSTVENC_SEQ_REG_DIT_PERIOD	    (5)

/*!
 * Total number of registers.
 */
#define SSTVENC_SEQ_NUM_REGS		    (5)

/*!
 * @}
 */

/*!
 * @defgroup sequence_toneslopes Tone slopes
 * @{
 * This bitmap decides whether we enable the rising or falling edges of the
 * pulse shaper. Selectively turning these on and off allows us to "join"
 * multiple tones together gapless.
 */

#define SSTVENC_SEQ_SLOPE_NONE		    (0) /*!< No slopes, rising or falling */
#define SSTVENC_SEQ_SLOPE_RISING	    (1) /*!< Rising slope only */
#define SSTVENC_SEQ_SLOPE_FALLING	    (2) /*!< Falling slope only */
#define SSTVENC_SEQ_SLOPE_BOTH		    (3) /*!< Both slopes, rising and falling */

/*!
 * @}
 */

struct sstvenc_sequencer;
struct sstvenc_sequencer_step;
struct sstvenc_sequencer_ausrc;

/*!
 * Callback routine for sequencer events.  This is called at the start of each
 * sequencer step, and finally at the end.  The callback should not block the
 * state machine for lengthy periods as this will cause delays in the
 * transmission.
 */
typedef void sstvenc_sequencer_event_cb(struct sstvenc_sequencer* const enc);

/*!
 * SSTV transmission sequencer data structure.  This stores a record of the
 * sequence being transmitted and the current step.
 */
struct sstvenc_sequencer {
	/*! The list of sequence steps to be carried out. */
	const struct sstvenc_sequencer_step* steps;

	/*! Event call-back, called on each state transition */
	sstvenc_sequencer_event_cb*	     event_cb;

	/*! Optional event callback context */
	const void*			     event_cb_ctx;

	/*! Output sample */
	double				     output;

	/*! Sequencer state machine variables */
	union sstvenc_sequencer_vars {
		/*!
		 * State machine logic for the "silence" states.
		 */
		struct sstvenc_sequencer_vars_silence {
			/*! Number of samples remaining of silence */
			uint32_t remaining;
		} silence;

		/*!
		 * State machine for tone states.
		 */
		struct sstvenc_sequencer_vars_tone {
			/*! Oscillator state machine */
			struct sstvenc_oscillator osc;
			/*! Pulse Shaper state machine */
			struct sstvenc_pulseshape ps;
		} tone;

		/*!
		 * State machine for CW transmission states.
		 */
		struct sstvenc_cw_mod cw;

		/*!
		 * State machine for SSTV transmission states.
		 */
		struct sstvenc_mod    sstv;
	} vars;

	/*!
	 * SSTV sequencer register parameters.  The indices are
	 */
	double	 regs[SSTVENC_SEQ_NUM_REGS];

	/*! Sample rate in hertz. */
	uint32_t sample_rate;

	/*!
	 * Error status when the state machine finishes in a failure state,
	 * the values are taken from `errno.h`.
	 */
	int	 err;

	/*!
	 * The current step being executed.  This is an index into
	 * sstvenc_sequencer#steps.
	 */
	uint16_t step;

	/*! Time scale unit of measure */
	uint8_t	 time_unit;

	/*! Sequencer state machine state, see @ref sequence_states */
	uint8_t	 state;
};

/*!
 * A sequencer step is a single instruction.  Some instructions alter the
 * current state of the state machine, others just adjust parameters.
 */
struct sstvenc_sequencer_step {
	union sstvenc_sequence_step_args {
		/*!
		 * Setting of the time-scale unit. sstvenc_sequencer_step#type
		 * is set to @ref SSTVENC_SEQ_STEP_TYPE_SET_TS_UNIT.
		 */
		struct sstvenc_sequence_step_set_ts_unit {
			/*! The new time-scale unit */
			uint8_t time_unit;
			/*! Convert fields to the new unit? */
			_Bool	convert;
		} ts;

		/*!
		 * Set a register to a new value.  sstvenc_sequencer_step#type
		 * is set to one of:
		 *
		 * - @ref SSTVENC_SEQ_STEP_TYPE_SET_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_DEC_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_INC_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_MUL_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_DIV_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_IDIV_REGISTER
		 * - @ref SSTVENC_SEQ_STEP_TYPE_IDEC_REGISTER
		 */
		struct sstvenc_sequence_step_set_reg {
			/*! The new register value / operand value */
			double	value;
			/*! The register being updated */
			uint8_t reg;
		} reg;

		/*!
		 * Emit silence or a tone for the specified time period.
		 * sstvenc_sequencer_step#type is set to one of:
		 *
		 * - @ref SSTVENC_SEQ_STEP_TYPE_EMIT_SILENCE
		 * - @ref SSTVENC_SEQ_STEP_TYPE_EMIT_TONE
		 */
		struct sstvenc_sequence_step_duration {
			/*!
			 * The number of time units (set by
			 * sstvenc_sequencer_regs#time_unit) that we should
			 * be emitting silence or a tone for.  Set to INFINITY
			 * for an infinite time period.
			 */
			double	duration;
			/*!
			 * Slopes enabled.  Whether or not we enable the
			 * rising or falling slopes on the pulse.  Not used
			 * for silences.
			 */
			uint8_t slopes;
		} duration;

		/*!
		 * Send a CW message.  sstvenc_sequencer_step#type is set to
		 * @ref SSTVENC_SEQ_STEP_TYPE_EMIT_CW.
		 */
		struct sstvenc_sequence_step_cw {
			/*! The CW string to emit */
			const char* text;
		} cw;

		/*!
		 * Send a SSTV image.  sstvenc_sequencer_step#type is set to
		 * @ref SSTVENC_SEQ_STEP_TYPE_EMIT_IMAGE.
		 */
		struct sstvenc_sequence_step_image {
			/*! The SSTV image mode to use for transmission */
			const struct sstvenc_mode* mode;
			/*! The image framebuffer */
			const uint8_t*		   framebuffer;
			/*! The FSK ID */
			const char*		   fsk_id;
		} image;

		/*!
		 * Send an audio recording.  sstvenc_sequencer_step#type is
		 * set to
		 * @ref SSTVENC_SEQ_STEP_TYPE_EMIT_AUDIO.
		 */
		struct sstvenc_sequence_step_audio {
			/*! The audio source emitting the samples */
			struct sstvenc_sequencer_ausrc* src;
		} audio;

	} args;

	/*!
	 * The type of sequencer step.  See @ref sequence_step_type
	 */
	uint8_t type;
};

/*!
 * Sequencer audio source interface.  This defines the callback functions
 * necessary for reading audio from a particular source type or to reset the
 * source's state.
 */
struct sstvenc_sequencer_ausrc_interface {
	/*!
	 * Initialise the audio source ready for reading samples.  The
	 * function should assume the existing state of the context is
	 * meaningless.
	 *
	 * @param[inout]	ausrc	Audio source context
	 *
	 * @retval		0	Success
	 * @retval		<0	An error from `errno.h`, negated.
	 */
	int (*init)(struct sstvenc_sequencer_ausrc* const ausrc);

	/*!
	 * Reset the audio source back to the initial state.  The function may
	 * assume the structure has been initialised already.  Resources can
	 * be released if this would be easier than resetting their states.
	 *
	 * In the event of an error, it is the responsibility of the audio
	 * interface to clean up its state and release resources.
	 *
	 * @param[inout]	ausrc	Audio source context
	 *
	 * @retval		0	Success
	 * @retval		<0	An error from `errno.h`, negated.
	 */
	int (*reset)(struct sstvenc_sequencer_ausrc* const ausrc);

	/*!
	 * Read and return the next audio sample.  The function may assume the
	 * context has been initialised already (that is,
	 * sstvenc_sequencer_ausrc_interface#init has been called).
	 *
	 * When we reach the end, the audio source interface is responsible
	 * for freeing/closing resources acquired in the initialisation stage.
	 * Calling this function again after this point should be a no-op
	 * until sstvenc_sequencer_ausrc_interface#reset is called.
	 *
	 * In the event of an error, it is the responsibility of the audio
	 * interface to clean up its state and release resources.
	 *
	 * @param[inout]	ausrc	Audio source context
	 * @param[out]		sample	The audio sample read
	 *
	 * @retval		1	Success, a sample has been written.
	 * @retval		0	Success, there are no more samples to
	 * 				read.
	 * @retval		<0	An error from `errno.h`, negated.
	 */
	int (*next)(struct sstvenc_sequencer_ausrc* const ausrc,
		    double* const			  sample);

	/*!
	 * Close the audio source.  This should release any resources acquired
	 * during initialisation.
	 *
	 * @param[inout]	ausrc	Audio source context
	 *
	 * @retval		0	Success
	 * @retval		<0	An error from `errno.h`, negated.
	 */
	int (*close)(struct sstvenc_sequencer_ausrc* const ausrc);
};

/*!
 * Sequencer audio source context.  This binds an audio source interface to
 * arbitrary context information needed to generate the audio samples.
 */
struct sstvenc_sequencer_ausrc {
	/*!
	 * Audio source interface.  This defines the methods for this instance
	 * of audio source.
	 */
	const struct sstvenc_sequencer_ausrc_interface* iface;

	/*!
	 * Context pointer.  This may be used for any means the audio
	 * interface wishes.
	 */
	void*						context;
};

/*!
 * Configure a step that sets the timescale unit.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		time_unit	Timescale unit
 * @param[in]		convert		Convert existing registers?
 */
void sstvenc_sequencer_step_set_timescale(
    struct sstvenc_sequencer_step* const step, uint8_t time_unit,
    _Bool convert);

/*!
 * Configure a step that sets a register to a new value.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		New register value
 */
void sstvenc_sequencer_step_set_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `old_value + value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to add to the register value
 */
void sstvenc_sequencer_step_inc_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `old_value - value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to subtract from the register
 */
void sstvenc_sequencer_step_dec_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `old_value * value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to multiply the register value
 * by
 */
void sstvenc_sequencer_step_mul_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `old_value / value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to divide the register value by
 */
void sstvenc_sequencer_step_div_reg(struct sstvenc_sequencer_step* const step,
				    uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `value - old_value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to add to the register value
 */
void sstvenc_sequencer_step_idec_reg(
    struct sstvenc_sequencer_step* const step, uint8_t reg, double value);

/*!
 * Configure a step that sets the register to `value / old_value`.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		reg		Register to adjust (see @ref
 * 					sequence_regs)
 * @param[in]		value		Amount to subtract from the register
 */
void sstvenc_sequencer_step_idiv_reg(
    struct sstvenc_sequencer_step* const step, uint8_t reg, double value);

/*!
 * Configure a step that emits silence.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		duration	The length of time to emit silence.
 * 					Use INFINITY for indefinite time
 * periods.
 */
void sstvenc_sequencer_step_silence(struct sstvenc_sequencer_step* const step,
				    double duration);

/*!
 * Configure a step that emits a tone.  The frequency and phase information is
 * set by whatever frequency is configured in the frequency register.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		duration	The length of time to emit a tone.
 * 					Use INFINITY for indefinite time
 * @param[in]		slopes		Which slopes to enable
 * 					(see @ref sequence_toneslopes)
 * periods.
 */
void sstvenc_sequencer_step_tone(struct sstvenc_sequencer_step* const step,
				 double duration, uint8_t slopes);

/*!
 * Configure a step that emits CW text.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		text		The CW text to emit.
 */
void sstvenc_sequencer_step_cw(struct sstvenc_sequencer_step* const step,
			       const char*			    text);

/*!
 * Configure a step that emits a SSTV image.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		mode		The SSTV mode to use.
 * @param[in]		framebuffer	The framebuffer containing the SSTV
 * 					image.  The image must be in the
 * 					correct colourspace format and
 * dimensions for the SSTV mode selected.
 */
void sstvenc_sequencer_step_image(struct sstvenc_sequencer_step* const step,
				  const struct sstvenc_mode* const     mode,
				  const uint8_t* framebuffer,
				  const char*	 fsk_id);

/*!
 * Configure a step that emits an audio recording.
 *
 * @param[out]		step		Sequencer step
 * @param[in]		ausrc		The audio source being emitted.
 */
void sstvenc_sequencer_step_audio(
    struct sstvenc_sequencer_step* const  step,
    struct sstvenc_sequencer_ausrc* const ausrc);

/*!
 * Configure the final step in the sequence.
 *
 * @param[out]		step		Sequencer step
 */
void   sstvenc_sequencer_step_end(struct sstvenc_sequencer_step* const step);

/*!
 * Initialise the sequencer with the given sequencer steps.
 *
 * @param[inout]	seq		Sequencer to initialise
 * @param[in]		steps		Sequencer steps, the last step MUST
 * 					have sstvenc_sequence_step#type
 * 					set to @ref SSTVENC_SEQ_STEP_TYPE_END.
 * @param[in]		event_cb	Optional event callback, set to NULL
 * 					for no callback.
 * @param[in]		event_cb_ctx	Optional event callback context.
 * @param[in]		sample_rate	Sample rate in hertz.
 */
void   sstvenc_sequencer_init(struct sstvenc_sequencer* const	   seq,
			      const struct sstvenc_sequencer_step* steps,
			      sstvenc_sequencer_event_cb*	   event_cb,
			      const void* event_cb_ctx, uint32_t sample_rate);

/*!
 * Reset the state machine back to the initial state.
 */
void   sstvenc_sequencer_reset(struct sstvenc_sequencer* const seq);

/*!
 * Advance the state of the state machine when generating infinite tones or
 * silence.  This does nothing unless the state machine is in one of the
 * following states:
 *
 * - @ref SSTVENC_SEQ_STATE_GEN_INF_SILENCE
 * - @ref SSTVENC_SEQ_STATE_GEN_INF_TONE
 */
void   sstvenc_sequencer_advance(struct sstvenc_sequencer* const seq);

/*!
 * Compute the next sample to be emitted by the sequencer.
 * The result will be stored in sstvenc_sequencer#output.
 *
 * This should be called until sstvenc_sequencer#state reaches
 * the value @ref SSTVENC_SEQ_STATE_DONE.
 */
void   sstvenc_sequencer_compute(struct sstvenc_sequencer* const seq);

/*!
 * Fill the given buffer with audio samples from the sequencer.  Stop if
 * we run out of buffer space or if the sequencer state machine finishes.
 * Return the number of samples generated.
 *
 * @param[inout]	seq		Sequencer state machine to pull
 * 					samples from.
 * @param[out]		buffer		Audio buffer to write samples to.
 * @param[in]		buffer_sz	Size of the audio buffer in samples.
 *
 * @returns		Number of samples written to @a buffer
 */
size_t sstvenc_sequencer_fill_buffer(struct sstvenc_sequencer* const seq,
				     double* buffer, size_t buffer_sz);

#endif
