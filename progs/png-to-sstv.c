/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <gd.h>
#include <getopt.h>
#include <libsstvenc/oscillator.h>
#include <libsstvenc/sstv.h>
#include <libsstvenc/sunau.h>
#include <libsstvenc/timescale.h>
#include <libsstvenc/yuv.h>
#include <stdio.h>
#include <string.h>

static void show_modes(void) {
	uint8_t			   idx	= 0;
	const struct sstvenc_mode* mode = sstvenc_get_mode_by_idx(idx);

	printf("MODE     : Description                      "
	       "Wdth x Hght CS.  TX Time*\n"
	       "-------- : -------------------------------- "
	       "----------- ---- ------------\n");

	while (mode != NULL) {
		const char* cspace;
		switch (mode->colour_space_order & SSTVENC_CSO_MASK_MODE) {
		case SSTVENC_CSO_MODE_MONO:
			cspace = "MONO";
			break;
		case SSTVENC_CSO_MODE_RGB:
			cspace = "RGB";
			break;
		case SSTVENC_CSO_MODE_YUV:
		case SSTVENC_CSO_MODE_YUV2:
			cspace = "YUV";
			break;
		}
		printf("%-8s : %-32s %4d x %4d %-4s %7.3f sec\n", mode->name,
		       mode->description, mode->width, mode->height, cspace,
		       sstvenc_mode_get_txtime(mode, NULL) * 1.0e-9);

		idx++;
		mode = sstvenc_get_mode_by_idx(idx);
	}

	printf("\n* not including FSK ID\n");
}

static void show_usage(const char* prog_name) {
	printf("Usage: %s [options] input.png output.au\n"
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
	       "  {--mode | -M} MODE: choose SSTV mode (see below)\n"
	       "  {--rate | -R} RATE: sample rate in Hz\n"
	       "  {--fsk-id | -f} FSKID: Set the FSK ID\n"
	       "\n"
	       "Supported SSTV modes:\n",
	       prog_name);

	show_modes();
}

