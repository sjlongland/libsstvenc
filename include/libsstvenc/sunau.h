#ifndef _SSTVENC_SUNAU_H
#define _SSTVENC_SUNAU_H

/*!
 * @defgroup sunau Sun Audio encoder.
 * @{
 *
 * This module implements a simple Sun Audio file encoder.
 *
 * Reference: https://en.wikipedia.org/wiki/Au_file_format
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*!
 * Magic bytes at start of the Sun Audio header.  This is in fact, the ASCII
 * characters ".snd".
 */
#define SSTVENC_SUNAU_MAGIC   (0x2e736e64u)

/*!
 * @addtogroup sunau_formats Audio encoding formats
 * @{
 *
 * More are supported by the standard, but are not implemented by this module.
 */

#define SSTVENC_SUNAU_FMT_S8  (0x02u) /*!< 8-bit signed integer */
#define SSTVENC_SUNAU_FMT_S16 (0x03u) /*!< 16-bit signed integer */
#define SSTVENC_SUNAU_FMT_S32 (0x05u) /*!< 32-bit signed integer */
#define SSTVENC_SUNAU_FMT_F32 (0x06u) /*!< 32-bit IEEE-754 float */
#define SSTVENC_SUNAU_FMT_F64 (0x07u) /*!< 64-bit IEEE-754 float */

/*!
 * @}
 */

/*!
 * Encoder context.  Stores the fields necessary to construct the header and
 * the file pointer.
 */
struct sstvenc_sunau_enc {
	/*! Pointer to the open file for writing */
	FILE*	 fh;
	/*! Number of bytes written */
	uint32_t written_sz;
	/*! File sample rate in Hz */
	uint32_t sample_rate;
	/*! Internal state */
	uint16_t state;
	/*! Audio encoding, see @ref sunau_formats */
	uint8_t	 encoding;
	/*! Channel count */
	uint8_t	 channels;
};

/*!
 * Validate the given settings as sane.
 *
 * @param[in]	sample_rate	Sample rate for the audio output in Hz
 * @param[in]	encoding	Audio encoding for the output file
 * @param[in]	channels	Number of channels in the audio file
 *
 * @retval	0		Settings are valid
 * @retval	-EINVAL		Invalid sample rate, encoding or channel count
 */
static inline int sstvenc_sunau_enc_check(uint32_t sample_rate,
					  uint8_t  encoding,
					  uint8_t  channels) {
	if (!channels)
		return -EINVAL;
	if (!sample_rate)
		return -EINVAL;
	switch (encoding) {
	case SSTVENC_SUNAU_FMT_S8:
	case SSTVENC_SUNAU_FMT_S16:
	case SSTVENC_SUNAU_FMT_S32:
	case SSTVENC_SUNAU_FMT_F32:
	case SSTVENC_SUNAU_FMT_F64:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*!
 * Initialise an audio context with an opened file.
 *
 * @param[out]		enc		SunAU encoder context
 * @param[inout]	fh		Existing file handle, open for writing
 * 					in binary mode, positioned at the
 * start of the file.
 * @param[in]		sample_rate	Sample rate for the audio output in Hz
 * @param[in]		encoding	Audio encoding for the output file
 * @param[in]		channels	Number of channels in the audio file
 *
 * @retval		0		Success
 * @retval		-EINVAL		Invalid sample rate, encoding or
 * channel count
 */
static inline int
sstvenc_sunau_enc_init_fh(struct sstvenc_sunau_enc* const enc, FILE* fh,
			  uint32_t sample_rate, uint8_t encoding,
			  uint8_t channels) {
	int res = sstvenc_sunau_enc_check(sample_rate, encoding, channels);
	if (res < 0) {
		return res;
	}

	enc->fh		 = fh;
	enc->written_sz	 = 0;
	enc->state	 = 0;
	enc->sample_rate = sample_rate;
	enc->encoding	 = encoding;
	enc->channels	 = channels;

	return 0;
}

/*!
 * Open a file for writing.
 *
 * @param[out]	enc		SunAU encoder context
 * @param[in]	path		Path to the file to open for writing.
 * @param[in]	sample_rate	Sample rate for the audio output in Hz
 * @param[in]	encoding	Audio encoding for the output file
 * @param[in]	channels	Number of channels in the audio file
 *
 * @retval	0		Success
 * @retval	-EINVAL		Invalid sample rate, encoding or channel count
 * @retval	<0		`-errno` result from `fopen()` call.
 */
static inline int sstvenc_sunau_enc_init(struct sstvenc_sunau_enc* const enc,
					 const char*			 path,
					 uint32_t sample_rate,
					 uint8_t encoding, uint8_t channels) {
	int res = sstvenc_sunau_enc_check(sample_rate, encoding, channels);
	if (res < 0) {
		return res;
	}

	enc->fh = fopen(path, "wb");
	if (enc->fh == NULL) {
		return -errno;
	}

	enc->written_sz	 = 0;
	enc->state	 = 0;
	enc->sample_rate = sample_rate;
	enc->encoding	 = encoding;
	enc->channels	 = channels;

	return 0;
}

/*!
 * Write some audio samples to the file.  Audio is assumed to be a whole
 * number of audio frames, given as double-precision values in the range
 * [-1.0, 1.0] in the sample rate defined for the file.
 *
 * @param[inout]	enc		SunAU encoder context
 * @param[in]		n_samples	Number of samples in the buffer
 * @param[in]		samples		The samples to be written
 *
 * @retval		0		Success
 * @retval		-EINVAL		Invalid number of samples (not a
 * multiple of `enc->channels`)
 * @retval		<0		Write error `errno` from `fwrite()`
 */
int sstvenc_sunau_enc_write(struct sstvenc_sunau_enc* const enc,
			    size_t n_samples, const double* samples);

/*!
 * Finish writing the file and close it.
 *
 * @param[inout]	enc		SunAU encoder context (to be closed)
 *
 * @retval		0		Success
 * @retval		<0		Write error `errno` from `fwrite()` or
 * `fclose()`.
 */
int sstvenc_sunau_enc_close(struct sstvenc_sunau_enc* const enc);

/*! @} */
#endif
