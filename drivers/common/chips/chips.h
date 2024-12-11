/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef DOS_CHIP_PLATFORM_HEADER
#define DOS_CHIP_PLATFORM_HEADER

#include <linux/amlogic/media/utils/vformat.h>

/* g12a ~ sc2 */
#define	EFUSE_LIC0	(0xc)
#define	EFUSE_LIC1	(0xd)
#define	EFUSE_LIC2	(0xe)
#define	EFUSE_LIC3	(0xf)

bool check_efuse_chip(int vformat);

/* fmt_support */
//vdec
#define FMT_MPEG2    BIT(VFORMAT_MPEG12)
#define FMT_MPEG4    BIT(VFORMAT_MPEG4)
#define FMT_H264     BIT(VFORMAT_H264)
#define FMT_MJPEG    BIT(VFORMAT_MJPEG)
#define FMT_VC1      BIT(VFORMAT_VC1)
#define FMT_AVS      BIT(VFORMAT_AVS)
#define FMT_MVC      BIT(VFORMAT_H264MVC)
//hevc
#define FMT_HEVC     BIT(VFORMAT_HEVC)
#define FMT_VP9      BIT(VFORMAT_VP9)
#define FMT_AVS2     BIT(VFORMAT_AVS2)
#define FMT_AV1      BIT(VFORMAT_AV1)
#define FMT_AVS3     BIT(VFORMAT_AVS3)
#define FMT_H266     BIT(VFORMAT_H266)
#define FMT_AVBCD    BIT(VFORMAT_AVBCD)

//hcodec
#define FMT_H264_ENC BIT(VFORMAT_H264_ENC)
#define FMT_JPEG_ENC BIT(VFORMAT_JPEG_ENC)

//frequently-used combination
#define FMT_VDEC_ALL               (FMT_MPEG2 | FMT_MPEG4 | FMT_H264 | FMT_MJPEG | FMT_VC1 | FMT_MVC | FMT_AVS)
#define FMT_VDEC_NO_AVS            (FMT_MPEG2 | FMT_MPEG4 | FMT_H264 | FMT_MJPEG | FMT_VC1 | FMT_MVC)

#define FMT_HEVC_VP9_AV1           (FMT_HEVC | FMT_VP9 | FMT_AV1 | FMT_AVBCD)
#define FMT_HEVC_VP9_AVS2          (FMT_HEVC | FMT_VP9 | FMT_AVS2 | FMT_AVBCD)
#define FMT_HEVC_VP9_AVS2_AV1      (FMT_AV1  | FMT_HEVC_VP9_AVS2)
#define FMT_HEVC_VP9_AVS2_AV1_AVS3 (FMT_AVS3 | FMT_HEVC_VP9_AVS2_AV1)


/* profile & level description */
#define PRO_LEVEL_LEN  64

struct profile_level_t {
	u32 fmt_level[VFORMAT_MAX];
	char profile_level_desc[VFORMAT_MAX][PRO_LEVEL_LEN];
};

void vcodec_profile_level_init(struct profile_level_t *plt);

void show_profile_level_idc(struct profile_level_t *plt);

#endif
