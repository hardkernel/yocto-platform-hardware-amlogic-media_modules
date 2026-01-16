// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <uapi/amlogic/amvdec_ioc.h>
#include <linux/amlogic/media/codec_mm/codec_mm_scatter.h>

#include "vdec_res_priv.h"

/* ratio: 0.6 */
#define AVBC_BODY_MUL_RATIO(s) ((s) * 6 / 10)

#define BIT_DEPTH_10	10
#define BIT_DEPTH_8	8

/* Look up table, convert to page count, round up */
static const int avbc_body_8bit[] = {
	AVBC_BODY_8BIT_BUF_SIZE_480P,
	AVBC_BODY_8BIT_BUF_SIZE_720P,
	AVBC_BODY_8BIT_BUF_SIZE_1080P,
	AVBC_BODY_8BIT_BUF_SIZE_4K,
	AVBC_BODY_8BIT_BUF_SIZE_8K
};

static const int avbc_body_10bit[] = {
	AVBC_BODY_10BIT_BUF_SIZE_480P,
	AVBC_BODY_10BIT_BUF_SIZE_720P,
	AVBC_BODY_10BIT_BUF_SIZE_1080P,
	AVBC_BODY_10BIT_BUF_SIZE_4K,
	AVBC_BODY_10BIT_BUF_SIZE_8K
};

