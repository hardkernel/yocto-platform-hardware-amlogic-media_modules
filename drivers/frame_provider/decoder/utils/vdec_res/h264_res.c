// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define H264_WK_BUF_SIZE	2273280
#define H264_MV_BUF_SIZE_480P	147456
#define H264_MV_BUF_SIZE_720P	368640
#define H264_MV_BUF_SIZE_1080P	783360
#define H264_MV_BUF_SIZE_4K	3133440
#define H264_EXTIF_BUF_SIZE_4K	262144
#define H264_AUX_BUF_SIZE	16384
#define H264_LMEM_BUF_SIZE	4096
#define H264_MMU_BUF_SIZE_4K	20480

#define H264_DPB_NUM_480P	11
#define H264_DPB_NUM_720P	11
#define H264_DPB_NUM_1080P	11
#define H264_DPB_NUM_4K		12
#define H264_DPB_NUM_8K		0

#define REFERENCE_BUF_MARGIN	4

static const struct vdec_res_buf h264_wk_buf[] = {
	{PAGE_COUNT(H264_WK_BUF_SIZE), 1},
};

/* decoder mb_total*96 */
static const struct vdec_res_buf h264_mv_buf[] = {
	{PAGE_COUNT(H264_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(H264_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(H264_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(H264_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(0), 1},
};

static const struct vdec_res_buf h264_extif_buf[] = {
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(H264_EXTIF_BUF_SIZE_4K), 1},
	{PAGE_COUNT(0), 1},
};

static const struct vdec_res_buf h264_aux_buf[] = {
	{PAGE_COUNT(H264_AUX_BUF_SIZE), 1},
};
static const struct vdec_res_buf h264_lmem_buf[] = {
	{PAGE_COUNT(H264_LMEM_BUF_SIZE), 1},
};
static const struct vdec_res_buf h264_mmu_buf[] = {
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(0), 1},
	{PAGE_COUNT(H264_MMU_BUF_SIZE_4K), 1},
	{PAGE_COUNT(0), 1},
};

static const int h264_dpb_num[] = {
	H264_DPB_NUM_480P,
	H264_DPB_NUM_720P,
	H264_DPB_NUM_1080P,
	H264_DPB_NUM_4K,
	0,
};

static inline int get_h264_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return h264_wk_buf[0].buf_size;
}

static inline int get_h264_mv_buf(struct resman_cb_est_param_t *est_param)
{
	/*
	* H264 mv_num is different from different video,
	* but the maximum does not exceed dpb_num
	*/
	if (est_param->dpb_num > 0)
		return h264_mv_buf[est_param->res_type].buf_size *
			(est_param->dpb_num - 1 + REFERENCE_BUF_MARGIN);
	else
		return h264_mv_buf[est_param->res_type].buf_size *
			(2 * REFERENCE_BUF_MARGIN);
}

static inline int get_h264_extif_buf(struct resman_cb_est_param_t *est_param)
{
	return h264_extif_buf[est_param->res_type].buf_size;
}

static inline int get_h264_aux_buf(struct resman_cb_est_param_t *est_param)
{
	return h264_aux_buf[0].buf_size;
}

static inline int get_h264_lmem_buf(struct resman_cb_est_param_t *est_param)
{
	return h264_lmem_buf[0].buf_size;
}

static inline int get_h264_mmu_buf(struct resman_cb_est_param_t *est_param)
{
	return h264_mmu_buf[est_param->res_type].buf_size;
}

static inline int get_h264_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			yuv_num /= 3;
	} else {
		yuv_num = h264_dpb_num[est_param->res_type];
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}
static inline int get_h264_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;
	if (est_param->margin_num > 0 &&
		est_param->dpb_num > 0) {
		avbc_nums = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			avbc_nums /= 3;
	} else {
		avbc_nums = h264_dpb_num[est_param->res_type];
	}
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_h264_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_h264_wk_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("h264 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_h264_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int st_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_h264_wk_buf(est_param);

	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("h264 min buf dtv: wk %d, st %d\n",
			wk_buf, st_buf);
	return (wk_buf + st_buf);
}

static inline int get_h264_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_h264_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_h264_min_buf_dtv(est_param);
	return 0;
}

static inline int get_h264_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int mv_buf = 0;
	int extif_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int aux_buf = 0;
	int lmem_buf = 0;
	int mmu_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_h264_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_h264_mv_buf(est_param);

	/* Module4: yuv buffer and avbc header buffer */
	yuv_buf = get_h264_yuv_buf(est_param);

	/* Module5: avbc header buffer */
	avbc_header_buf = get_h264_avbc_header_buf(est_param);

	/* Module6: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);

	/* Module7: aux buffer */
	aux_buf = get_h264_aux_buf(est_param);

	/* Module8: lmem buffer */
	lmem_buf = get_h264_lmem_buf(est_param);

	if (est_param->res_type == VDEC_RES_4K) {
		/* Module9: mmu buffer */
		mmu_buf = get_h264_mmu_buf(est_param);
		/* Module10: extif buffer */
		extif_buf = get_h264_extif_buf(est_param);
	}

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("h264 expect buf: wk %d, es %d, mv %d, extif %d, yuv %d\n",
			wk_buf, es_buf, mv_buf, extif_buf, yuv_buf);
		pr_info("avbc_header %d, avbc_body %d, aux %d, lmem %d, mmu %d\n",
			avbc_header_buf, avbc_body_buf, aux_buf, lmem_buf, mmu_buf);
	}
	return (wk_buf + es_buf + mv_buf + extif_buf + yuv_buf +
		avbc_header_buf + avbc_body_buf + aux_buf + lmem_buf + mmu_buf);
}

static inline int get_h264_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int mv_buf = 0;
	int vframe_input_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int extif_buf = 0;
	int aux_buf = 0;
	int mmu_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_h264_wk_buf(est_param);
	/* Module2: mv buffer */
	mv_buf = get_h264_mv_buf(est_param);
	/* Module3: vframe input buffer, it's common size */
	vframe_input_buf = get_common_vframe_input_buf_size(est_param);
	/* Module4: yuv buffer and avbc header buffer */
	yuv_buf = get_h264_yuv_buf(est_param);
	/* Module5: avbc header buffer */
	avbc_header_buf = get_h264_avbc_header_buf(est_param);
	/* Module6: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);
	/* Module7: extif buffer */
	if (est_param->res_type == VDEC_RES_4K)
		extif_buf = get_h264_extif_buf(est_param);
	/* Module8: aux buffer */
	aux_buf = get_h264_aux_buf(est_param);
	/* Module9: mmu buffer */
	mmu_buf = get_h264_mmu_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("h264 expect buf: wk %d, mv %d, vframe_input %d, yuv %d\n",
			wk_buf, mv_buf, vframe_input_buf, yuv_buf);
		pr_info("avbc_header %d, avbc_body %d, extif %d, aux %d, mmu %d\n",
			avbc_header_buf, avbc_body_buf, extif_buf, aux_buf, mmu_buf);
	}

	return (wk_buf + mv_buf + vframe_input_buf +
		yuv_buf + avbc_header_buf + avbc_body_buf +
		extif_buf + aux_buf + mmu_buf);
}

static inline int get_h264_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_h264_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_h264_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_h264_mem_est_if = {
	.get_min_size		= get_h264_min_bufsize,
	.get_expected_size	= get_h264_expected_bufsize,
};

struct vdec_common_mem_est_if *get_h264_mem_est_if(void)
{
	return &vdec_h264_mem_est_if;
}

