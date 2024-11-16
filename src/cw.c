/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include "cw.h"
#include <string.h>

/*!
 * Symbol table mapping element.  Used in two contexts:
 * - to map a symbol to its morse-code representation
 * - to map a symbol to its alias
 *
 * It can be considered a key-value pair of strings.
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
 * - ' ': a dit's worth of space
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
	} else {
		cw->state = SSTVENC_CW_MOD_STATE_DONE;
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
	sstvenc_ps_reset(&(cw->ps), INFINITY);
	cw->pos++;
	if (cw->symbol->value[cw->pos]) {
		/* We need to produce another mark */
		cw->state = SSTVENC_CW_MOD_STATE_MARK;
	} else {
		/* End of the symbol */
		cw->state = SSTVENC_CW_MOD_STATE_DAHSPACE;
	}
}

static void sstvenc_cw_end_symbol(struct sstvenc_cw_mod* const cw) {
	sstvenc_ps_reset(&(cw->ps), INFINITY);
	cw->state	 = SSTVENC_CW_MOD_STATE_NEXT_SYM;
	cw->text_string += strlen(cw->symbol->key);
	cw->symbol	 = NULL;
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
		cw->output	= 0.0;
		cw->text_string = NULL;
		cw->symbol	= NULL;
		return;
	}
}
