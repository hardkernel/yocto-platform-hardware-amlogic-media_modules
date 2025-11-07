/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 * new double write api
 * Description:
 */

#ifndef AML_OUTPUT_MODE_DEFINE_H
#define AML_OUTPUT_MODE_DEFINE_H

#include <linux/types.h>

/*
 * Format definitions (enum style with sequential values 16 bit)
 */
enum format_mode {
	/* ---------- YUV format ---------- */
	FORMAT_PIX_Y8        = 1 << 0,  /* V4L2_PIX_FMT_GREY (8-bit grayscale) */
	FORMAT_PIX_NV12      = 1 << 1,  /* V4L2_PIX_FMT_NV12 */
	FORMAT_PIX_NV21      = 1 << 2,  /* V4L2_PIX_FMT_NV21 */
	FORMAT_PIX_NV12M     = 1 << 3,  /* V4L2_PIX_FMT_NV12M */
	FORMAT_PIX_NV21M     = 1 << 4,  /* V4L2_PIX_FMT_NV21M */
	FORMAT_PIX_YUV420    = 1 << 5,  /* V4L2_PIX_FMT_YUV420 */
	FORMAT_PIX_YUV420M   = 1 << 6,  /* V4L2_PIX_FMT_YUV420M */

	FORMAT_P010          = 1 << 10,  /* P010 format */

	/* ---------- compress format ---------- */
	FORMAT_AVBC          = 1 << 15,  /* AVBC format */
};

/*
 * Scalar definitions (enum style with sequential values 16 bit)
 */
enum scalar_mode {
	SCALAR_1_1		= 1 << 0,	/*width x height, 1:1 scaling */
	SCALAR_1_4		= 1 << 1,	/*width/2 x height/2, 1/4 area scaling */
	SCALAR_1_16		= 1 << 2,	/*width/4 x height/4, 1/16 area scaling */
	SCALAR_1_64		= 1 << 3,	/*width/8 x height/8, 1/64 area scaling */

	SCALAR_AUTO_540P	= 1 << 10,	/* <=540p =>1/1, <=1080p =>width/2 x height/2, >1080p =>width/4 x height/4 */
	SCALAR_AUTO_720P_1_4	= 1 << 11,	/* >720p => width/2 x height/2, else => 1/1 */
	SCALAR_AUTO_1080P_1_4	= 1 << 12,	/* >1080p => width/2 x height/2, else => 1/1 */
	SCALAR_AUTO_1080P_1_16 	= 1 << 13,	/* >1080p => width/4 x height/4, else => 1/1 */

};

/*
 * Flags definitions (enum style with sequential values 8 bit)
 */
enum flags_mode {
	FLAG_AVBCD = 1 << 0,         /*AVBCD flag */
};

#define FORMAT_PIX_YUV_MASK \
    (FORMAT_PIX_Y8 | FORMAT_PIX_NV12 | FORMAT_PIX_NV21 | \
     FORMAT_PIX_NV12M | FORMAT_PIX_NV21M | \
     FORMAT_PIX_YUV420 | FORMAT_PIX_YUV420M)

struct output_mode {
	__u32 format;
	__u32 scalar;
	__u32 flags;
	__u32 reserved;
};

struct output_mode_ext {
	u32 fmt;
	u32 double_write_mode;
	struct output_mode modes[3];
	u32 data[3];
};

#endif