int main(int argc, char* argv[]) {
	const char*	     opt_fsk_id	   = NULL;
	const char*	     opt_channels  = "1";
	const char*	     opt_mode	   = "M1";
	const char*	     opt_input_img = NULL;
	const char*	     opt_output_au = NULL;
	char*		     endptr	   = NULL;
	int		     opt_bits	   = 16;
	int		     opt_rate	   = 48000;
	int		     opt_idx	   = 0;
	uint8_t		     total_audio_channels;
	uint8_t		     select_audio_channels;
	uint32_t	     audio_encoding = SSTVENC_SUNAU_FMT_S16;

	static struct option long_options[] = {
	    {.name    = "bits",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'B'},
	    {.name    = "chan",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'C'},
	    {.name    = "mode",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'M'},
	    {.name    = "rate",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'R'},
	    {.name    = "fsk-id",
	     .has_arg = required_argument,
	     .flag    = NULL,
	     .val     = 'f'},
	    {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0},
	};

	while (1) {
		int c = getopt_long(argc, argv, "B:C:M:R:f:", long_options,
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
		case 'M': /* Set SSTV mode */
			opt_mode = optarg;
			break;
		case 'R': /* Set sample rate */
			opt_rate = atoi(optarg);
			break;
		case 'f': /* Set FSK ID */
			opt_fsk_id = optarg;
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
	opt_input_img = argv[optind];
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

	const struct sstvenc_mode* mode = sstvenc_get_mode_by_name(opt_mode);
	struct sstvenc_encoder	   enc;
	struct sstvenc_oscillator  osc;
	struct sstvenc_sunau_enc   au;

	if (!mode) {
		fprintf(stderr, "Unknown mode %s\n", opt_mode);
		printf("Valid modes are:\n");
		show_modes();
		return 1;
	}

	uint16_t colourspace
	    = mode->colour_space_order & SSTVENC_CSO_MASK_MODE;
	uint8_t colours = 3;
	if (colourspace == SSTVENC_CSO_MODE_MONO) {
		colours = 1;
	}

	uint8_t* fb
	    = malloc(mode->width * mode->height * colours * sizeof(uint8_t));

	FILE* in = fopen(opt_input_img, "rb");
	if (!in) {
		perror("Failed to open input file");
		return 1;
	}
	gdImagePtr im = gdImageCreateFromPng(in);
	fclose(in);

	/* Resize to destination */
	gdImagePtr im_resized = gdImageScale(im, mode->width, mode->height);

	for (uint16_t y = 0; y < mode->height; y++) {
		for (uint16_t x = 0; x < mode->width; x++) {
			int c = gdImageGetTrueColorPixel(im_resized, x, y);
			uint32_t idx = sstvenc_get_pixel_posn(mode, x, y);
			uint8_t	 pv[colours];
			uint8_t	 r = gdTrueColorGetRed(c);
			uint8_t	 g = gdTrueColorGetGreen(c);
			uint8_t	 b = gdTrueColorGetBlue(c);

			switch (colourspace) {
			case SSTVENC_CSO_MODE_MONO:
				pv[0] = sstvenc_yuv_calc_y(r, g, b);
				break;
			case SSTVENC_CSO_MODE_RGB:
				pv[0] = r;
				pv[1] = g;
				pv[2] = b;
				break;
			case SSTVENC_CSO_MODE_YUV:
			case SSTVENC_CSO_MODE_YUV2: {
				pv[0] = sstvenc_yuv_calc_y(r, g, b);
				pv[1] = sstvenc_yuv_calc_u(r, g, b);
				pv[2] = sstvenc_yuv_calc_v(r, g, b);
				break;
			}
			default:
				break;
			}

			for (uint8_t c = 0; c < colours; c++) {
				fb[idx + c] = pv[c];
			}
		}
	}
	gdImageDestroy(im_resized);
	gdImageDestroy(im);

	sstvenc_encoder_init(&enc, mode, opt_fsk_id, fb);
	sstvenc_osc_init(&osc, 1.0, 0.0, 0.0, opt_rate);
	{
		int res = sstvenc_sunau_enc_init(
		    &au, opt_output_au, osc.sample_rate, audio_encoding,
		    total_audio_channels);
		if (res < 0) {
			fprintf(stderr, "Failed to open output file %s: %s\n",
				opt_output_au, strerror(-res));
		}
	}

	/*
	 * Begin writing and computing the audio data.
	 */
	while (enc.phase != SSTVENC_ENCODER_PHASE_DONE) {
		/*
		 * As we do this, we may encounter slippage due to rounding of
		 * durations (in nanoseconds) to samples.  We account for this
		 * by tallying up both separately, and comparing them.
		 */
		static uint64_t total_samples = 0;
		static uint64_t total_ns      = 0;
		static uint32_t remaining     = 0;

		/*
		 * Our audio samples for this audio frame
		 */
		double		samples[total_audio_channels];

		if (!remaining) {
			/*
			 * No samples remaining, compute the next pulse.  This
			 * function returns NULL when it has nothing more for
			 * us.
			 */
			const struct sstvenc_encoder_pulse* pulse
			    = sstvenc_encoder_next_pulse(&enc);

			if (pulse) {
				/* Update the oscillator frequency */
				sstvenc_osc_set_frequency(&osc,
							  pulse->frequency);

				/* Figure out time duration in samples */
				remaining = sstvenc_ts_unit_to_samples(
				    pulse->duration_ns, osc.sample_rate,
				    SSTVENC_TS_UNIT_NANOSECONDS);

				/* Total up time and sample count */
				total_samples += remaining;
				total_ns      += pulse->duration_ns;

				/* Sanity check timing, adjust for any
				 * slippage */
				uint64_t expected_total_samples
				    = sstvenc_ts_unit_to_samples(
					total_ns, osc.sample_rate,
					SSTVENC_TS_UNIT_NANOSECONDS);
				if (expected_total_samples > total_samples) {
					/*
					 * Rounding error has caused a slip,
					 * add samples to catch up.
					 */
					uint64_t diff = expected_total_samples
							- total_samples;
					remaining     += diff;
					total_samples += diff;
				}
			}
		}

		/* Compute the next oscillator output sample */
		sstvenc_osc_compute(&osc);

		for (uint8_t ch = 0; ch < total_audio_channels; ch++) {
			if (select_audio_channels & (1 << ch)) {
				samples[ch] = osc.output;
			} else {
				samples[ch] = 0.0;
			}

			int au_res = sstvenc_sunau_enc_write(
			    &au, total_audio_channels, samples);
			if (au_res < 0) {
				fprintf(stderr,
					"Failed to write audio samples: %s\n",
					strerror(-au_res));
				return (2);
			}
		}

		/* Count this sample */
		remaining--;
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
