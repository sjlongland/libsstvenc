/*!
 * @addtogroup sunau
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

/*!
 * Magic bytes at start of the Sun Audio header.  This is in fact, the ASCII
 * characters ".snd".
 */
#define SSTVENC_SUNAU_MAGIC	(0x2e736e64u)

/*!
 * Size of a Sun Audio header in 32-bit words.
 */
#define SSTVENC_SUNAU_HEADER_SZ (7)

#include <assert.h>
#include <libsstvenc/sunau.h>

#ifdef MISSING_ENDIAN_H
/*
 * These implement the missing features from the C library
 * https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/endian.h.html
 */
#include <arpa/inet.h>
static uint16_t be16toh(uint16_t in) { return ntohs(in); }
static uint16_t htobe16(uint16_t in) { return htons(in); }

static uint32_t be32toh(uint32_t in) { return ntohl(in); }
static uint32_t htobe32(uint32_t in) { return htonl(in); }

static uint64_t be64toh(uint64_t in) {
	return ((uint64_t)((((uint64_t)ntohl(in >> 32)) << 32)
			   | ntohl(in & UINT32_MAX)));
}
static uint64_t htobe64(uint64_t in) {
	return ((uint64_t)((((uint64_t)htonl(in >> 32)) << 32)
			   | htonl(in & UINT32_MAX)));
}
#else
#include <endian.h>
#endif

/*! SunAU encoder state bit: header is written */
#define SSTVENC_SUNAU_STATE_HEADER (0x0001)

/*! SunAU decoder state bit: end of file has been reached */
#define SSTVENC_SUNAU_STATE_EOF	   (0x8000)

/*! Convert a 32-bit IEEE-754 float to big-endian */
static uint32_t fhtobe32(float in) {
	union {
		float	 f;
		uint32_t ui;
	} tmp;

	tmp.f = in;
	return htobe32(tmp.ui);
}

/*! Convert a 32-bit IEEE-754 float from big-endian */
static float fbe32toh(uint32_t in) {
	union {
		float	 f;
		uint32_t ui;
	} tmp;

	tmp.ui = be32toh(in);
	return be32toh(tmp.f);
}

/*! Convert a 64-bit IEEE-754 float to big-endian */
static uint64_t dhtobe64(double in) {
	union {
		double	 f;
		uint64_t ui;
	} tmp;

	tmp.f = in;
	return htobe64(tmp.ui);
}

/*! Convert a 64-bit IEEE-754 float from big-endian */
static double dbe64toh(uint64_t in) {
	union {
		double	 f;
		uint64_t ui;
	} tmp;

	tmp.ui = be64toh(in);
	return be64toh(tmp.f);
}

/*!
 * Initialise the audio source ready for reading samples.  The
 * function should assume the existing state of the context is
 * meaningless.
 *
 * @param[inout]	ausrc	Audio source context
 *
 * @retval		0	Success
 * @retval		<0	An error from `errno.h`, negated.
 */
static int
sstvenc_sunau_src_init(struct sstvenc_sequencer_ausrc* const ausrc);

/*!
 * Reset the audio source back to the initial state.  The function may
 * assume the structure has been initialised already.  Resources can
 * be released if this would be easier than resetting their states.
 *
 * In the event of an error, it is the responsibility of the audio
 * interface to clean up its state and release resources.
 *
 * @param[inout]	ausrc	Audio source context
 *
 * @retval		0	Success
 * @retval		<0	An error from `errno.h`, negated.
 */
static int
sstvenc_sunau_src_reset(struct sstvenc_sequencer_ausrc* const ausrc);

/*!
 * Read and return the next audio sample.  The function may assume the
 * context has been initialised already (that is,
 * sstvenc_sequencer_ausrc_interface#init has been called).
 *
 * When we reach the end, the audio source interface is responsible
 * for freeing/closing resources acquired in the initialisation stage.
 * Calling this function again after this point should be a no-op
 * until sstvenc_sequencer_ausrc_interface#reset is called.
 *
 * In the event of an error, it is the responsibility of the audio
 * interface to clean up its state and release resources.
 *
 * @param[inout]	ausrc	Audio source context
 * @param[out]		sample	The audio sample read
 *
 * @retval		1	Success, a sample has been written.
 * @retval		0	Success, there are no more samples to
 * 				read.
 * @retval		<0	An error from `errno.h`, negated.
 */
