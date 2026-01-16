// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define H266_WK_BUF_SIZE	4194304
#define H266_MV_BUF_SIZE_480P	262144
#define H266_MV_BUF_SIZE_720P	262144
#define H266_MV_BUF_SIZE_1080P	262144
#define H266_MV_BUF_SIZE_4K	1179648
#define H266_MV_BUF_SIZE_8K	4718592
#define H266_PRM_BUF_SIZE	4096
#define H266_AUX_BUF_SIZE	24576
#define H266_LMEM_BUF_SIZE	4096
#define H266_REF_LIST_BUF_SIZE	266240
#define H266_MMU_BUF_SIZE	77824
#define H266_DTV_WK_BUF_SIZE	4259840

#define H266_MV_NUM_480P	16
#define H266_MV_NUM_720P	16
#define H266_MV_NUM_1080P	16
#define H266_MV_NUM_4K		8
#define H266_MV_NUM_8K		6
#define H266_DPB_NUM_480P	23
#define H266_DPB_NUM_720P	23
#define H266_DPB_NUM_1080P	23
#define H266_DPB_NUM_4K		15
#define H266_DPB_NUM_8K		12

static const struct vdec_res_buf h266_wk_buf[] = {
	{PAGE_COUNT(H266_WK_BUF_SIZE), 1},
};
static const struct vdec_res_buf dtv_h266_wk_buf[] = {
	{PAGE_COUNT(H266_DTV_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf h266_mv_buf[] = {
	{PAGE_COUNT(H266_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(H266_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(H266_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(H266_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(H266_MV_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf h266_prm_buf[] = {
	{PAGE_COUNT(H266_PRM_BUF_SIZE), 1},
};
static const struct vdec_res_buf h266_aux_buf[] = {
	{PAGE_COUNT(H266_AUX_BUF_SIZE), 1},
};
static const struct vdec_res_buf h266_lmem_buf[] = {
	{PAGE_COUNT(H266_LMEM_BUF_SIZE), 1},
};
static const struct vdec_res_buf h266_ref_list_buf[] = {
	{PAGE_COUNT(H266_REF_LIST_BUF_SIZE), 1},
};
static const struct vdec_res_buf h266_mmu_buf[] = {
	{PAGE_COUNT(H266_MMU_BUF_SIZE), 1},
};

static const int h266_mv_num[] = {
	H266_MV_NUM_480P,
	H266_MV_NUM_720P,
	H266_MV_NUM_1080P,
	H266_MV_NUM_4K,
	H266_MV_NUM_8K,
};
static const int h266_dpb_num[] = {
	H266_DPB_NUM_480P,
	H266_DPB_NUM_720P,
	H266_DPB_NUM_1080P,
	H266_DPB_NUM_4K,
	H266_DPB_NUM_8K,
};

static inline int get_h266_ref_list_buf(struct resman_cb_est_param_t *est_param)
{
	return h266_ref_list_buf[0].buf_size;
}

static inline int get_h266_prm_buf(struct resman_cb_est_param_t *est_param)
{
	return h266_prm_buf[0].buf_size;
}

static inline int get_h266_aux_buf(struct resman_cb_est_param_t *est_param)
{
	return h266_aux_buf[0].buf_size;
}

static inline int get_h266_lmem_buf(struct resman_cb_est_param_t *est_param)
{
	return h266_lmem_buf[0].buf_size;
}

static inline int get_h266_mmu_buf(struct resman_cb_est_param_t *est_param)
{
	return h266_mmu_buf[0].buf_size;
}

static inline int get_h266_wk_buf(struct resman_cb_est_param_t *est_param)
{
	if (is_amport_pipeline(est_param))
		return dtv_h266_wk_buf[0].buf_size;
	else if (is_v4l2_pipeline(est_param))
		return h266_wk_buf[0].buf_size;
	return 0;
}

static inline int get_h266_mv_buf(struct resman_cb_est_param_t *est_param)
{
	int mv_num = 0;

	if (est_param->dpb_num > 0) {
		mv_num = est_param->dpb_num - 1;
	} else {
		mv_num = h266_mv_num[est_param->res_type];
	}
	return h266_mv_buf[est_param->res_type].buf_size * mv_num;
}

static inline int get_h266_prealloc_buf(struct resman_cb_est_param_t *est_param)
{
	int cpu_type = get_cpu_type();
	int total_pages = 0;

	if (!est_param->is_tvp || !est_param->is_android || !est_param->pipeline)
		return 0;

	if (cpu_type != MESON_CPU_MAJOR_ID_T6D)
		return 0;

	total_pages = get_common_yuv_buf_size(est_param) * 10;
	total_pages += PAGE_COUNT(24 * SZ_1M);

	return total_pages;
}

static inline int get_h266_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
	} else {
		yuv_num = h266_dpb_num[est_param->res_type];
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_h266_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		avbc_nums = est_param->margin_num + est_param->dpb_num;
	else
		avbc_nums = h266_dpb_num[est_param->res_type];
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_h266_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_h266_wk_buf(est_param);

	/* Module3: prealloc buffer */
	prealloc_buf = get_h266_prealloc_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("h266 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_h266_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int st_buf = 0;
	int avbc_body_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_h266_wk_buf(est_param);

	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);

	/* Module3: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("h266 min buf dtv: wk %d, avbc_body %d, st %d\n",
			wk_buf, avbc_body_buf, st_buf);
	return (wk_buf + avbc_body_buf + st_buf);
}

static inline int get_h266_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_h266_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_h266_min_buf_dtv(est_param);
	return 0;
}

static inline int get_h266_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int mv_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int ref_list_buf = 0;
	int prm_buf = 0;
	int aux_buf = 0;
	int lmem_buf = 0;
	int mmu_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_h266_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_h266_mv_buf(est_param);

	/* Module4: yuv size */
	yuv_buf = get_h266_yuv_buf(est_param);

	/* Module5: avbc header size */
	avbc_header_buf = get_h266_avbc_header_buf(est_param);

	/* Module6: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);

	/* Module7: ref list buffer */
	ref_list_buf = get_h266_ref_list_buf(est_param);

	/* Module8: prm buffer */
	prm_buf = get_h266_prm_buf(est_param);

	/* Module9: aux buffer */
	aux_buf = get_h266_aux_buf(est_param);

	/* Module10: lmem buffer */
	lmem_buf = get_h266_lmem_buf(est_param);

	/* Module11: mmu buffer */
	mmu_buf = get_h266_mmu_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("h266 expect buf: wk %d, es %d, mv %d, yuv %d, avbc_header %d\n",
			wk_buf, es_buf, mv_buf, yuv_buf, avbc_header_buf);
		pr_info("avbc_body %d, ref_list %d, prm  %d, aux %d, lmem %d, mmu %d\n",
			avbc_body_buf, ref_list_buf, prm_buf, aux_buf, lmem_buf, mmu_buf);
	}
	return (wk_buf + es_buf + mv_buf + yuv_buf + avbc_header_buf +
		avbc_body_buf + ref_list_buf + prm_buf + aux_buf + lmem_buf + mmu_buf);
}

static inline int get_h266_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int mv_buf = 0;
	int vframe_input_buf = 0;
	int aux_buf = 0;
	int mmu_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_h266_wk_buf(est_param);
	/* Module2: mv buffer */
	mv_buf = get_h266_mv_buf(est_param);
	/* Module3: vframe input buffer */
	vframe_input_buf = get_common_vframe_input_buf_size(est_param);
	/* Module4: aux buffer */
	aux_buf = get_h266_aux_buf(est_param);
	/* Module5: mmu buffer */
	mmu_buf = get_h266_mmu_buf(est_param);
	/* Module6: yuv size */
	yuv_buf = get_h266_yuv_buf(est_param);
	/* Module7: avbc header size */
	avbc_header_buf = get_h266_avbc_header_buf(est_param);
	/* Module8: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("h266 expect buf dtv: wk %d, mv %d, vframe_input %d, aux %d\n",
			wk_buf, mv_buf, vframe_input_buf, aux_buf);
		pr_info("mmu %d, yuv %d, avbc_header %d, avbc_body %d\n",
			mmu_buf, yuv_buf, avbc_header_buf, avbc_body_buf);
	}
	return (wk_buf + mv_buf + vframe_input_buf +
		aux_buf + mmu_buf + yuv_buf + avbc_header_buf + avbc_body_buf);
}

static inline int get_h266_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_h266_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_h266_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_h266_mem_est_if = {
	.get_min_size		= get_h266_min_bufsize,
	.get_expected_size	= get_h266_expected_bufsize,
};

struct vdec_common_mem_est_if *get_h266_mem_est_if(void)
{
	return &vdec_h266_mem_est_if;
}


