/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <arpa/inet.h>
#include <errno.h>
#include <gd.h>
#include <getopt.h>
#include <libsstvenc/oscillator.h>
#include <libsstvenc/sstv.h>
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

/*!
 * Write an audio sample to the given output file.  Interface signature.
 *
 * @param[in]	sample	Audio sample being written
 * @param[out]	output	Output audio file
 *
 * @retval	1	Success
 * @retval	0	Failure (see errno)
 */
typedef size_t write_sample_fn(double sample, FILE* const output);

size_t	       write_sample_s8(double sample, FILE* const output) {
	/* Scale to 8-bit fixed-point */
	int8_t isample = INT8_MAX * sample;
	return fwrite(&isample, sizeof(isample), 1, output);
}

size_t write_sample_s16(double sample, FILE* const output) {
	/* Scale to 16-bit fixed-point */
	int8_t isample = INT16_MAX * sample;

	/* Convert to big-endian and write */
	isample	       = htons(isample);
	return fwrite(&isample, sizeof(isample), 1, output);
}

size_t write_sample_s32(double sample, FILE* const output) {
	/* Scale to 16-bit fixed-point */
	int32_t isample = INT32_MAX * sample;

	/* Convert to big-endian and write */
	isample		= htonl(isample);
	return fwrite(&isample, sizeof(isample), 1, output);
}

size_t write_sample_f32(double sample, FILE* const output) {
	union {
		float	 f;
		uint32_t ui;
	} fsample;

	fsample.f  = sample;

	/* Convert to big-endian and write */
	fsample.ui = htonl(fsample.ui);
	return fwrite(&fsample.ui, sizeof(fsample.ui), 1, output);
}

size_t write_sample_f64(double sample, FILE* const output) {
	union {
		double	 f;
		uint64_t ui;
	} fsample;
	uint32_t out[2];

	fsample.f = sample;

	/* Sadly, there's no htonll */
	out[0]	  = fsample.ui >> 32;
	out[1]	  = fsample.ui & UINT32_MAX;

	/* Convert to big-endian */
	out[0]	  = htonl(out[0]);
	out[1]	  = htonl(out[1]);

	return fwrite(out, sizeof(out), 1, output);
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
	uint32_t	     audio_encoding = 3;
	write_sample_fn*     write_sample   = write_sample_s16;

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
				write_sample   = write_sample_s8;
				audio_encoding = 2;
				break;
			case 16:
				/* 16-bit signed */
				write_sample   = write_sample_s16;
				audio_encoding = 3;
				break;
			case 32:
				switch (endptr[0]) {
				case 0:
				case 'S':
				case 's':
					/* 32-bit signed */
					write_sample   = write_sample_s32;
					audio_encoding = 5;
					break;
				case 'f':
				case 'F':
					/* 32-bit float */
					write_sample   = write_sample_f32;
					audio_encoding = 6;
					break;
				}
				break;
			case 64:
				/* 64-bit float */
				write_sample   = write_sample_f64;
				audio_encoding = 7;
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

	FILE* out = fopen(opt_output_au, "wb");
	if (!out) {
		perror("Failed to open output file");
		return (2);
	}

	/*
	 * Write out a Sun audio header.
	 * https://en.wikipedia.org/wiki/Au_file_format
	 */
	uint32_t hdr[7] = {
	    0x2e736e64,		  // Magic ".snd"
	    28,			  // Data offset: 28 bytes (7*4-bytes)
	    UINT32_MAX,		  // Data size: unknown
	    audio_encoding,	  // Encoding: int16_t linear
	    osc.sample_rate,	  // Sample rate
	    total_audio_channels, // Channels
	    0,			  // Annotation (unused)
	};
	for (int i = 0; i < 7; i++) {
		// Byte swap to big-endian
		hdr[i] = htonl(hdr[i]);
	}
	fwrite(hdr, sizeof(int32_t), 7, out);

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
		 * Our 16-bit fixed-point sample for the audio output.
		 */
		int16_t		sample;

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
			size_t res;
			if (select_audio_channels & (1 << ch)) {
				res = write_sample(osc.output, out);
			} else {
				res = write_sample(0.0, out);
			}

			if (res != 1) {
				perror("Failed to write audio sample");
				return (2);
			}
		}

		/* Count this sample */
		remaining--;
	}
	fclose(out);

	return 0;
}