static int sstvenc_sunau_src_next(struct sstvenc_sequencer_ausrc* const ausrc,
				  double* const sample);

/*!
 * Close the audio source.  This should release any resources acquired
 * during initialisation.
 *
 * @param[inout]	ausrc	Audio source context
 *
 * @retval		0	Success
 * @retval		<0	An error from `errno.h`, negated.
 */
static int
sstvenc_sunau_src_close(struct sstvenc_sequencer_ausrc* const ausrc);

/*!
 * SSTV sequencer audio stream interface.
 */
const static struct sstvenc_sequencer_ausrc_interface sstvenc_sunau_src_iface
    = {
	.init  = sstvenc_sunau_src_init,
	.reset = sstvenc_sunau_src_reset,
	.next  = sstvenc_sunau_src_next,
	.close = sstvenc_sunau_src_close,
};

/*!
 * Write the Sun Audio header to the output file.
 */
static int sstvenc_sunau_enc_write_header(struct sstvenc_sunau* const enc) {
	uint32_t hdr[SSTVENC_SUNAU_HEADER_SZ] = {
	    SSTVENC_SUNAU_MAGIC, // Magic ".snd"
	    sizeof(hdr),	 // Data offset: 28 bytes (7*4-bytes)
	    UINT32_MAX,		 // Data size: unknown for now
	    enc->encoding,	 // Encoding: int16_t linear
	    enc->sample_rate,	 // Sample rate
	    enc->channels,	 // Channels
	    0,			 // Annotation (unused)
	};

	/* Check we have not written anything yet */
	assert(enc->written_sz == 0);
	assert(!(enc->state & SSTVENC_SUNAU_STATE_HEADER));

	/* Convert to big-endian */
	for (int i = 0; i < SSTVENC_SUNAU_HEADER_SZ; i++) {
		/* Byte swap to big-endian */
		hdr[i] = htobe32(hdr[i]);
	}

	/* Write */
	size_t res
	    = fwrite(hdr, sizeof(int32_t), SSTVENC_SUNAU_HEADER_SZ, enc->fh);
	if (res < SSTVENC_SUNAU_HEADER_SZ) {
		return -errno;
	} else {
		enc->state |= SSTVENC_SUNAU_STATE_HEADER;
		return 0;
	}
}

/*!
 * Write the given samples in signed 8-bit integer format.
 */
static int sstvenc_sunau_write_s8(struct sstvenc_sunau* const enc,
				  size_t n_sample, const double* sample) {
	/* Scale to 8-bit fixed-point */
	int8_t isample[n_sample];
	int    i;

	for (i = 0; i < n_sample; i++) {
		/* Scale */
		isample[i] = INT8_MAX * sample[i];
	}

	size_t sz = fwrite(isample, n_sample, sizeof(int8_t), enc->fh);
	if (sz < n_sample) {
		return -errno;
	} else {
		enc->written_sz += sz * sizeof(int8_t);
		return 0;
	}
}

/*!
 * Write the given samples in signed 16-bit integer format.
 */
static int sstvenc_sunau_write_s16(struct sstvenc_sunau* const enc,
				   size_t n_sample, const double* sample) {
	/* Scale to 16-bit fixed-point */
	int16_t isample[n_sample];
	int	i;

	for (i = 0; i < n_sample; i++) {
		/* Scale */
		isample[i] = INT16_MAX * sample[i];
		/* Byte swap */
		isample[i] = htobe16(isample[i]);
	}

	size_t sz = fwrite(isample, n_sample, sizeof(int16_t), enc->fh);
	if (sz < n_sample) {
		return -errno;
	} else {
		enc->written_sz += sz * sizeof(int16_t);
		return 0;
	}
}

/*!
 * Write the given samples in signed 32-bit integer format.
 */
