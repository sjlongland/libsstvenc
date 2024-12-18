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

#include <libsstvenc/sequence.h>

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
 * Encoder/decoder context.  Stores the fields necessary to construct the
 * header and the file pointer.
 */
struct sstvenc_sunau {
	/*! Pointer to the open file for reading or writing */
	FILE*	 fh;
	/*! Number of bytes written, stores the size of the header when
	 * reading */
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
 * SSTV sequencer arbitrary audio source.  Reads from the given audio file
 * which is assumed to contain samples at the expected sample rate.
 */
struct sstvenc_sunau_src {
	/*! Sample read buffer pointer */
	double*			       buffer;

	/*! SSTV audio source context */
	struct sstvenc_sequencer_ausrc src;

	/*! Decoder state machine */
	struct sstvenc_sunau	       dec;

	/*! File path. */
	const char*		       path;

	/*! Total size of the buffer */
	uint16_t		       buffer_sz;

	/*! Number of samples present in the buffer */
	uint16_t		       buffer_len;

	/*! Position within the buffer */
	uint16_t		       buffer_ptr;

	/*! Channel selection bitmap */
	uint8_t			       channels;
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
int sstvenc_sunau_check(uint32_t sample_rate, uint8_t encoding,
			uint8_t channels);

/*!
 * Initialise an audio encoder context with an opened file.
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
int sstvenc_sunau_enc_init_fh(struct sstvenc_sunau* const enc, FILE* fh,
			      uint32_t sample_rate, uint8_t encoding,
			      uint8_t channels);

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
int sstvenc_sunau_enc_init(struct sstvenc_sunau* const enc, const char* path,
			   uint32_t sample_rate, uint8_t encoding,
			   uint8_t channels);

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
int sstvenc_sunau_enc_write(struct sstvenc_sunau* const enc, size_t n_samples,
			    const double* samples);

/*!
 * Finish writing the file and close it.
 *
 * @param[inout]	enc		SunAU encoder context (to be closed)
 *
 * @retval		0		Success
 * @retval		<0		Write error `errno` from `fwrite()` or
 * `fclose()`.
 */
int sstvenc_sunau_enc_close(struct sstvenc_sunau* const enc);

/*!
 * Initialise an audio decoder context with an opened file.
 *
 * @param[out]		dec		SunAU decoder context
 * @param[inout]	fh		Existing file handle, open for writing
 * 					in binary mode, positioned at the
 * 					start of the file.
 *
 * @retval		0		Success
 * @retval		-EINVAL		Invalid sample rate, encoding or
 * 					channel count
 */
int sstvenc_sunau_dec_init_fh(struct sstvenc_sunau* const dec, FILE* fh);

/*!
 * Open a file for reading.
 *
 * @param[out]	dec		SunAU decoder context
 * @param[in]	path		Path to the file to open for reading.
 *
 * @retval	0		Success
 * @retval	-EINVAL		Invalid sample rate, encoding or channel count
 * @retval	<0		`-errno` result from `fopen()` call.
 */
int sstvenc_sunau_dec_init(struct sstvenc_sunau* const dec, const char* path);

/*!
 * Reset the file back to the beginning.
 *
 * @param[out]	dec		SunAU decoder context
 *
 * @retval	0		Success
 * @retval	<0		`-errno` result from `fseek()` call.
 */
int sstvenc_sunau_dec_reset(struct sstvenc_sunau* const dec);

/*!
 * Read some audio samples from the file.  n_samples is assumed to be a
 * multiple of the channel count.
 *
 * @param[inout]	enc		SunAU decoder context
 * @param[inout]	n_samples	Number of samples in the buffer, will
 * be updated with the number of samples *actually* read.
 * @param[out]		samples		The samples to be read
 *
 * @retval		0		Success
 * @retval		-EINVAL		Invalid number of samples (not a
 * 					multiple of `dec->channels`)
 * @retval		<0		Read error `errno` from `fread()`
 */
int sstvenc_sunau_dec_read(struct sstvenc_sunau* const enc,
			   size_t* const n_samples, double* samples);

/*!
 * Close the file opened for reading.
 *
 * @param[inout]	dec		SunAU decoder context (to be closed)
 *
 * @retval		0		Success
 * @retval		<0		Write error `errno` from `fclose()`.
 */
int sstvenc_sunau_dec_close(struct sstvenc_sunau* const dec);

/*!
 * Configure a sequencer step that emits an audio recording.
 *
 * @param[out]		step		Sequencer step
 * @param[inout]	src		SunAU decoder state machine.
 * @param[in]		path 		The path to the audio file to play.
 * @param[inout]	buffer		Location to a buffer we can use to
 * 					store samples that have been read from
 * the file.
 * @param[in]		buffer_sz	The size of the buffer provided.
 * @param[in]		channels	The channels to read from the audio
 * 					file.  They will be summed into a mono
 * 					output.  Use UINT8_MAX for all
 * 					channels.
 */
void sstvenc_sequencer_step_sunau(struct sstvenc_sequencer_step* const step,
				  struct sstvenc_sunau_src* const      src,
				  const char* path, double* buffer,
				  uint16_t buffer_sz, uint8_t channels);

/*! @} */
#endif
