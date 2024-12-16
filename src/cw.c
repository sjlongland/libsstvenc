/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/cw.h>
#include <string.h>

/*!
 * @addtogroup cw
 * @{
 */

/*!
 * Symbol table mapping element.  Used to map a symbol to its morse-code
 * representation.  It can be considered a key-value pair of strings.
 */
struct sstvenc_cw_pair {
	const char* key;
	const char* value;
};

/*!
 * Symbol table… UTF-8 symbol mapped to the morse code sequence that
 * represents it.  This table is for single-byte sequences only.
 *
 * The values are strings where the characters have the following meanings:
 * - '.': a tone the length of a 'dit'
 * - '-': a tone the length of a 'dah' (3 dits)
 * - ' ': a dit's worth of space, this is a special-case kludge used to handle
 *   the space character between words.
 */
static struct sstvenc_cw_pair sstvenc_cw_symbols[] = {
    /* Whitespace */
    {.key = " ", .value = "  "}, /* NB: some space is already added */
    /* Letters */
    {.key = "A", .value = ".-"},
    {.key = "B", .value = "-..."},
    {.key = "C", .value = "-.-."},
    {.key = "D", .value = "-.."},
    {.key = "E", .value = "."},
    {.key = "F", .value = "..-."},
    {.key = "G", .value = "--."},
    {.key = "H", .value = "...."},
    {.key = "I", .value = ".."},
    {.key = "J", .value = ".---"},
    {.key = "K", .value = "-.-"},
    {.key = "L", .value = ".-.."},
    {.key = "M", .value = "--"},
    {.key = "N", .value = "-."},
    {.key = "O", .value = "---"},
    {.key = "P", .value = ".--."},
    {.key = "Q", .value = "--.-"},
    {.key = "R", .value = ".-."},
    {.key = "S", .value = "..."},
    {.key = "T", .value = "-"},
    {.key = "U", .value = "..-"},
    {.key = "V", .value = "...-"},
    {.key = "W", .value = ".--"},
    {.key = "X", .value = "-..-"},
    {.key = "Y", .value = "-.--"},
    {.key = "Z", .value = "--.."},
    /* Digits */
    {.key = "0", .value = "-----"},
    {.key = "1", .value = ".----"},
    {.key = "2", .value = "..---"},
    {.key = "3", .value = "...--"},
    {.key = "4", .value = "....-"},
    {.key = "5", .value = "....."},
    {.key = "6", .value = "-...."},
    {.key = "7", .value = "--..."},
    {.key = "8", .value = "---.."},
    {.key = "9", .value = "----."},
    /* Symbols */
    {.key = ".", .value = ".-.-.-"},
    {.key = ",", .value = "--..--"},
    {.key = "?", .value = "..--.."},
    {.key = "'", .value = ".----."},
    {.key = "!", .value = "-.-.--"},
    {.key = "/", .value = "-..-."},
    {.key = "(", .value = "-.--."},
    {.key = ")", .value = "-.--.-"},
    {.key = "&", .value = ".-..."},
    {.key = ":", .value = "---..."},
    {.key = "=", .value = "-...-"},
    {.key = "+", .value = ".-.-."},
    {.key = "-", .value = "-....-"},
    {.key = "_", .value = "..--.-"},
    {.key = "\"", .value = ".-..-."},
    {.key = "$", .value = "...-..-"},
    {.key = "@", .value = ".--.-."},
    {.key = NULL, .value = NULL},
};

/*!
 * Multi-byte symbols.  These symbols have keys that are more than one byte
 * each.  Non-English and CW prosigns.
 */