static int sstvenc_sunau_write_s32(struct sstvenc_sunau* const enc,
				   size_t n_sample, const double* sample) {
	/* Scale to 32-bit fixed-point */
	int32_t isample[n_sample];
	int	i;

	for (i = 0; i < n_sample; i++) {
		/* Scale */
		isample[i] = INT32_MAX * sample[i];
		/* Byte swap */
		isample[i] = htobe32(isample[i]);
	}

	size_t sz = fwrite(isample, n_sample, sizeof(int32_t), enc->fh);
	if (sz < n_sample) {
		return -errno;
	} else {
		enc->written_sz += sz * sizeof(int32_t);
		return 0;
	}
}

/*!
 * Write the given samples in 32-bit IEEE-754 floating-point format.
 */
static int sstvenc_sunau_write_f32(struct sstvenc_sunau* const enc,
				   size_t n_sample, const double* sample) {
	/* Convert to big-endian float */
	int32_t fsample[n_sample];
	int	i;

	for (i = 0; i < n_sample; i++) {
		/* Byte swap */
		fsample[i] = fhtobe32(sample[i]);
	}

	size_t sz = fwrite(fsample, n_sample, sizeof(int32_t), enc->fh);
	if (sz < n_sample) {
		return -errno;
	} else {
		enc->written_sz += sz * sizeof(int32_t);
		return 0;
	}
}

/*!
 * Write the given samples in 64-bit IEEE-754 floating-point format.
 */
static int sstvenc_sunau_write_f64(struct sstvenc_sunau* const enc,
				   size_t n_sample, const double* sample) {
	/* Convert to big-endian float */
	int64_t fsample[n_sample];
	int	i;

	for (i = 0; i < n_sample; i++) {
		/* Byte swap */
		fsample[i] = dhtobe64(sample[i]);
	}

	size_t sz = fwrite(fsample, n_sample, sizeof(int64_t), enc->fh);
	if (sz < n_sample) {
		return -errno;
	} else {
		enc->written_sz += sz * sizeof(int64_t);
		return 0;
	}
}

