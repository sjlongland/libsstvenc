# `libsstvenc`: A simple SSTV encoder library in C.

**THIS PROJECT IS A WORK-IN-PROGRESS**.

The idea of this project is to provide a reasonable quality SSTV encoder
implementation in C for use in custom SSTV applications.

## Implemented modes:

- Robot 8, 12 and 24 monochrome modes
- Robot 36 and 72 colour modes
- Scottie S1, S2 and DX
- Martin M1 and M2
- Pasokon P3, P5 and P7
- PD-50, PD-90, PD-120, PD-160, PD-180, PD-240 and PD-290
- Wraase SC-2 120 and 180

## Current work:

- Fixing timing issues in the state machine which is throwing the timing off.

## Example usage (using `libgd`):

```c
#include "gd.h"
#include "sstv.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: %s mode input.png output.raw\n",
			argv[0]);
		return 1;
	}

	const struct sstvenc_mode* mode = sstvenc_get_mode_by_name(argv[1]);
	struct sstvenc_encoder	   enc;

	uint16_t		   colourspace
	    = mode->colour_space_order & SSTVENC_CSO_MASK_MODE;
	uint8_t colours = 3;
	if (colourspace == SSTVENC_CSO_MODE_MONO) {
		colours = 1;
	}

	double* fb
	    = malloc(mode->width * mode->height * colours * sizeof(double));

	FILE* in = fopen(argv[2], "rb");
	if (!in) {
		fprintf(stderr, "Open of %s failed\n", argv[2]);
		return 1;
	}
	gdImagePtr im = gdImageCreateFromPng(in);
	fclose(in);

	/* Resize to destination */
	gdImagePtr im_resized = gdImageScale(im, mode->width, mode->height);

	memset(fb, 0, sizeof(fb));
	for (uint16_t y = 0; y < mode->height; y++) {
		for (uint16_t x = 0; x < mode->width; x++) {
			int c = gdImageGetTrueColorPixel(im_resized, x, y);
			uint32_t idx = sstvenc_get_pixel_posn(mode, x, y);
			double	 pv[colours];

			switch (colourspace) {
			case SSTVENC_CSO_MODE_MONO:
				pv[0] = gdTrueColorGetGreen(c) / 255.0;
				break;
			case SSTVENC_CSO_MODE_RGB:
				pv[0] = gdTrueColorGetRed(c) / 255.0;
				pv[1] = gdTrueColorGetGreen(c) / 255.0;
				pv[2] = gdTrueColorGetBlue(c) / 255.0;
				break;
			case SSTVENC_CSO_MODE_YUV:
			case SSTVENC_CSO_MODE_YUV2: {
				double r = gdTrueColorGetRed(c);
				double g = gdTrueColorGetGreen(c);
				double b = gdTrueColorGetBlue(c);
				pv[0]	 = (16.0
					    + (.003906
					       * ((65.738 * r) + (129.057 * g)
						  + (25.064 * b))))
					/ 255.0;
				pv[1] = (128.0
					 + (.003906
					    * ((112.439 * r) + (-94.154 * g)
					       + (-18.285 * b))))
					/ 255.0;
				pv[2] = (128.0
					 + (.003906
					    * ((-37.945 * r) + (-74.494 * g)
					       + (112.439 * b))))
					/ 255.0;
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

	sstvenc_encoder_init(&enc, mode, NULL, "TEST", fb, 1.0, 0, 48000);

	FILE* out = fopen(argv[3], "wb");
	while (enc.phase != SSTVENC_ENCODER_PHASE_DONE) {
		sstvenc_encoder_compute(&enc);
		fwrite(&enc.output, sizeof(double), 1, out);
	}
	fclose(out);

	return 0;
}
```

This produces a raw audio file at 48kHz sample rate, mono, 64-bit IEEE-754
float as output.