static struct sstvenc_cw_pair sstvenc_cw_mbsymbols[] = {
    /* Non-English */
    {.key = "À", .value = ".--.-"}, /* also Å */
    {.key = "Ä", .value = ".-.-"},  /* also Æ Ą */
    {.key = "Å", .value = ".--.-"},
    {.key = "Æ", .value = ".-.-"},
    {.key = "Ą", .value = ".-.-"},
    {.key = "Ć", .value = "-.-.."}, /* also Ĉ Ç */
    {.key = "Ĉ", .value = "-.-.."},
    {.key = "Ç", .value = "-.-.."},
    {.key = "Ð", .value = "..--."},
    {.key = "É", .value = "..-.."}, /* also Ę */
    {.key = "È", .value = ".-..-"}, /* also Ł */
    {.key = "Ę", .value = "..-.."},
    {.key = "Ĝ", .value = "--.-."},
    {.key = "Ĥ", .value = "----"}, /* also <CH> Š */
    {.key = "Ĵ", .value = ".---."},
    {.key = "Ł", .value = ".-..-"},
    {.key = "Ń", .value = "--.--"}, /* also Ñ */
    {.key = "Ñ", .value = "--.--"},
    {.key = "Ó", .value = "---."}, /* also Ö Ø */
    {.key = "Ö", .value = "---."},
    {.key = "Ø", .value = "---."},
    {.key = "Ś", .value = "...-..."},
    {.key = "Ŝ", .value = "...-."},
    {.key = "Š", .value = "----"},
    {.key = "Þ", .value = ".--.."},
    {.key = "Ü", .value = "..--"}, /* also Ŭ */
    {.key = "Ŭ", .value = "..--"},
    {.key = "Ź", .value = "--..-."},
    {.key = "Ż", .value = "--..-"},
    {.key = "<CH>", .value = "----"},
    /*
     * Prosigns: since < and > are not valid, we use these to
     * define the start and end of a prosign name.
     */
    {.key = "<END_OF_WORK>", .value = "...-.-"},
    {.key = "<ERROR>", .value = "........"},
    {.key = "<INVITATION>", .value = "-.-"},
    {.key = "<START>", .value = "-.-.-"},
    {.key = "<NEW_MESSAGE>", .value = ".-.-."},
    {.key = "<VERIFIED>", .value = "...-."},
    {.key = "<WAIT>", .value = ".-..."},
    {.key = NULL, .value = NULL},
};

/*!
 * Compare the character sequence given to the candidate symbol.
 *
 * @param[in]	sym		Text symbol to be compared.  May be
 * 				followed by other symbols.  Assumed UTF-8
 * 				encoding.
 *
 * @param[in]	candidate	Candidate morse code symbol being compared.
 * The string @a sym is a match if it starts with the sstvenc_cw_pair#key
 * given by @a candidate.
 *
 * @retval	candidate	The candidate is a match.
 * @retval	NULL		Not a match.
 */
static const struct sstvenc_cw_pair*
sstvenc_cw_symbol_match(const char*		      sym,
			const struct sstvenc_cw_pair* candidate);

/*!
 * Look up the symbol at the start of @a sym in the table @a table.
 *
 * @param[in]	sym		Text symbol being looked up.  May be followed
 * 				by other symbols.  Assumed UTF-8 encoding.
 *
 * @param[in]	table		Symbol table to scan for possible matches.
 * 				The table must be terminated by a
 * sstvenc_cw_pair with both sstvenc_cw_pair#key and sstvenc_cw_pair#value set
 * to NULL.
 *
 * @param[in]	len		The fixed symbol length to check.
 * 				For sstvenc_cw_symbols, all values are single
 * 				byte ASCII characters, so in that situation,
 * set this parameter to 1.  This optimises the most likely case: plain
 * English characters.
 *
 * 				If the symbols are a variable length, which is
 * 				the case for sstvenc_cw_mbsymbols, set this to
 * 				0 instead.
 *
 * @returns	The matching sstvenc_cw_pair for the symbol seen at the start
 * 		of @a sym.
 *
 * @retval	NULL		None of the symbols in @a table match the
 * 				character sequence at @a sym.
 */
static const struct sstvenc_cw_pair*
sstvenc_cw_symbol_lookup(const char* sym, const struct sstvenc_cw_pair* table,
			 uint8_t len);

/*!
 * Try to locate the CW symbol that represents the text at the start of the
 * string @a sym.  First, sstvenc_cw_symbols is searched for the most likely
 * case (English letters, digits and punctuation), then if that fails,
 * sstvenc_cw_mbsymbols is scanned (non-English symbols and prosigns).
 *
 * @param[in]	sym		The text to be matched to a morse-code symbol.
 * 				Assumed to be UTF-8 encoding.
 *
 * @returns	The matching sstvenc_cw_pair for the symbol seen at the start
 * 		of @a sym.
 *
 * @retval	NULL		The character sequence seen at the start of
 * 				@a sym does not match any known morse code
 * 				symbol.
 */
static const struct sstvenc_cw_pair* sstvenc_cw_get_symbol(const char* sym);

