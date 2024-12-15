#ifndef _SSTVENC_CW_H
#define _SSTVENC_CW_H

/*!
 * @defgroup cw CW encoder and modulator implementation.
 * @{
 *
 * This module implements a morse-code modulator for generating CW IDs
 * as required by some juristictions for computer-generated transmissions.
 *
 * Operation is pretty simple:
 * ```c
 *
 * struct sstvenc_cw_mod cw;
 * sstvenc_cw_init(
 * 	&cw,				// pass in the context
 *	"HELLO WORLD",			// your text to transmit
 *	1.0,				// amplitude [0.0, 1.0]
 *	800.0,				// frequency in Hz
 *	200,				// dit period
 *	5,				// rising/falling slope period
 *	48000,				// sample rate in Hz
 *	SSTVENC_TS_UNIT_MILLISECONDS	// time unit
 * );
 *
 * while(cw.state != SSTVENC_CW_MOD_STATE_DONE) {
 * 	sstvenc_cw_compute(&cw);
 * 	write_to_audio_output(&cw.output);
 * }
 * ```
 *
 * The text is assumed to be UTF-8 format using symbols defined in one of
 * two symbol tables:
 *
 * - sstvenc_cw_symbols : holds all the common single-byte ASCII symbols
 * - sstvenc_cw_mbsymbols : holds non-English symbols and prosigns.
 *
 * Any character not in those tables will be ignored by the state machine.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/oscillator.h>
#include <libsstvenc/pulseshape.h>
#include <libsstvenc/timescale.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declaration of CW pair type, for internal use. */
struct sstvenc_cw_pair;

/*!
 * @defgroup cw_states CW modulator states
 * @{
 */

/*!
 * Modulator is in the initial state.  The value of sstvenc_cw_mod#output
 * is not guaranteed to be meaningful at this point.
 */
#define SSTVENC_CW_MOD_STATE_INIT     (0)

/*!
 * Modulator is to load the next symbol into the state machine on the next
 * call to sstvenc_cw_compute.
 */
#define SSTVENC_CW_MOD_STATE_NEXT_SYM (1)

/*!
 * Modulator transmitting a "mark" (dah or dit).
 */
#define SSTVENC_CW_MOD_STATE_MARK     (2)

/*!
 * Modulator is transmitting a "space" the length of a 'dit'.
 */
#define SSTVENC_CW_MOD_STATE_DITSPACE (3)

/*!
 * Modulator is transmitting a "space" the length of a 'dah'.
 */
#define SSTVENC_CW_MOD_STATE_DAHSPACE (4)

/*!
 * Modulator has finished transmitting the text string.
 * sstvenc_cw_mod#output will emit zeros from now on.
 */
#define SSTVENC_CW_MOD_STATE_DONE     (5)

/*!
 * @}
 */

/*!
 * CW state machine context.  This structure bundles an oscillator and pulse
 * shaper to generate a morse code waveform from the plain text given.
 *
 * This should be initialised by calling @ref sstvenc_cw_init.
 * Then, to obtain samples, call @ref sstvenc_cw_compute.  Each output sample
 * is then emitted via sstvenc_cw_mod#output.
 *
 * The state machine's progress can be tracked by observing
 * sstvenc_cw_mod#state.  The modulator is finished transmitting when this
 * changes to SSTVENC_CW_MOD_STATE_DONE.
 */
struct sstvenc_cw_mod {
	/*!
	 * Output sample.  Call @sstvenc_cw_compute to calculate this, then
	 * read the result from here.
	 */
	double			      output;

	/*!
	 * UTF-8 string to be transmitted.  Valid symbols for transmission
	 * are given in the tables @ref sstvenc_cw_symbols and
	 * @ref sstvenc_cw_mbsymbols.
	 *
	 * The non-English Ch character can be emitted using the pseudo
	 * prosign <CH> or the equivalent character Ĥ to avoid it getting
	 * confused with the C and H symbols.  Anything else will be ignored.
	 *
	 * During transmission, this pointer will be incremented.
	 */
	const char*		      text_string;

	/*! Current symbol being transmitted */
	const struct sstvenc_cw_pair* symbol;

	/*! Oscillator for the modulator. */
	struct sstvenc_oscillator     osc;

	/*! Pulse shaper for the modulator. */
	struct sstvenc_pulseshape     ps;

	/*! Dit period in samples */
	uint16_t		      dit_period;

	/*! Current state machine state */
	uint8_t			      state;

	/*! Position within the current symbol being transmitted. */
	uint8_t			      pos;
};

/*!
 * Initialise a CW state machine.
 *
 * @param[inout]	cw		CW state machine context being
 * 					initialised.
 * @param[in]		text		Text to transmit in CW (morse code)
 * @param[in]		amplitude	Peak amplitude of the carrier on the
 * 					scale 0.0-1.0.
 * @param[in]		frequency	Oscillator frequency in Hz.
 * @param[in]		dit_period	The length of a morse code 'dit'.
 * @param[in]		slope_period	The duration used for rising and
 * 					falling pulse edges for bandwidth
 * 					reduction.
 * @param[in]		sample_rate	The sample rate of the output waveform
 * 					in hertz.
 * @param[in]		time_unit	The time unit used for measuring
 * 					@a dit_period and @a slope_period.
 */
void sstvenc_cw_init(struct sstvenc_cw_mod* const cw, const char* text,
		     double amplitude, double frequency, double dit_period,
		     double slope_period, uint32_t sample_rate,
		     uint8_t time_unit);

/*!
 * Compute the next sample in the state machine.  The state machine will be
 * advanced to the next state and a sample written to sstvenc_cw_mod#output.
 */
void sstvenc_cw_compute(struct sstvenc_cw_mod* const cw);

/*! @} */
#endif