static const struct vdec_res_buf vdec_es_bufs_group[] = {
	{PAGE_COUNT(ES_BUF_SIZE_2K), 1},
	{PAGE_COUNT(ES_BUF_SIZE_4K), 1},
	{PAGE_COUNT(ES_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf common_avbc_header_bufs[] = {
	{PAGE_COUNT(AVBC_HEADER_BUF_SIZE_2K), 1},
	{PAGE_COUNT(AVBC_HEADER_BUF_SIZE_4K), 1},
	{PAGE_COUNT(AVBC_HEADER_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf common_st_bufs[] = {
	{PAGE_COUNT(ST_BUF_SIZE), 1},
};

static const struct vdec_res_buf common_fetch_bufs[] = {
	{PAGE_COUNT(FETCH_BUF_SIZE), 1},
};

static const struct vdec_res_buf common_vframe_input_bufs[] = {
	{PAGE_COUNT(VFRAME_INPUT_BUF_SIZE), 1},
};

bool is_2k_vdec_platform(void)
{
	int cpu_type = get_cpu_type();
	int pack_type = get_meson_cpu_version(MESON_CPU_VERSION_LVL_PACK);

	if (cpu_type == MESON_CPU_MAJOR_ID_T5D ||
		cpu_type == MESON_CPU_MAJOR_ID_T6D ||
		cpu_type == MESON_CPU_MAJOR_ID_TXHD2 ||
		cpu_type == MESON_CPU_MAJOR_ID_S1A ||
		(cpu_type == MESON_CPU_MAJOR_ID_S4 && pack_type == 2) ||
		(cpu_type == MESON_CPU_MAJOR_ID_S7 && pack_type == 3))
		return true;

	return false;
}

bool is_v4l2_pipeline(struct resman_cb_est_param_t *est_param)
{
	if (est_param->pipeline == RESMAN_PLAYBACK_PIPELINE_V4L2_FRAMEMODE ||
		est_param->pipeline == RESMAN_PLAYBACK_PIPELINE_V4L2_STREAMMODE)
		return true;
	return false;
}

bool is_amport_pipeline(struct resman_cb_est_param_t *est_param)
{
	if (est_param->pipeline == RESMAN_PLAYBACK_PIPELINE_AMPORT_STREAMMODE)
		return true;
	return false;
}

bool is_stream_mode_pipeline(struct resman_cb_est_param_t *est_param)
{
	if (est_param->pipeline == RESMAN_PLAYBACK_PIPELINE_V4L2_STREAMMODE ||
		est_param->pipeline == RESMAN_PLAYBACK_PIPELINE_AMPORT_STREAMMODE)
		return true;
	return false;
}

int get_common_es_buf_size(struct resman_cb_est_param_t *est_param)
{
	enum vdec_res_group_type res_group_type =
		get_res_group_type(est_param->res_type);

	if (est_param->is_tvp || is_stream_mode_pipeline(est_param))
		return 0;

	return vdec_es_bufs_group[res_group_type].buf_size;
}

inline int get_common_st_buf_size(struct resman_cb_est_param_t *est_param)
{
	return common_st_bufs[0].buf_size;
}

inline int get_common_avbc_header_size(struct resman_cb_est_param_t *est_param)
{
	enum vdec_res_group_type res_group_type =
		get_res_group_type(est_param->res_type);

	if (est_param->dw == DM_YUV_ONLY)
		return 0;

	return common_avbc_header_bufs[res_group_type].buf_size;
}

inline int get_avbc_body_size(struct resman_cb_est_param_t *est_param)
{
	int avbc_body_size = 0;
	int total_num = 0;

	if (est_param->dw == DM_YUV_ONLY)
		return 0;
	/*
	 * Secure Playback or SCATTER_ALLOC_FROM_CMA
	 * means scatter is allocated from codec_mm
	 */
	if (codec_mm_scatter_source_check(est_param->is_tvp) == SCATTER_ALLOC_FROM_CMA) {
		if (est_param->margin_num > 0 && est_param->dpb_num > 0)
			total_num = est_param->margin_num + est_param->dpb_num;

		if (total_num == 0)
			return 0;

		if (est_param->bitdepth == BIT_DEPTH_10) {
			avbc_body_size = (total_num - 1) *
				AVBC_BODY_MUL_RATIO(avbc_body_10bit[est_param->res_type]);
			avbc_body_size += avbc_body_10bit[est_param->res_type];
		} else {
			avbc_body_size = (total_num - 1) *
				AVBC_BODY_MUL_RATIO(avbc_body_8bit[est_param->res_type]);
			avbc_body_size += avbc_body_8bit[est_param->res_type];
		}
	}

	return avbc_body_size;
}

inline int get_common_fetch_buf_size(struct resman_cb_est_param_t *est_param)
{
	return common_fetch_bufs[0].buf_size;
}

inline int get_common_vframe_input_buf_size(struct resman_cb_est_param_t *est_param)
{
	return common_vframe_input_bufs[0].buf_size;
}

static void get_basic_size(int res_type, int *p_width, int *p_height)
{
	if (res_type == VDEC_RES_480P) {
		*p_width = 720;
		*p_height = 480;
	} else if (res_type == VDEC_RES_720P) {
		*p_width = 1280;
		*p_height = 720;
	} else if (res_type == VDEC_RES_1080P) {
		*p_width = 1920;
		*p_height = 1080;
	} else if (res_type == VDEC_RES_4K) {
		*p_width = 3840;
		*p_height = 2160;
	} else if (res_type == VDEC_RES_8K) {
		*p_width = 7680;
		*p_height = 4320;
	}
}

int get_common_yuv_buf_size(struct resman_cb_est_param_t *est_param)
{
	int width = 0;
	int height = 0;
	int dw = est_param->dw;
	int y_size, uv_size;
	int platform_width = 4096;
	int platform_height = 2304;

	get_basic_size(est_param->res_type, &width, &height);
	/*
	 * The following two are both 2k videos
	 * Input 1920*540 resolution, decoder requests yuv buffer as 1920*1080
	 * Input 1920*1088 resolution, decoder requests yuv buffer as 1920*1088
	 */
	if ((width * height) < (est_param->width * est_param->height)) {
		width = est_param->width;
		height = est_param->height;
	}

	if (is_stream_mode_pipeline(est_param)) {
		if (est_param->video_format == VFORMAT_MPEG12 ||
			est_param->video_format == VFORMAT_MPEG4 ||
			est_param->video_format == VFORMAT_AVS ||
			est_param->video_format == VFORMAT_VC1) {
			return PAGE_COUNT(YUV_BUF_2K);
		} else if (est_param->video_format == VFORMAT_MJPEG) {
			return PAGE_COUNT(YUV_BUF_4K);
		}
	}

	if (is_2k_vdec_platform()) {
		platform_width = 1920;
		platform_height = 1080;
	}

	if (dw == DM_AVBC_ONLY) {
		width = 64;
		height = 64;
	} else if (dw == DM_YUV_AUTO_1_4_AVBC) {
		width = 1920;
		height = 1080;
	} else if (dw == DM_YUV_AUTO_14_12_AVBC) {
		width = 960;
		height = 540;
	} else if (dw == DM_YUV_1_4_AVBC_A || dw == DM_YUV_1_4_AVBC_B) {
		width = ALIGN(platform_width >> 2, 64);
		height = ALIGN(platform_height >> 2, 64);
	} else if (dw == DM_YUV_1_2_AVBC) {
		width = ALIGN(platform_width >> 1, 64);
		height = ALIGN(platform_height >> 1, 64);
	}

	if (est_param->is_interlace && est_param->video_format == VFORMAT_HEVC)
		height = ALIGN(height / 2, 64);

	y_size = ALIGN(width, 64) * ALIGN(height, 64);
	uv_size = y_size >> 1;

	pr_info("width %d, height %d, dw %d, y_size %d, uv_size %d\n",
		width, height, dw, y_size, uv_size);

	return PAGE_COUNT(y_size + uv_size);
}

struct vdec_common_mem_est_if *vdec_mem_est_if(int video_type)
{
	switch (video_type) {
	case VFORMAT_H264:
		return get_h264_mem_est_if();
	case VFORMAT_HEVC:
		return get_h265_mem_est_if();
	case VFORMAT_H266:
		return get_h266_mem_est_if();
	case VFORMAT_VP9:
		return get_vp9_mem_est_if();
	case VFORMAT_MPEG12:
		return get_mpeg12_mem_est_if();
	case VFORMAT_MPEG4:
		return get_mpeg4_mem_est_if();
	case VFORMAT_MJPEG:
		return get_mjpeg_mem_est_if();
	case VFORMAT_AV1:
		return get_av1_mem_est_if();
	case VFORMAT_AVS:
		return get_avs_mem_est_if();
	case VFORMAT_AVS2:
		return get_avs2_mem_est_if();
	case VFORMAT_AVS3:
		return get_avs3_mem_est_if();
	case VFORMAT_VC1:
		return get_vc1_mem_est_if();

	default:
		return NULL;
	}
}

