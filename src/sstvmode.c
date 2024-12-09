/*!
 * @addtogroup sstvmode
 * @{
 */

/*
 * © Stuart Longland VK4MSL
 * SPDX-License-Identifier: MIT
 */

#include <libsstvenc/sstvfreq.h>
#include <libsstvenc/sstvmode.h>

/* SSTV mode specifications -- Robot B/W modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_robotbw_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 7000000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Robot colour modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_robot36_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 9000000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 3000000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_robot36_gap[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 4500000},
    {.frequency = SSTVENC_FREQ_VIS_START, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_robot72_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 9000000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 3000000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_robot72_gap01[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 4500000},
    {.frequency = SSTVENC_FREQ_VIS_START, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_robot72_gap12[] = {
    {.frequency = SSTVENC_FREQ_WHITE, .duration_ns = 4500000},
    {.frequency = SSTVENC_FREQ_VIS_START, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Scottie modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_scottie_fp[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_scottie_sep01[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 3000000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_scottie_sep12[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1500000},
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 9000000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_scottie_bp[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1500000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Martin modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_martin_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 4862000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 572000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_martin_sep[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 572000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Pasokon modes */

#define SSTVENC_PASOKON_P3_TIMEUNIT (1000000000.0 / 4800.0)
#define SSTVENC_PASOKON_P5_TIMEUNIT (1000000000.0 / 3200.0)
#define SSTVENC_PASOKON_P7_TIMEUNIT (1000000000.0 / 2400.0)

