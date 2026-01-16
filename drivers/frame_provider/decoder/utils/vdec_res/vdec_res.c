// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <uapi/amlogic/amvdec_ioc.h>
#include <linux/amlogic/media/codec_mm/codec_mm_mem_info.h>
#include "../../../../common/media_utils/media_utils.h"

#include "vdec_res_priv.h"

static inline bool is_within_size(int w, int h, int mul)
{
	if (h && ((mul / h) >= w))
		return true;
	return false;
}

static int get_res_type_by_size(int width, int height)
{
	if (is_within_size(width, height, 768 * 576))
		return VDEC_RES_480P;
	if (is_within_size(width, height, 1280 * 768))
		return VDEC_RES_720P;
	if (is_within_size(width, height, 1920 * 1088))
		return VDEC_RES_1080P;
	if (is_within_size(width, height, 3840 * 2176))
		return VDEC_RES_4K;
	if (is_within_size(width, height, 7680 * 4352))
		return VDEC_RES_8K;
	return VDEC_RES_INVALID;
}

static void pre_process_est_param(struct resman_cb_est_param_t *est_param)
{
	if (est_param->video_format == VFORMAT_VC1 ||
		est_param->video_format == VFORMAT_MPEG12 ||
		est_param->video_format == VFORMAT_MPEG4 ||
		est_param->video_format == VFORMAT_MJPEG ||
		est_param->video_format == VFORMAT_AVS ||
		est_param->is_interlace)
		est_param->dw = DM_YUV_ONLY;
}

int query_min_memory_func(struct resman_cb_est_param_t *est_param)
{
	struct vdec_common_mem_est_if *mem_est_if =
		vdec_mem_est_if(est_param->video_format);

	pre_process_est_param(est_param);
	est_param->res_type =
		get_res_type_by_size(est_param->width, est_param->height);
	if (est_param->res_type == VDEC_RES_INVALID) {
		pr_err("vdec mem est err: invalid res type, width %d, height %d\n",
			est_param->width, est_param->height);
		return -1;
	}
	if (mem_est_if && mem_est_if->get_min_size)
		return mem_est_if->get_min_size(est_param);

	return -1;
}

int query_expected_memory_func(struct resman_cb_est_param_t *est_param)
{
	struct vdec_common_mem_est_if *mem_est_if =
		vdec_mem_est_if(est_param->video_format);

	pre_process_est_param(est_param);
	est_param->res_type =
		get_res_type_by_size(est_param->width, est_param->height);
	if (est_param->res_type == VDEC_RES_INVALID) {
		pr_err("vdec mem est err: invalid res type, width %d, height %d\n",
			est_param->width, est_param->height);
		return -1;
	}
	if (mem_est_if && mem_est_if->get_expected_size)
		return mem_est_if->get_expected_size(est_param);

	return -1;
}

int query_current_memory_func(struct resman_cb_dec_status_t *dst_param)
{
	return query_decoder_mem(dst_param->ssid, dst_param->flag);
}