/*!
 * Scan the string given in sstvenc_cw_mod#text_string by repeatedly calling
 * sstvenc_cw_symbol_lookup.
 *
 * If the text at sstvenc_cw_mod#text_string does not match a known symbol,
 * we increment that and try again (until we run out of characters in
 * sstvenc_cw_mod#text_string).
 *
 * If we run out of characters, we move the state machine to
 * SSTVENC_CW_MOD_STATE_DONE -- we are finished.
 *
 * Otherwise, we load the symbol into sstvenc_cw_mod#symbol, reset
 * sstvenc_cw_mod#pos and enter SSTVENC_CW_MOD_STATE_MARK.
 */
static void sstvenc_cw_get_next_sym(struct sstvenc_cw_mod* const cw);

/*!
 * Prepare to transmit a "mark" (or a gap between marks).
 *
 * sstvenc_cw_mod#pos gives the position in the current morse-code symbol
 * being processed.  It points to either `' '`, `'.'` or `'-'`.
 *
 * For a `' '`: the oscillator amplitude is set to 0.0 and we reset the
 * pulse shaper for the duration of a single dit.
 *
 * For a `'.'`: the oscillator amplitude is set to 1.0 and we reset the
 * pulse shaper for the duration of a single dit.
 *
 * For a `'-'`: the oscillator amplitude is set to 1.0 and we reset the
 * pulse shaper for the duration of three dits.
 */
static void sstvenc_cw_start_mark(struct sstvenc_cw_mod* const cw);

/*!
 * Handle the end of a sub-symbol (dah or dit).  This resets the pulse
 * shaper then increments sstvenc_cw_mod#pos.  If there's more sub-symbols
 * we go back to state SSTVENC_CW_MOD_STATE_MARK, otherwise we enter
 * state SSTVENC_CW_MOD_STATE_DAHSPACE.
 */
static void sstvenc_cw_end_subsym(struct sstvenc_cw_mod* const cw);

/*!
 * Called at the end of a morse code symbol.  This advances
 * sstvenc_cw_mod#text_string by the length of the sstvenc_cw_pair#key
 * pointed to by sstvenc_cw_mod#symbol.
 *
 * sstvenc_cw_mod#symbol is then dropped and we move on in state
 * SSTVENC_CW_MOD_STATE_NEXT_SYM.
 */
static void sstvenc_cw_end_symbol(struct sstvenc_cw_mod* const cw);

/*!
 * Sub-state machine built around the pulse shaper state machine to handle
 * transmission of a mark.  This routine initialises the pulse shaper
 * by calling sstvenc_cw_start_mark then will step the pulse shaper through
 * its state machine by calling sstvenc_ps_compute.
 *
 * The envelope is modulated by the oscillator by calling sstvenc_osc_compute
 * then multiplying the outputs, storing these in sstvenc_cw_mod#output.
 *
 * At the end of the envelope, it resets sstvenc_cw_mod#output.
 * If we were actually transmitting a space, we move to the next sub-symbol
 * otherwise we enter SSTVENC_CW_MOD_STATE_DITSPACE to emit the
 * inter-sub-symbol dit space.
 */
static void sstvenc_cw_handle_state_mark(struct sstvenc_cw_mod* const cw);

/*!
 * Handling of the spaces between sub-symbols (individual dahs and dits). This
 * is the length of one "dit", and is measured by
 * sstvenc_pulseshape#sample_idx which is incremented each time
 * sstvenc_ps_compute is called whilst the shaper remains in the
 * SSTVENC_PS_PHASE_DONE state.
 *
 * When it passes sstvenc_cw_mod#dit_period, we call sstvenc_cw_end_subsym to
 * move to the next sub-symbol.
 */
static void sstvenc_cw_handle_state_ditspace(struct sstvenc_cw_mod* const cw);

/*!
 * Handling of the spaces between symbols (morse code symbols).  This is the
 * length of one "dah" (3 "dits"), and is measured by
 * sstvenc_pulseshape#sample_idx which is incremented each time
 * sstvenc_ps_compute is called whilst the shaper remains in the
 * SSTVENC_PS_PHASE_DONE state.
 *
 * As we enter this state _after_ being in the SSTVENC_CW_MOD_STATE_DITSPACE
 * state, one "dit"'s worth of space has already been emitted, so we carry on.
 *
 * When it passes sstvenc_cw_mod#dit_period * 2, we call sstvenc_cw_end_symbol
 * to move to the next symbol.
 */
static void sstvenc_cw_handle_state_dahspace(struct sstvenc_cw_mod* const cw);

