/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/amlogic/media/utils/vformat.h>
#include "../../../common/chips/chips.h"
#include "../../../common/chips/decoder_cpu_ver_info.h"
#include "vdec_dw.h"

enum vdec_dec_mode {
	DM_INVALID            = 0,
	DM_AVBC_ONLY          = 0,
	DM_YUV_1_1_AVBC       = 1,
	DM_YUV_1_4_AVBC_A     = 2,
	DM_YUV_1_4_AVBC_B     = 3,
	DM_YUV_1_2_AVBC       = 4,
	DM_YUV_1_8_AVBC       = 8,
	DM_YUV_ONLY           = 0x10,
	DM_AVBC_1_1           = 0x21,
	DM_AVBC_1_4           = 0x22,
	DM_AVBC_1_2           = 0x24,
	DM_YUV_1_1_10BIT_AVBC = 0x10001,
	DM_YUV_1_4_10BIT_AVBC = 0x10003,
	DM_YUV_1_2_10BIT_AVBC = 0x10004,
	DM_YUV_1_8_10BIT_AVBC = 0x10008,
};

static const char * const format_name[] = {
	"MPEG12",
	"MPEG4",
	"H264",
	"MJPEG",
	"REAL",
	"JPEG",
	"VC1",
	"AVS",
	"YUV",
	"H264MVC",
	"H264_4K2K",
	"HEVC",
	"H264_ENC",
	"JPEG_ENC",
	"VP9",
	"AVS2",
	"AV1",
	"AVS3",
	"H266",
	"AVBCD"
};

static bool is_only_support_yuv_fmt(int format)
{
	switch (format) {
		case VFORMAT_MPEG12:
		case VFORMAT_MPEG4:
		case VFORMAT_MJPEG:
		case VFORMAT_VC1:
		case VFORMAT_AVS:
			return true;
		default:
			return false;
	}
}

static bool is_unsupport_dw_format(int format)
{
	if (!is_support_format(format))
		return true;

	switch (format) {
		case VFORMAT_JPEG:
		case VFORMAT_H264MVC:
		case VFORMAT_AVBCD:
			return true;
		default:
			return false;
	}
}

static struct dos_hw_dw_mode chip_dw_mode[VFORMAT_MAX];
static struct dos_hw_output_mode chip_output_mode[VFORMAT_MAX];

struct dos_hw_dw_mode* get_dos_dw_mode_all(void)
{
	return chip_dw_mode;
}
EXPORT_SYMBOL(get_dos_dw_mode_all);

int *get_dos_dw_mode(enum vformat_e fmt)
{
	if (!is_unsupport_dw_format(fmt))
		return chip_dw_mode[fmt].hw_dw_mode;
	else
		return NULL;
}
EXPORT_SYMBOL(get_dos_dw_mode);

struct dos_hw_output_mode* get_dos_output_mode_all(void)
{
	return chip_output_mode;
}
EXPORT_SYMBOL(get_dos_output_mode_all);

struct map_item {
	unsigned int bit;
	int dm;
	enum scalar_mode scalar;
	enum vformat_e fmt;
};

/* table entries: add/change here to extend behavior */
static const struct map_item map_table[] = {
	{ YUV_1_1, DM_YUV_1_1_AVBC,   SCALAR_1_1,  VFORMAT_MAX },
	{ YUV_1_2, DM_YUV_1_2_AVBC,   SCALAR_1_4,  VFORMAT_MAX },
	{ YUV_1_4, DM_YUV_1_4_AVBC_A, SCALAR_1_16, VFORMAT_MAX },
	{ YUV_1_4, DM_YUV_1_4_AVBC_B, SCALAR_1_16, VFORMAT_MAX },
	{ YUV_1_8, DM_YUV_1_8_AVBC,   SCALAR_1_64, VFORMAT_MAX },

	{ AVBC_1_1, DM_AVBC_1_1, SCALAR_1_1,  VFORMAT_MAX },
	{ AVBC_1_2, DM_AVBC_1_2, SCALAR_1_4,  VFORMAT_MAX },
	{ AVBC_1_4, DM_AVBC_1_4, SCALAR_1_16, VFORMAT_MAX },

	{ P010_1_1, DM_YUV_1_1_10BIT_AVBC, SCALAR_1_1,  VFORMAT_MAX },
	{ P010_1_2, DM_YUV_1_2_10BIT_AVBC, SCALAR_1_4,  VFORMAT_MAX },
	{ P010_1_4, DM_YUV_1_4_10BIT_AVBC, SCALAR_1_16, VFORMAT_MAX },
	{ P010_1_8, DM_YUV_1_8_10BIT_AVBC, SCALAR_1_64, VFORMAT_MAX },

	{ AV1_YUV_ONLY,  DM_YUV_ONLY, SCALAR_1_1, VFORMAT_AV1 },
	{ H266_YUV_ONLY, DM_YUV_ONLY, SCALAR_1_1, VFORMAT_H266 },
};

