/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

/*!
 * Example program that takes a PNG image as input and encodes a
 * Sun Audio file at 48kHz sample rate, 16-bit mono in the specified
 * SSTV mode.
 *
 * Compile with -lsstvenc -lgd -lm.
 */

#include <arpa/inet.h>
#include <gd.h>
#include <libsstvenc/oscillator.h>
#include <libsstvenc/sstv.h>
#include <libsstvenc/timescale.h>
#include <libsstvenc/yuv.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: %s mode input.png output.au\n",
			argv[0]);
		return 1;
	}

	const struct sstvenc_mode* mode = sstvenc_get_mode_by_name(argv[1]);
	struct sstvenc_encoder	   enc;
	struct sstvenc_oscillator  osc;

	if (!mode) {
		fprintf(stderr, "Unknown mode %s\n", argv[1]);
		printf("Valid modes are:\n");
		uint8_t idx = 0;
		mode	    = sstvenc_get_mode_by_idx(idx);
		while (mode != NULL) {
			const char* cspace;
			switch (mode->colour_space_order
				& SSTVENC_CSO_MASK_MODE) {
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
			printf("%-8s : %-32s %4d x %4d %-4s %7.3f sec\n",
			       mode->name, mode->description, mode->width,
			       mode->height, cspace,
			       sstvenc_mode_get_txtime(mode, NULL) * 1.0e-9);

			idx++;
			mode = sstvenc_get_mode_by_idx(idx);
		}
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

	FILE* in = fopen(argv[2], "rb");
	if (!in) {
		fprintf(stderr, "Open of %s failed\n", argv[2]);
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

	sstvenc_encoder_init(&enc, mode, "TEST", fb);
	sstvenc_osc_init(&osc, 1.0, 0.0, 0.0, 48000);

	FILE*	 out	= fopen(argv[3], "wb");

	/*
	 * Write out a Sun audio header.
	 * https://en.wikipedia.org/wiki/Au_file_format
	 */
	uint32_t hdr[7] = {
	    0x2e736e64,	     // Magic ".snd"
	    28,		     // Data offset: 28 bytes (7*4-bytes)
	    UINT32_MAX,	     // Data size: unknown
	    3,		     // Encoding: int16_t linear
	    osc.sample_rate, // Sample rate
	    1,		     // Channels
	    0,		     // Annotation (unused)
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

		/* Scale to 16-bit fixed-point */
		sample = INT16_MAX * osc.output;

		/* Convert to big-endian and write */
		sample = htons(sample);
		fwrite(&sample, sizeof(int16_t), 1, out);

		/* Count this sample */
		remaining--;
	}
	fclose(out);

	return 0;
}