/*!
 * Handling of the end of transmission.  We reset state variables and the
 * output so it emits zeroes from now on.
 */
static void sstvenc_cw_handle_state_done(struct sstvenc_cw_mod* const cw);

void	    sstvenc_cw_init(struct sstvenc_cw_mod* const cw, const char* text,
			    double amplitude, double frequency, double dit_period,
			    double slope_period, uint32_t sample_rate,
			    uint8_t time_unit) {

	       sstvenc_ps_init(&(cw->ps), amplitude, slope_period, INFINITY,
			       slope_period, sample_rate, time_unit);
	       sstvenc_osc_init(&(cw->osc), 1.0, frequency, 0.0, sample_rate);
	       cw->dit_period
		   = sstvenc_ts_unit_to_samples(dit_period, sample_rate, time_unit);
	       cw->pos	       = 0;
	       cw->symbol      = NULL;
	       cw->text_string = text;
	       cw->state       = SSTVENC_CW_MOD_STATE_INIT;
}

static const struct sstvenc_cw_pair*
sstvenc_cw_symbol_match(const char*		      sym,
			const struct sstvenc_cw_pair* candidate) {
	/* Check match */
	if (!strncmp(sym, candidate->key, strlen(candidate->key))) {
		/* Return the match */
		return candidate;
	} else {
		/* Not a match */
		return NULL;
	}
}

static const struct sstvenc_cw_pair*
sstvenc_cw_symbol_lookup(const char* sym, const struct sstvenc_cw_pair* table,
			 uint8_t len) {
	while (table->key) {
		if (len) {
			/* Known length, compare the symbols */
			if (!strncmp(sym, table->key, len)
			    && (table->key[len] == 0)) {
				/* Return the match */
				return table;
			}
		} else {
			const struct sstvenc_cw_pair* match
			    = sstvenc_cw_symbol_match(sym, table);
			if (match) {
				return match;
			}
		}
		table++;
	}

	return NULL;
}

static const struct sstvenc_cw_pair* sstvenc_cw_get_symbol(const char* sym) {
	/* Short-cut, look for single character symbols first */
	const struct sstvenc_cw_pair* match
	    = sstvenc_cw_symbol_lookup(sym, sstvenc_cw_symbols, 1);
	if (match) {
		return match;
	}

	/* Try searching the multi-byte table */
	match = sstvenc_cw_symbol_lookup(sym, sstvenc_cw_mbsymbols, 0);
	if (match) {
		return match;
	}

	/* Nothing found */
	return NULL;
}

static void sstvenc_cw_get_next_sym(struct sstvenc_cw_mod* const cw) {
	while ((cw->symbol == NULL) && cw->text_string[0]) {
		/* Look up the next symbol in the string */
		cw->symbol = sstvenc_cw_get_symbol(cw->text_string);
		if (cw->symbol == NULL) {
			/* Nothing here, advance to the next position */
			cw->text_string++;
		}
	}

	if (cw->symbol) {
		/* We have a character, reset the position */
		cw->state = SSTVENC_CW_MOD_STATE_MARK;
		cw->pos	  = 0;

		/* Process the mark so we have a valid output sample */
		sstvenc_cw_handle_state_mark(cw);
	} else {
		cw->state = SSTVENC_CW_MOD_STATE_DONE;
		sstvenc_cw_handle_state_done(cw);
	}
}

static void sstvenc_cw_start_mark(struct sstvenc_cw_mod* const cw) {
	switch (cw->symbol->value[cw->pos]) {
	case ' ':
		/* A space */
		cw->osc.amplitude = 0.0;
		sstvenc_ps_reset_samples(&(cw->ps), cw->dit_period
							- cw->ps.rise_sz
							- cw->ps.fall_sz);
		break;
	case '.':
		/* A dit */
		cw->osc.amplitude = 1.0;
		sstvenc_ps_reset_samples(&(cw->ps), cw->dit_period
							- cw->ps.rise_sz
							- cw->ps.fall_sz);
		break;
	case '-':
		/* A dah */
		cw->osc.amplitude = 1.0;
		sstvenc_ps_reset_samples(&(cw->ps), (cw->dit_period * 3)
							- cw->ps.rise_sz
							- cw->ps.fall_sz);
		break;
	}
}

