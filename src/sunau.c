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
#define SSTVENC_SUNAU_MAGIC (0x2e736e64u)

#include <assert.h>
#include <libsstvenc/sunau.h>

#ifdef MISSING_ENDIAN_H
/*
 * These implement the missing features from the C library
 * https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/endian.h.html
 */
#include <arpa/inet.h>
static uint16_t htobe16(uint16_t in) { return htons(in); }

static uint32_t htobe32(uint32_t in) { return htonl(in); }

static uint64_t htobe64(uint64_t in) {
	return ((uint64_t)((((uint64_t)htonl(in >> 32)) << 32)
			   | htonl(in & UINT32_MAX)));
}
#else
#include <endian.h>
#endif

/*! SunAU encoder state bit: header is written */
#define SSTVENC_SUNAU_STATE_HEADER (0x0001)

/*! Convert a 32-bit IEEE-754 float to big-endian */
static uint32_t fhtobe32(float in) {
	union {
		float	 f;
		uint32_t ui;
	} tmp;

	tmp.f = in;
	return htobe32(tmp.ui);
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

/*!
 * Write the Sun Audio header to the output file.
 */
static int
sstvenc_sunau_enc_write_header(struct sstvenc_sunau_enc* const enc) {
	uint32_t hdr[7] = {
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
	for (int i = 0; i < 7; i++) {
		/* Byte swap to big-endian */
		hdr[i] = htobe32(hdr[i]);
	}

	/* Write */
	size_t res = fwrite(hdr, sizeof(int32_t), 7, enc->fh);
	if (res < 7) {
		return -errno;
	} else {
		enc->state |= SSTVENC_SUNAU_STATE_HEADER;
		return 0;
	}
}

/*!
 * Write the given samples in signed 8-bit integer format.
 */
static int sstvenc_sunau_write_s8(struct sstvenc_sunau_enc* const enc,
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
static int sstvenc_sunau_write_s16(struct sstvenc_sunau_enc* const enc,
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
static int sstvenc_sunau_write_s32(struct sstvenc_sunau_enc* const enc,
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
static int sstvenc_sunau_write_f32(struct sstvenc_sunau_enc* const enc,
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
static int sstvenc_sunau_write_f64(struct sstvenc_sunau_enc* const enc,
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

int sstvenc_sunau_enc_check(uint32_t sample_rate, uint8_t encoding,
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

int sstvenc_sunau_enc_init_fh(struct sstvenc_sunau_enc* const enc, FILE* fh,
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

int sstvenc_sunau_enc_init(struct sstvenc_sunau_enc* const enc,
			   const char* path, uint32_t sample_rate,
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

int sstvenc_sunau_enc_write(struct sstvenc_sunau_enc* const enc,
			    size_t n_samples, const double* samples) {
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

int sstvenc_sunau_enc_close(struct sstvenc_sunau_enc* const enc) {
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

/*! @} */