int sstvenc_sunau_check(uint32_t sample_rate, uint8_t encoding,
			uint8_t channels) {
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

int sstvenc_sunau_enc_init_fh(struct sstvenc_sunau* const enc, FILE* fh,
			      uint32_t sample_rate, uint8_t encoding,
			      uint8_t channels) {
	int res = sstvenc_sunau_check(sample_rate, encoding, channels);
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

int sstvenc_sunau_enc_init(struct sstvenc_sunau* const enc, const char* path,
			   uint32_t sample_rate, uint8_t encoding,
			   uint8_t channels) {
	int res = sstvenc_sunau_check(sample_rate, encoding, channels);
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

int sstvenc_sunau_enc_write(struct sstvenc_sunau* const enc, size_t n_samples,
			    const double* samples) {
	if ((n_samples % enc->channels) != 0) {
		return -EINVAL;
	}

	if (!(enc->state & SSTVENC_SUNAU_STATE_HEADER)) {
		int res = sstvenc_sunau_enc_write_header(enc);
		if (res < 0) {
			return res;
		}
	}

	switch (enc->encoding) {
	case SSTVENC_SUNAU_FMT_S8:
		return sstvenc_sunau_write_s8(enc, n_samples, samples);
	case SSTVENC_SUNAU_FMT_S16:
		return sstvenc_sunau_write_s16(enc, n_samples, samples);
	case SSTVENC_SUNAU_FMT_S32:
		return sstvenc_sunau_write_s32(enc, n_samples, samples);
	case SSTVENC_SUNAU_FMT_F32:
		return sstvenc_sunau_write_f32(enc, n_samples, samples);
	case SSTVENC_SUNAU_FMT_F64:
		return sstvenc_sunau_write_f64(enc, n_samples, samples);
	default:
		assert(0);
		return -EINVAL;
	}
}

int sstvenc_sunau_enc_close(struct sstvenc_sunau* const enc) {
	if (!(enc->state & SSTVENC_SUNAU_STATE_HEADER)) {
		int res = sstvenc_sunau_enc_write_header(enc);
		if (res < 0) {
			return res;
		}
	}

	/* Can we seek in this file? */
	if (fseek(enc->fh, sizeof(uint32_t) * 2, SEEK_SET) == 0) {
		/* We can, write out the *correct* file size */
		uint32_t data_sz = htobe32(enc->written_sz);
		size_t	 res = fwrite(&data_sz, 1, sizeof(uint32_t), enc->fh);
		if (res < sizeof(uint32_t)) {
			/* Write failed, close and bail! */
			int ret = -errno;
			fclose(enc->fh);
			return ret;
		}
	}

	/* If not, no harm done!  We can close the file now. */
	if (fclose(enc->fh) != 0) {
		/* Close failed */
		return -errno;
	} else {
		return 0;
	}
}

int sstvenc_sunau_dec_init_fh(struct sstvenc_sunau* const dec, FILE* fh) {
	uint32_t hdr[SSTVENC_SUNAU_HEADER_SZ];
	int	 res = 0;

	if (fread(hdr, sizeof(hdr), 1, fh) < 1) {
		/* Incomplete or failed read */
		return -errno;
	}

	/* Convert to host-endian format */
	for (uint8_t i = 0; i < SSTVENC_SUNAU_HEADER_SZ; i++) {
		hdr[i] = be32toh(hdr[i]);
	}

	/* Assert we have a valid SunAU header */
	if (hdr[0] != SSTVENC_SUNAU_MAGIC) {
		/* This is not a valid header */
		return -EINVAL;
	}

	/* Extract the header fields we care about */
	dec->encoding	 = hdr[3];
	dec->sample_rate = hdr[4];
	dec->channels	 = hdr[5];

	/* Check these are supported */
	res = sstvenc_sunau_check(dec->sample_rate, dec->encoding,
				  dec->channels);
	if (res < 0) {
		return res;
	}

	/* All good, seek to the spot where the audio begins */
	if (fseek(fh, hdr[1], SEEK_SET) < 0) {
		/* Seek failed */
		return -errno;
	}

	/* All ready */
	dec->fh		= fh;
	dec->written_sz = hdr[1];
	dec->state	= 0;
	return 0;
}

int sstvenc_sunau_dec_init(struct sstvenc_sunau* const dec,
			   const char*		       path) {
	FILE* fh = fopen(path, "rb");
	if (fh == NULL) {
		return -errno;
	}

	int res = sstvenc_sunau_dec_init_fh(dec, fh);
	if (res < 0) {
		/* Try our best, if the close fails, too bad! */
		fclose(fh);
	}

	return res;
}

int sstvenc_sunau_dec_reset(struct sstvenc_sunau* const dec) {
	if (fseek(dec->fh, dec->written_sz, SEEK_SET) < 0) {
		/* Seek failed */
		return -errno;
	} else {
		/* Clear the EOF bit, as we should be back at the start */
		dec->state &= ~SSTVENC_SUNAU_STATE_EOF;
		return 0;
	}
}

static int sstvenc_sunau_read_s8(struct sstvenc_sunau* const dec,
				 size_t* const n_samples, double* samples) {
	/* Use the end of the output buffer as scratch space. */
	int8_t* in_buffer = (int8_t*)(&(samples[*n_samples]))
			    - (sizeof(int8_t) * (*n_samples));
	size_t in_buffer_sz = *n_samples;
	size_t read_sz	    = 0;

	/* Read the raw audio data */
	errno		    = 0;
	read_sz = fread(in_buffer, sizeof(int8_t), in_buffer_sz, dec->fh);
	if (read_sz < in_buffer_sz) {
		/* Short read, check for read errors */
		if (errno != 0) {
			return -errno;
		} else {
			dec->state |= SSTVENC_SUNAU_STATE_EOF;
		}
	}

	/* Read is successful, write sample count, convert samples to
	 * double-precision float */
	*n_samples = read_sz;
	while (read_sz) {
		*samples = -(double)(*in_buffer) / (double)INT8_MIN;
		samples++;
		in_buffer++;
		read_sz--;
	}

	return 0;
}

static int sstvenc_sunau_read_s16(struct sstvenc_sunau* const dec,
				  size_t* const n_samples, double* samples) {
	/* Use the end of the output buffer as scratch space. */
	int16_t* in_buffer = (int16_t*)(&(samples[*n_samples]))
			     - (sizeof(int16_t) * (*n_samples));
	size_t in_buffer_sz = *n_samples;
	size_t read_sz	    = 0;

	/* Read the raw audio data */
	errno		    = 0;
	read_sz = fread(in_buffer, sizeof(int16_t), in_buffer_sz, dec->fh);
	if (read_sz < in_buffer_sz) {
		/* Short read, check for read errors */
		if (errno != 0) {
			return -errno;
		} else {
			dec->state |= SSTVENC_SUNAU_STATE_EOF;
		}
	}

	/* Read is successful, write sample count, convert samples to
	 * double-precision float */
	*n_samples = read_sz;
	while (read_sz) {
		int16_t host_endian = be16toh(*in_buffer);

		*samples = -(double)(host_endian) / (double)INT16_MIN;
		samples++;
		in_buffer++;
		read_sz--;
	}

	return 0;
}

static int sstvenc_sunau_read_s32(struct sstvenc_sunau* const dec,
				  size_t* const n_samples, double* samples) {
	/* Use the end of the output buffer as scratch space. */
	int32_t* in_buffer = (int32_t*)(&(samples[*n_samples]))
			     - (sizeof(int32_t) * (*n_samples));
	size_t in_buffer_sz = *n_samples;
	size_t read_sz	    = 0;

	/* Read the raw audio data */
	errno		    = 0;
	read_sz = fread(in_buffer, sizeof(int32_t), in_buffer_sz, dec->fh);
	if (read_sz < in_buffer_sz) {
		/* Short read, check for read errors */
		if (errno != 0) {
			return -errno;
		} else {
			dec->state |= SSTVENC_SUNAU_STATE_EOF;
		}
	}

	/* Read is successful, write sample count, convert samples to
	 * double-precision float */
	*n_samples = read_sz;
	while (read_sz) {
		int32_t host_endian = be32toh(*in_buffer);

		*samples = -(double)(host_endian) / (double)INT32_MIN;
		samples++;
		in_buffer++;
		read_sz--;
	}

	return 0;
}

static int sstvenc_sunau_read_f32(struct sstvenc_sunau* const dec,
				  size_t* const n_samples, double* samples) {
	/* Use the end of the output buffer as scratch space. */
	uint32_t* in_buffer = (uint32_t*)(&(samples[*n_samples]))
			      - (sizeof(uint32_t) * (*n_samples));
	size_t in_buffer_sz = *n_samples;
	size_t read_sz	    = 0;

	/* Read the raw audio data */
	errno		    = 0;
	read_sz = fread(in_buffer, sizeof(float), in_buffer_sz, dec->fh);
	if (read_sz < in_buffer_sz) {
		/* Short read, check for read errors */
		if (errno != 0) {
			return -errno;
		} else {
			dec->state |= SSTVENC_SUNAU_STATE_EOF;
		}
	}

	/* Read is successful, write sample count, convert samples to native
	 * endianness */
	*n_samples = read_sz;
	while (read_sz) {
		*samples = fbe32toh(*in_buffer);
		samples++;
		in_buffer++;
		read_sz--;
	}

	return 0;
}

static int sstvenc_sunau_read_f64(struct sstvenc_sunau* const dec,
				  size_t* const n_samples, double* samples) {
	/* Use the output buffer as scratch space */
	uint64_t* in_buffer    = (uint64_t*)samples;
	size_t	  in_buffer_sz = *n_samples;
	size_t	  read_sz      = 0;

	/* Read the raw audio data */
	errno		       = 0;
	read_sz = fread(in_buffer, sizeof(double), in_buffer_sz, dec->fh);
	if (read_sz < in_buffer_sz) {
		/* Short read, check for read errors */
		if (errno != 0) {
			return -errno;
		} else {
			dec->state |= SSTVENC_SUNAU_STATE_EOF;
		}
	}

	/* Read is successful, write sample count, convert samples to native
	 * endianness */
	*n_samples = read_sz;
	while (read_sz) {
		*samples = dbe64toh(*in_buffer);
		samples++;
		in_buffer++;
		read_sz--;
	}

	return 0;
}

int sstvenc_sunau_dec_read(struct sstvenc_sunau* const dec,
			   size_t* const n_samples, double* samples) {
	switch (dec->encoding) {
	case SSTVENC_SUNAU_FMT_S8:
		return sstvenc_sunau_read_s8(dec, n_samples, samples);
	case SSTVENC_SUNAU_FMT_S16:
		return sstvenc_sunau_read_s16(dec, n_samples, samples);
	case SSTVENC_SUNAU_FMT_S32:
		return sstvenc_sunau_read_s32(dec, n_samples, samples);
	case SSTVENC_SUNAU_FMT_F32:
		return sstvenc_sunau_read_f32(dec, n_samples, samples);
	case SSTVENC_SUNAU_FMT_F64:
		return sstvenc_sunau_read_f64(dec, n_samples, samples);
	default:
		assert(0);
		return -EINVAL;
	}
}

int sstvenc_sunau_dec_close(struct sstvenc_sunau* const dec) {
	int res = fclose(dec->fh);
	dec->fh = NULL;

	if (res < 0) {
		return -errno;
	} else {
		return 0;
	}
}

void sstvenc_sequencer_step_sunau(struct sstvenc_sequencer_step* const step,
				  struct sstvenc_sunau_src* const      src,
				  const char* path, double* buffer,
				  uint16_t buffer_sz, uint8_t channels) {
	src->src.iface	 = &sstvenc_sunau_src_iface;
	src->src.context = (void*)src;
	src->path	 = path;
	src->channels	 = channels;
	src->buffer	 = buffer;
	src->buffer_sz	 = buffer_sz;
}

static int
sstvenc_sunau_src_init(struct sstvenc_sequencer_ausrc* const ausrc) {
	struct sstvenc_sunau_src* const src
	    = (struct sstvenc_sunau_src*)(ausrc->context);
	int res = sstvenc_sunau_dec_init(&(src->dec), src->path);
	if (res == 0) {
		src->buffer_ptr = 0;
		src->buffer_len = 0;
	}
	return res;
}

static int
sstvenc_sunau_src_reset(struct sstvenc_sequencer_ausrc* const ausrc) {
	struct sstvenc_sunau_src* const src
	    = (struct sstvenc_sunau_src*)(ausrc->context);
	int res = sstvenc_sunau_dec_reset(&(src->dec));
	if (res == 0) {
		src->buffer_ptr = 0;
		src->buffer_len = 0;
	}
	return res;
}

static int sstvenc_sunau_src_next(struct sstvenc_sequencer_ausrc* const ausrc,
				  double* const sample) {
	struct sstvenc_sunau_src* const src
	    = (struct sstvenc_sunau_src*)(ausrc->context);

	double	output = 0.0;
	uint8_t count  = 0;

	for (uint8_t ch = 0; ch < src->dec.channels; ch++) {
		if (src->buffer_ptr >= src->buffer_len) {
			size_t sz = src->buffer_sz;

			if (src->dec.state & SSTVENC_SUNAU_STATE_EOF) {
				/* There is nothing more to read */
				if (count) {
					*sample = output / (double)count;
				}

				if (src->dec.fh) {
					return sstvenc_sunau_dec_close(
					    &(src->dec));
				} else {
					return 0;
				}
			}

			int res = sstvenc_sunau_dec_read(&(src->dec), &sz,
							 src->buffer);
			if (res < 0) {
				/* Read failed, attempt to close the file */
				sstvenc_sunau_dec_close(&(src->dec));
				return res;
			}

			/* Read succeeded */
			src->buffer_len = sz;
			src->buffer_ptr = 0;

			if (sz == 0) {
				/* There was nothing read, this is it */
				if (count) {
					*sample = output / (double)count;
				}

				return sstvenc_sunau_dec_close(&(src->dec));
			}
		}

		if (src->channels & (1 << ch)) {
			output += src->buffer[src->buffer_ptr];
			count++;
		}

		src->buffer_ptr++;
	}

	if (count) {
		*sample = output / (double)count;
		return 1;
	} else {
		return 0;
	}
}

static int
sstvenc_sunau_src_close(struct sstvenc_sequencer_ausrc* const ausrc) {
	struct sstvenc_sunau_src* const src
	    = (struct sstvenc_sunau_src*)(ausrc->context);
	return sstvenc_sunau_dec_close(&(src->dec));
}

/*! @} */