static void sstvenc_cw_end_subsym(struct sstvenc_cw_mod* const cw) {
	sstvenc_ps_reset(&(cw->ps), INFINITY, SSTVENC_TS_UNIT_SECONDS);
	cw->pos++;
	if (cw->symbol->value[cw->pos]) {
		/* We need to produce another mark */
		cw->state = SSTVENC_CW_MOD_STATE_MARK;
		sstvenc_cw_handle_state_mark(cw);
	} else {
		/* End of the symbol */
		cw->state = SSTVENC_CW_MOD_STATE_DAHSPACE;
		sstvenc_cw_handle_state_dahspace(cw);
	}
}

static void sstvenc_cw_end_symbol(struct sstvenc_cw_mod* const cw) {
	sstvenc_ps_reset(&(cw->ps), INFINITY, SSTVENC_TS_UNIT_SECONDS);
	cw->text_string += strlen(cw->symbol->key);
	cw->symbol	 = NULL;
	cw->state	 = SSTVENC_CW_MOD_STATE_NEXT_SYM;
	sstvenc_cw_get_next_sym(cw);
}

static void sstvenc_cw_handle_state_mark(struct sstvenc_cw_mod* const cw) {
	switch (cw->ps.phase) {
	case SSTVENC_PS_PHASE_INIT:
		/* Start of a dah/dit/space */
		sstvenc_cw_start_mark(cw);
		/* Fall thru */
	case SSTVENC_PS_PHASE_RISE:
	case SSTVENC_PS_PHASE_HOLD:
	case SSTVENC_PS_PHASE_FALL:
		/* Dah/Dit in progress */
		sstvenc_ps_compute(&(cw->ps));
		sstvenc_osc_compute(&(cw->osc));
		cw->output = cw->ps.output * cw->osc.output;
		break;
	case SSTVENC_PS_PHASE_DONE:
		/* Dah/Dit finished, space starts here */
		cw->output	  = 0.0;
		cw->osc.amplitude = 0.0;
		if (cw->symbol->value[cw->pos] == ' ') {
			/* This was actually a space, don't add more! */
			sstvenc_cw_end_subsym(cw);
		} else {
			sstvenc_ps_compute(&(cw->ps));
			cw->state = SSTVENC_CW_MOD_STATE_DITSPACE;
			sstvenc_cw_handle_state_ditspace(cw);
		}
	}
}

static void
sstvenc_cw_handle_state_ditspace(struct sstvenc_cw_mod* const cw) {
	sstvenc_ps_compute(&(cw->ps));
	if (cw->ps.sample_idx > cw->dit_period) {
		/* That's enough space. */
		sstvenc_cw_end_subsym(cw);
	}
}

static void
sstvenc_cw_handle_state_dahspace(struct sstvenc_cw_mod* const cw) {
	sstvenc_ps_compute(&(cw->ps));
	if (cw->ps.sample_idx > (2 * cw->dit_period)) {
		/* That's enough space. */
		sstvenc_cw_end_symbol(cw);
	}
}

static void sstvenc_cw_handle_state_done(struct sstvenc_cw_mod* const cw) {
	cw->output	= 0.0;
	cw->text_string = NULL;
	cw->symbol	= NULL;
}

void sstvenc_cw_compute(struct sstvenc_cw_mod* const cw) {
	switch (cw->state) {
	case SSTVENC_CW_MOD_STATE_INIT:
	case SSTVENC_CW_MOD_STATE_NEXT_SYM:
		sstvenc_cw_get_next_sym(cw);
		break;
	case SSTVENC_CW_MOD_STATE_MARK:
		sstvenc_cw_handle_state_mark(cw);
		break;
	case SSTVENC_CW_MOD_STATE_DITSPACE:
		sstvenc_cw_handle_state_ditspace(cw);
		break;
	case SSTVENC_CW_MOD_STATE_DAHSPACE:
		sstvenc_cw_handle_state_dahspace(cw);
		break;
	case SSTVENC_CW_MOD_STATE_DONE:
	default:
		sstvenc_cw_handle_state_done(cw);
		return;
	}
}

size_t sstvenc_cw_fill_buffer(struct sstvenc_cw_mod* const cw, double* buffer,
			      size_t buffer_sz) {
	size_t written_sz = 0;

	while ((buffer_sz > 0) && (cw->state < SSTVENC_CW_MOD_STATE_DONE)) {
		sstvenc_cw_compute(cw);

		buffer[0] = cw->output;
		buffer++;
		buffer_sz--;

		written_sz++;
	}

	return written_sz;
}

/*!
 * @}
 */