static void push_mode_if_not_present(struct dos_hw_dw_mode *dw_mode,
	int fmt_index, int dm)
{
	int i;

	for (i = 0; i < 32; i++) {
		/* already present */
		if (dw_mode[fmt_index].hw_dw_mode[i] == dm)
			return;

		/* found insertion slot */
		if (dw_mode[fmt_index].hw_dw_mode[i] == -1) {
			dw_mode[fmt_index].hw_dw_mode[i] = dm;
			break;
		}
	}
}

void fill_dw_mode_from_hw_flag(struct dos_hw_dw_mode *dw_mode,
	int fmt_index, unsigned int hw_flag, unsigned int mask)
{
	int map_n, i;

	/* walk mapping table */
	map_n = sizeof(map_table) / sizeof(map_table[0]);

	for (i = 0; i < map_n; i++) {
		if (!(hw_flag & map_table[i].bit) ||
			(mask && !(mask & map_table[i].bit)))
			continue; /* hw doesn't support this bit */

		if ((map_table[i].fmt == VFORMAT_MAX) ||
			(map_table[i].fmt == dw_mode[fmt_index].fmt))
			push_mode_if_not_present(dw_mode, fmt_index, map_table[i].dm);
	}
}

void fill_output_mode_from_hw_flag(struct dos_hw_output_mode *output_mode,
	int fmt_index, unsigned int hw_flag, unsigned int mask)
{
	int map_n, i;

	/* walk mapping table */
	map_n = sizeof(map_table) / sizeof(map_table[0]);

	output_mode[fmt_index].mode[0].format = FORMAT_PIX_NV12;
	output_mode[fmt_index].mode[1].format = FORMAT_AVBC;
	if (is_support_p010_mode() && !mask)
		output_mode[fmt_index].mode[2].format = FORMAT_P010;

	for (i = 0; i < map_n; i++) {
		if (!(hw_flag & map_table[i].bit) ||
			(mask && !(mask & map_table[i].bit)))
			continue; /* hw doesn't support this bit */

		if ((map_table[i].fmt == VFORMAT_MAX) ||
			(map_table[i].fmt == output_mode[fmt_index].fmt)) {
			if (map_table[i].bit & YUV_BIT_MASK) {
				output_mode[fmt_index].mode[0].scalar |= map_table[i].scalar;

				if (map_table[i].bit & YUV_1_1) {
					output_mode[fmt_index].mode[1].scalar |= map_table[i].scalar;
					output_mode[fmt_index].mode[1].flags |= FLAG_AVBCD;
				}
			} else if (map_table[i].bit & AVBC_BIT_MASK) {
				output_mode[fmt_index].mode[1].scalar |= map_table[i].scalar;
				output_mode[fmt_index].mode[1].flags |= FLAG_AVBCD;
			} else if (map_table[i].bit & P010_BIT_MASK) {
				output_mode[fmt_index].mode[2].scalar |= map_table[i].scalar;
				output_mode[fmt_index].mode[2].flags |= FLAG_AVBCD;
			}
		}
	}
}

static const char* format_to_str(u32 format)
{
	switch (format) {
		case FORMAT_PIX_NV12:
			return "FORMAT_PIX_NV12";
		case FORMAT_AVBC:
			return "FORMAT_AVBC";
		case FORMAT_P010:
			return "FORMAT_P010";
		default:
			return "";
	}
}

