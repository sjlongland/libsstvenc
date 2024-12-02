/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <getopt.h>
#include <libsstvenc/cw.h>
#include <libsstvenc/sunau.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void show_usage(const char* prog_name) {
	printf("Usage: %s [options] \"MORSE CODE TEXT\" output.au\n"
	       "Options:\n"
	       "  {--bits | -B} BITS: set the number of bits per sample\n"
	       "    8:    8-bit signed integer\n"
	       "    16:   16-bit signed integer (default)\n"
	       "    32:   32-bit signed integer\n"
	       "    32s:  also 32-bit signed integer (alias)\n"
	       "    32f:  32-bit IEEE-754 float\n"
	       "    64:   64-bit IEEE-754 float\n"
	       "  {--chan | -C} CHAN: select audio channels\n"
	       "    1:    mono output (default)\n"
	       "    m:    mono output (alias)\n"
	       "    2:    stereo output         -- use both channels\n"
	       "    s:    stereo output (alias) -- use both channels\n"
	       "    l:    stereo output         -- use left channel\n"
	       "    r:    stereo output         -- use right channel\n"
	       "  {--rate | -R} RATE: sample rate in Hz\n"
	       "  {--slope | -S} SLOPE: slope period in milliseconds "
	       "(negative for "
	       "auto)\n"
	       "  {--dit-period | -d} PERIOD: Set the morse code dit period "
	       "in milliseconds\n"
	       "  {--freq | -f} FREQ: Set the morse code oscillator "
	       "frequency\n",
	       prog_name);
}

int main(int argc, char* argv[]) {
	const char*	     opt_channels     = "1";
	const char*	     opt_input_txt    = NULL;
	const char*	     opt_output_au    = NULL;
	char*		     endptr	      = NULL;
	int		     opt_bits	      = 16;
	int		     opt_rate	      = 48000;
	int		     opt_idx	      = 0;
	double		     opt_freq	      = 800.0;
	double		     opt_dit_period   = 80.0;
	double		     opt_slope_period = -1.0;
	uint8_t		     total_audio_channels;
	uint8_t		     select_audio_channels;
	uint8_t		     audio_encoding = SSTVENC_SUNAU_FMT_S16;

	static struct option long_options[] = {
	    {.name    = "bits",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'B'},
	    {.name    = "chan",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'C'},
	    {.name    = "rate",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'R'},
	    {.name    = "slope",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'S'},
	    {.name    = "dit-period",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'd'},
	    {.name    = "freq",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'f'},
	    {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0},
	};

	while (1) {
		int c = getopt_long(argc, argv, "B:C:R:S:d:f:", long_options,
				    &opt_idx);
		if (c == -1) {
			break;
		} else if (!c && !long_options[opt_idx].flag) {
			c = long_options[opt_idx].val;
		}

		switch (c) {
		case 0:
			break;
		case 'B': /* Set bits */
			opt_bits = strtol(optarg, &endptr, 10);
			switch (opt_bits) {
			case 8:
				/* 8-bit signed */
				audio_encoding = SSTVENC_SUNAU_FMT_S8;
				break;
			case 16:
				/* 16-bit signed */
				audio_encoding = SSTVENC_SUNAU_FMT_S16;
				break;
			case 32:
				switch (endptr[0]) {
				case 0:
				case 'S':
				case 's':
					/* 32-bit signed */
					audio_encoding
					    = SSTVENC_SUNAU_FMT_S32;
					break;
				case 'f':
				case 'F':
					/* 32-bit float */
					audio_encoding
					    = SSTVENC_SUNAU_FMT_F32;
					break;
				}
				break;
			case 64:
				/* 64-bit float */
				audio_encoding = SSTVENC_SUNAU_FMT_F64;
				break;
			default:
				fprintf(stderr,
					"Invalid number of bits: %s\n"
					"Supported values: 8, 16, 32/32s/32f "
					"and 64\n",
					optarg);
				return (1);
			}
			break;
		case 'C': /* Set channel count / usage */
			opt_channels = optarg;
			break;
		case 'R': /* Set sample rate */
			opt_rate = atoi(optarg);
			break;
		case 'S': /* Set slope period */
			opt_slope_period = strtod(optarg, &endptr);
			if (endptr[0]) {
				fprintf(stderr, "Invalid slope period: %s\n",
					optarg);
				return (1);
			}
			break;
		case 'd': /* Set dit period */
			opt_dit_period = strtod(optarg, &endptr);
			if (endptr[0]) {
				fprintf(stderr, "Invalid dit period: %s\n",
					optarg);
				return (1);
			}
			break;
		case 'f': /* Set frequency */
			opt_freq = strtod(optarg, &endptr);
			if (endptr[0]) {
				fprintf(stderr, "Invalid frequency: %s\n",
					optarg);
				return (1);
			}
			break;
		default:
			show_usage(argv[0]);
			return 1;
		}
	}

	if ((argc - optind) != 2) {
		show_usage(argv[0]);
		return 1;
	}
	opt_input_txt = argv[optind];
	opt_output_au = argv[optind + 1];

	switch (opt_channels[0]) {
	case '1':
	case 'm':
	case 'M':
		/* Mono output */
		total_audio_channels  = 1;
		select_audio_channels = 1;
		break;
	case '2':
	case 's':
	case 'S':
		/* Stereo output : both channels */
		total_audio_channels  = 2;
		select_audio_channels = UINT8_MAX;
		break;
	case 'l':
	case 'L':
		/* Stereo output : left channel */
		total_audio_channels  = 2;
		select_audio_channels = 1;
		break;
	case 'r':
	case 'R':
		/* Stereo output : left channel */
		total_audio_channels  = 2;
		select_audio_channels = 2;
		break;
	default:
		printf("Unknown channel mode: %s\n", opt_channels);
		return (1);
	}

	if (opt_slope_period < 0) {
		opt_slope_period = 0.2 * opt_dit_period;
	}

	struct sstvenc_cw_mod	 cw;
	struct sstvenc_sunau_enc au;

	sstvenc_cw_init(&cw, opt_input_txt, 1.0, opt_freq, opt_dit_period,
			opt_slope_period, opt_rate,
			SSTVENC_TS_UNIT_MILLISECONDS);
	{
		int res = sstvenc_sunau_enc_init(&au, opt_output_au, opt_rate,
						 audio_encoding,
						 total_audio_channels);
		if (res < 0) {
			fprintf(stderr, "Failed to open output file %s: %s\n",
				opt_output_au, strerror(-res));
		}
	}

	while (cw.state != SSTVENC_CW_MOD_STATE_DONE) {
		/* Our audio samples for this audio frame */
		double samples[total_audio_channels];

		/* Compute the next CW sample. */
		sstvenc_cw_compute(&cw);

		/* Fill the frame */
		for (uint8_t ch = 0; ch < total_audio_channels; ch++) {
			if (select_audio_channels & (1 << ch)) {
				samples[ch] = cw.output;
			} else {
				samples[ch] = 0.0;
			}
		}

		/* Write the audio frame */
		int au_res = sstvenc_sunau_enc_write(
		    &au, total_audio_channels, samples);
		if (au_res < 0) {
			fprintf(stderr, "Failed to write audio samples: %s\n",
				strerror(-au_res));
			return (2);
		}
	}

	{
		int au_res = sstvenc_sunau_enc_close(&au);
		if (au_res < 0) {
			fprintf(stderr,
				"Failed to close audio output file: %s\n",
				strerror(-au_res));
			return (2);
		}
	}

	return 0;
}