#define SSTVENC_PASOKON_SYNC(unit)  (25 * unit)
#define SSTVENC_PASOKON_GAP(unit)   (5 * unit)
#define SSTVENC_PASOKON_SCAN(unit)  (640 * unit)

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p3_fp[] = {
    {.frequency	  = SSTVENC_FREQ_SYNC,
     .duration_ns = SSTVENC_PASOKON_SYNC(SSTVENC_PASOKON_P3_TIMEUNIT)},
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P3_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p3_sep[] = {
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P3_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p5_fp[] = {
    {.frequency	  = SSTVENC_FREQ_SYNC,
     .duration_ns = SSTVENC_PASOKON_SYNC(SSTVENC_PASOKON_P5_TIMEUNIT)},
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P5_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p5_sep[] = {
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P5_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p7_fp[] = {
    {.frequency	  = SSTVENC_FREQ_SYNC,
     .duration_ns = SSTVENC_PASOKON_SYNC(SSTVENC_PASOKON_P7_TIMEUNIT)},
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P7_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_pasokon_p7_sep[] = {
    {.frequency	  = SSTVENC_FREQ_BLACK,
     .duration_ns = SSTVENC_PASOKON_GAP(SSTVENC_PASOKON_P7_TIMEUNIT)},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- PD modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_pd_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 20000000},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 2080000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- Wraase SC-2 modes */
const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_180_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 5522500},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 500000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_120_fp[] = {
    {.frequency = SSTVENC_FREQ_SYNC, .duration_ns = 5522500},
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 1000000},
    {.frequency = 0, .duration_ns = 0},
};

const static struct sstvenc_encoder_pulse sstvenc_sstv_wraasesc2_sep[] = {
    {.frequency = SSTVENC_FREQ_BLACK, .duration_ns = 500000},
    {.frequency = 0, .duration_ns = 0},
};

/* SSTV mode specifications -- the table */
const static struct sstvenc_mode sstvenc_sstv_modes[] = {
    /* Robot B/W modes */
    {
	.description	    = "Robot 8 B/W",
	.name		    = "R8BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {59900000, 0, 0},
	.width		    = 160,
	.height		    = 120,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_NONE,
	    SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x02,
    },
    {
	.description	    = "Robot 12 B/W",
	.name		    = "R12BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {93000000, 0, 0},
	.width		    = 160,
	.height		    = 120,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_NONE,
	    SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x86,
    },
    {
	.description	    = "Robot 24 B/W",
	.name		    = "R24BW",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robotbw_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {93000000, 0, 0},
	.width		    = 320,
	.height		    = 240,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_MONO, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_NONE,
	    SSTVENC_CSO_CH_NONE, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x0a,
    },
    /* Robot colour modes */
    {
	.description	    = "Robot 36",
	.name		    = "R36",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robot36_fp,
	.gap01		    = sstvenc_sstv_robot36_gap,
	.gap12		    = sstvenc_sstv_robot36_fp,
	.gap23		    = sstvenc_sstv_robot36_gap,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {88000000, 44000000, 88000000, 44000000},
	.width		    = 320,
	.height		    = 240,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_Y2, SSTVENC_CSO_CH_V),
	.vis_code = 0x08,
    },
    {
	.description	    = "Robot 72",
	.name		    = "R72",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_robot72_fp,
	.gap01		    = sstvenc_sstv_robot72_gap01,
	.gap12		    = sstvenc_sstv_robot72_gap12,
	.gap23		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {138000000, 69000000, 69000000},
	.width		    = 320,
	.height		    = 240,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x0c,
    },
    /* Scottie modes */
    {
	.description	    = "Scottie S1",
	.name		    = "S1",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_scottie_fp,
	.gap01		    = sstvenc_sstv_scottie_sep01,
	.gap12		    = sstvenc_sstv_scottie_sep12,
	.backporch	    = sstvenc_sstv_scottie_bp,
	.finalseq	    = NULL,
	.scanline_period_ns = {136740000, 136740000, 136740000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B,
	    SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x3c,
    },
    {
	.description	    = "Scottie S2",
	.name		    = "S2",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_scottie_fp,
	.gap01		    = sstvenc_sstv_scottie_sep01,
	.gap12		    = sstvenc_sstv_scottie_sep12,
	.backporch	    = sstvenc_sstv_scottie_bp,
	.finalseq	    = NULL,
	.scanline_period_ns = {86564000, 86564000, 86564000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B,
	    SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x38,
    },
    {
	.description	    = "Scottie DX",
	.name		    = "SDX",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_scottie_fp,
	.gap01		    = sstvenc_sstv_scottie_sep01,
	.gap12		    = sstvenc_sstv_scottie_sep12,
	.backporch	    = sstvenc_sstv_scottie_bp,
	.finalseq	    = NULL,
	.scanline_period_ns = {344100000, 344100000, 344100000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B,
	    SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x4c,
    },
    /* Martin modes */
    {
	.description	    = "Martin M1",
	.name		    = "M1",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_martin_fp,
	.gap01		    = sstvenc_sstv_martin_sep,
	.gap12		    = sstvenc_sstv_martin_sep,
	.backporch	    = sstvenc_sstv_martin_sep,
	.finalseq	    = NULL,
	.scanline_period_ns = {146432000, 146432000, 146432000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B,
	    SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x2c,
    },
    {
	.description	    = "Martin M2",
	.name		    = "M2",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_martin_fp,
	.gap01		    = sstvenc_sstv_martin_sep,
	.gap12		    = sstvenc_sstv_martin_sep,
	.backporch	    = sstvenc_sstv_martin_sep,
	.finalseq	    = NULL,
	.scanline_period_ns = {73216000, 73216000, 73216000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_G, SSTVENC_CSO_CH_B,
	    SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x28,
    },
    /* Pasokon modes */
    {
	.description = "Pasokon P3",
	.name	     = "P3",
	.initseq     = NULL,
	.frontporch  = sstvenc_sstv_pasokon_p3_fp,
	.gap01	     = sstvenc_sstv_pasokon_p3_sep,
	.gap12	     = sstvenc_sstv_pasokon_p3_sep,
	.backporch   = sstvenc_sstv_pasokon_p3_sep,
	.finalseq    = NULL,
	.scanline_period_ns
	= {SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P3_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P3_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P3_TIMEUNIT)},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x71,
    },
    {
	.description = "Pasokon P5",
	.name	     = "P5",
	.initseq     = NULL,
	.frontporch  = sstvenc_sstv_pasokon_p5_fp,
	.gap01	     = sstvenc_sstv_pasokon_p5_sep,
	.gap12	     = sstvenc_sstv_pasokon_p5_sep,
	.backporch   = sstvenc_sstv_pasokon_p5_sep,
	.finalseq    = NULL,
	.scanline_period_ns
	= {SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P5_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P5_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P5_TIMEUNIT)},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x72,
    },
    {
	.description = "Pasokon P7",
	.name	     = "P7",
	.initseq     = NULL,
	.frontporch  = sstvenc_sstv_pasokon_p7_fp,
	.gap01	     = sstvenc_sstv_pasokon_p7_sep,
	.gap12	     = sstvenc_sstv_pasokon_p7_sep,
	.backporch   = sstvenc_sstv_pasokon_p7_sep,
	.finalseq    = NULL,
	.scanline_period_ns
	= {SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P7_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P7_TIMEUNIT),
	   SSTVENC_PASOKON_SCAN(SSTVENC_PASOKON_P7_TIMEUNIT)},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x73,
    },
    /* PD modes */
    {
	.description	    = "PD-50",
	.name		    = "PD50",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {91520000, 91520000, 91520000, 91520000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x5d,
    },
    {
	.description	    = "PD-90",
	.name		    = "PD90",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {170240000, 170240000, 170240000, 170240000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x63,
    },
    {
	.description	    = "PD-120",
	.name		    = "PD120",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {121600000, 121600000, 121600000, 121600000},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x5f,
    },
    {
	.description	    = "PD-160",
	.name		    = "PD160",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {195584000, 195584000, 195584000, 195584000},
	.width		    = 512,
	.height		    = 400,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x62,
    },
    {
	.description	    = "PD-180",
	.name		    = "PD180",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {183040000, 183040000, 183040000, 183040000},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x60,
    },
    {
	.description	    = "PD-240",
	.name		    = "PD240",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {244480000, 244480000, 244480000, 244480000},
	.width		    = 640,
	.height		    = 496,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x61,
    },
    {
	.description	    = "PD-290",
	.name		    = "PD290",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_pd_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {228800000, 228800000, 228800000, 228800000},
	.width		    = 800,
	.height		    = 616,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_YUV2, SSTVENC_CSO_CH_Y, SSTVENC_CSO_CH_U,
	    SSTVENC_CSO_CH_V, SSTVENC_CSO_CH_Y2),
	.vis_code = 0x5e,
    },
    /*
     * Wraase SC-2 modes
     *
     * Separator is needed for W260 and W2120, but not W2180 in QSSTV and slowrx.
     * W2180 has a half-length pulse in the front porch sequence.
     */
    {
	.description	    = "Wraase SC-2 60",
	.name		    = "W260",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_wraasesc2_120_fp,
	.gap01		    = sstvenc_sstv_wraasesc2_sep,
	.gap12		    = sstvenc_sstv_wraasesc2_sep,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {77627500, 77627500, 77627500},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0xbb,
    },
    {
	.description	    = "Wraase SC-2 120",
	.name		    = "W2120",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_wraasesc2_120_fp,
	.gap01		    = sstvenc_sstv_wraasesc2_sep,
	.gap12		    = sstvenc_sstv_wraasesc2_sep,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {155985000, 155985000, 155985000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x3f,
    },
    {
	.description	    = "Wraase SC-2 180",
	.name		    = "W2180",
	.initseq	    = NULL,
	.frontporch	    = sstvenc_sstv_wraasesc2_180_fp,
	.gap01		    = NULL,
	.gap12		    = NULL,
	.backporch	    = NULL,
	.finalseq	    = NULL,
	.scanline_period_ns = {235000000, 235000000, 235000000},
	.width		    = 320,
	.height		    = 256,
	.colour_space_order = SSTVENC_MODE_ORDER(
	    SSTVENC_CSO_MODE_RGB, SSTVENC_CSO_CH_R, SSTVENC_CSO_CH_G,
	    SSTVENC_CSO_CH_B, SSTVENC_CSO_CH_NONE),
	.vis_code = 0x37,
    },
};

uint8_t sstvenc_get_mode_count() {
	return sizeof(sstvenc_sstv_modes) / sizeof(sstvenc_sstv_modes[0]);
}

const struct sstvenc_mode* sstvenc_get_mode_by_idx(uint8_t idx) {
	if (idx < sstvenc_get_mode_count()) {
		return &sstvenc_sstv_modes[idx];
	} else {
		return NULL;
	}
}

const struct sstvenc_mode* sstvenc_get_mode_by_name(const char* name) {
	for (uint8_t idx = 0; idx < sstvenc_get_mode_count(); idx++) {
		if (!strcmp(sstvenc_sstv_modes[idx].name, name)) {
			return &sstvenc_sstv_modes[idx];
		}
	}
	return NULL;
}

/*! @} */