static void print_scalar(u32 scalar)
{
	if (scalar & SCALAR_1_1) {
		pr_cont("SCALAR_1_1");
	}
	if (scalar & SCALAR_1_4) {
		pr_cont(" | SCALAR_1_4");
	}
	if (scalar & SCALAR_1_16) {
		pr_cont(" | SCALAR_1_16");
	}
	if (scalar & SCALAR_1_64) {
		pr_cont(" | SCALAR_1_64");
	}
}

void pr_dw_infos(void)
{
	int fmt;
	int i;
	struct dos_hw_dw_mode *dw_mode = get_dos_dw_mode_all();
	struct dos_hw_output_mode *output_mode = get_dos_output_mode_all();

	for (fmt = 0; fmt < VFORMAT_MAX; fmt++) {
		if (is_unsupport_dw_format(fmt))
			continue;

		pr_cont("%s\n", format_name[fmt]);

		pr_cont("DoubleWrite  : [");
		for (i = 0; i < 32; i++) {
			if (dw_mode[fmt].hw_dw_mode[i] != -1)
				pr_cont(" 0x%x", dw_mode[fmt].hw_dw_mode[i]);
		}
		pr_cont(" ]\n");

		pr_cont("OutputMode   :\n");
		for (i = 0; i < 3; i++) {
			pr_cont("    format   : %s\n",
				format_to_str(output_mode[fmt].mode[i].format));

			pr_cont("    scalar   : ");
			print_scalar(output_mode[fmt].mode[i].scalar);

			pr_cont("\n    flag     : %s\n\n",
				(output_mode[fmt].mode[i].flags & FLAG_AVBCD) ? "FLAG_AVBCD" : "");
		}
	}
}

static void dos_dw_mode_param_init(void)
{
	int fmt;
	struct dos_hw_dw_mode *dw_mode = get_dos_dw_mode_all();
	struct dos_hw_output_mode *output_mode = get_dos_output_mode_all();

	for (fmt = 0; fmt < VFORMAT_MAX; fmt++) {
		dw_mode[fmt].fmt = fmt;
		output_mode[fmt].fmt = fmt;
		memset(dw_mode[fmt].hw_dw_mode, -1, sizeof(dw_mode[fmt].hw_dw_mode));

		switch (fmt) {
			case VFORMAT_MPEG12:
			case VFORMAT_MPEG4:
			case VFORMAT_H264:
			case VFORMAT_MJPEG:
			case VFORMAT_VC1:
			case VFORMAT_AVS:
				dw_mode[fmt].hw_dw_mode[0] = DM_YUV_ONLY;
				output_mode[fmt].mode[0].format = FORMAT_PIX_NV12;
				output_mode[fmt].mode[0].scalar = SCALAR_1_1;
				break;
			case VFORMAT_HEVC:
			case VFORMAT_VP9:
				dw_mode[fmt].hw_dw_mode[0] = DM_AVBC_ONLY;
				dw_mode[fmt].hw_dw_mode[1] = DM_YUV_ONLY;
				break;
			case VFORMAT_AVS2:
			case VFORMAT_AV1:
			case VFORMAT_AVS3:
			case VFORMAT_H266:
				dw_mode[fmt].hw_dw_mode[0] = DM_AVBC_ONLY;
				break;
			default:
				break;
		}
	}
}

void vdec_dos_dw_mode_init(void)
{
	int fmt;
	u32 dw_cap = get_dos_dw_capability();
	struct dos_hw_dw_mode *dw_mode = get_dos_dw_mode_all();
	struct dos_hw_output_mode *output_mode = get_dos_output_mode_all();

	dos_dw_mode_param_init();

	for (fmt = 0; fmt < VFORMAT_MAX; fmt++) {
		if (is_unsupport_dw_format(fmt) ||
			is_only_support_yuv_fmt(fmt))
			continue;

		if (fmt == VFORMAT_H264) {
			fill_dw_mode_from_hw_flag(dw_mode, fmt, dw_cap, YUV_1_2_4);
			fill_output_mode_from_hw_flag(output_mode, fmt, dw_cap, YUV_1_2_4);
		} else if (is_core_hevc_fmt(fmt)) {
			fill_dw_mode_from_hw_flag(dw_mode, fmt, dw_cap, 0);
			fill_output_mode_from_hw_flag(output_mode, fmt, dw_cap, 0);
		}
	}
}

