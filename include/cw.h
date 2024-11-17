#ifndef _SSTVENC_CW_H
#define _SSTVENC_CW_H

/*!
 * CW encoder and modulator implementation.
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include "oscillator.h"
#include "pulseshape.h"
#include <stddef.h>
#include <stdint.h>

/* Forward declaration of CW pair type, for internal use. */
struct sstvenc_cw_pair;

/* CW modulator states */
#define SSTVENC_CW_MOD_STATE_INIT     (0)
#define SSTVENC_CW_MOD_STATE_NEXT_SYM (1)
#define SSTVENC_CW_MOD_STATE_MARK     (2)
#define SSTVENC_CW_MOD_STATE_DITSPACE (3)
#define SSTVENC_CW_MOD_STATE_DAHSPACE (4)
#define SSTVENC_CW_MOD_STATE_DONE     (5)

struct sstvenc_cw_mod {
	/*!
	 * Output sample.
	 */
	double			      output;

	/*!
	 * UTF-8 string to be transmitted.  Valid symbols for transmission
	 * are:
	 *
	 * - letters (A-Z a-z)
	 * - digits (0-9)
	 * - punctuation symbols from the list:
	 *   `. , ? ' ! / ( ) & : = + - _ " $ @`
	 * - non-English symbols from the list:
	 *   `À Ä Å Æ Ą Ć Ĉ Ç Ð É È Ę Ĝ Ĥ Ĵ Ł Ń Ñ Ó Ö Ø Ś Ŝ Š Þ Ü Ŭ Ź Ż`
	 * - the following prosigns (which are indicated by <angle brackets>):
	 *   `<END_OF_WORK>` `<ERROR>` `<INVITATION>` `<START>`
	 *   `<NEW_MESSAGE>` `<VERIFIED>` `<WAIT>`
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

	/*!
	 * Oscillator for the modulator.
	 */
	struct sstvenc_oscillator     osc;

	/*!
	 * Pulse shaper for the modulator.
	 */
	struct sstvenc_pulseshape     ps;

	/*!
	 * Dit period in samples
	 */
	uint16_t		      dit_period;

	/*! Current state machine state */
	uint8_t			      state;

	/*! Position within the current symbol being transmitted. */
	uint8_t			      pos;
};

static inline void sstvenc_cw_init(struct sstvenc_cw_mod* const cw,
				   const char* text, double amplitude,
				   double frequency, double dit_period,
				   double slope_period, uint32_t sample_rate,
				   uint8_t time_unit) {

	sstvenc_ps_init(&(cw->ps), amplitude, slope_period, INFINITY,
			slope_period, sample_rate, time_unit);
	sstvenc_osc_init(&(cw->osc), 1.0, frequency, 0.0, sample_rate);
	cw->dit_period	= ((uint16_t)(sample_rate * dit_period + 0.5));
	cw->pos		= 0;
	cw->symbol	= NULL;
	cw->text_string = text;
	cw->state	= SSTVENC_CW_MOD_STATE_INIT;
}

void sstvenc_cw_compute(struct sstvenc_cw_mod* const cw);

#endif
