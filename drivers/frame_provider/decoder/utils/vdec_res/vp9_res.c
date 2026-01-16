// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define VP9_WK_BUF_SIZE		1769472
#define VP9_MV_BUF_SIZE_480P	65536
#define VP9_MV_BUF_SIZE_720P	196608
#define VP9_MV_BUF_SIZE_1080P	327680
#define VP9_MV_BUF_SIZE_4K	1245184
#define VP9_MV_BUF_SIZE_8K	4194304
#define VP9_PROB_BUF_SIZE	20480
#define VP9_COUNT_BUF_SIZE	12288
#define VP9_RDMA_BUF_SIZE	16384
#define VP9_RPM_BUF_SIZE	4096
#define VP9_LMEM_BUF_SIZE	4096
#define VP9_MMU_BUF_SIZE	73728

#define VP9_MV_NUM			2
#define VP9_DPB_NUM_480P	16
#define VP9_DPB_NUM_720P	16
#define VP9_DPB_NUM_1080P	16
#define VP9_DPB_NUM_4K		14
#define VP9_DPB_NUM_8K		13

static const struct vdec_res_buf vp9_wk_buf[] = {
	{PAGE_COUNT(VP9_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf vp9_mv_buf[] = {
	{PAGE_COUNT(VP9_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(VP9_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(VP9_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(VP9_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(VP9_MV_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf vp9_prob_buf[] = {
	{PAGE_COUNT(VP9_PROB_BUF_SIZE), 1}
};

static const struct vdec_res_buf vp9_count_buf[] = {
	{PAGE_COUNT(VP9_COUNT_BUF_SIZE), 1}
};

static const struct vdec_res_buf vp9_rdma_buf[] = {
	{PAGE_COUNT(VP9_RDMA_BUF_SIZE), 1},
};

static const struct vdec_res_buf vp9_rpm_buf[] = {
	{PAGE_COUNT(VP9_RPM_BUF_SIZE), 1}
};

static const struct vdec_res_buf vp9_lmem_buf[] = {
	{PAGE_COUNT(VP9_LMEM_BUF_SIZE), 1},
};

static const struct vdec_res_buf vp9_mmu_buf[] = {
	{PAGE_COUNT(VP9_MMU_BUF_SIZE), 1},
};

static const int vp9_dpb_num[] = {
	VP9_DPB_NUM_480P,
	VP9_DPB_NUM_720P,
	VP9_DPB_NUM_1080P,
	VP9_DPB_NUM_4K,
	VP9_DPB_NUM_8K,
};

static inline int get_vp9_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return vp9_wk_buf[0].buf_size;
}

static inline int get_vp9_mv_buf(struct resman_cb_est_param_t *est_param)
{
	return vp9_mv_buf[est_param->res_type].buf_size
		* VP9_MV_NUM;
}
static inline int get_vp9_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;

	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		yuv_num = est_param->margin_num + est_param->dpb_num;
	else
		yuv_num = vp9_dpb_num[est_param->res_type];
	return get_common_yuv_buf_size(est_param) * yuv_num;
}
static inline int get_vp9_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;

	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		avbc_nums = est_param->margin_num + est_param->dpb_num;
	else
		avbc_nums = vp9_dpb_num[est_param->res_type];
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_vp9_prealloc_buf(struct resman_cb_est_param_t *est_param)
{
	int cpu_type = get_cpu_type();
	int total_pages = 0;

	if (!est_param->is_tvp || !est_param->is_android || !est_param->pipeline)
		return 0;

	if (cpu_type != MESON_CPU_MAJOR_ID_T6D)
		return 0;

	total_pages = get_common_yuv_buf_size(est_param) * 10;
	total_pages += PAGE_COUNT(PREALLOC_BUF_SIZE);

	return total_pages;
}

static inline int get_vp9_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_vp9_wk_buf(est_param);

	/* Module3: prealloc buffer */
	prealloc_buf = get_vp9_prealloc_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("vp9 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_vp9_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_vp9_min_buf_mm(est_param);
	return 0;
}

static inline int get_vp9_prob_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_prob_buf[0].buf_size;
}

static inline int get_vp9_count_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_count_buf[0].buf_size;
}

static inline int get_vp9_rdma_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_rdma_buf[0].buf_size;
}

static inline int get_vp9_rpm_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_rpm_buf[0].buf_size;
}

static inline int get_vp9_lmem_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_lmem_buf[0].buf_size;
}

static inline int get_vp9_mmu_bufsize(struct resman_cb_est_param_t *est_param)
{
	return vp9_mmu_buf[0].buf_size;
}

static inline int get_vp9_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int mv_buf = 0;
	int prob_buf = 0;
	int count_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int rdma_buf = 0;
	int rpm_buf = 0;
	int lmem_buf = 0;
	int mmu_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_vp9_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_vp9_mv_buf(est_param);

	if (est_param->res_type != VDEC_RES_8K) {
		/* Module4: prob buffer */
		prob_buf = get_vp9_prob_bufsize(est_param);

		/* Module5: count buffer */
		count_buf = get_vp9_count_bufsize(est_param);
	}

	/* Module6: yuv size */
	yuv_buf = get_vp9_yuv_buf(est_param);

	/* Module7: avbc header size */
	avbc_header_buf = get_vp9_avbc_header_buf(est_param);

	/* Module8: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);

	if (est_param->res_type == VDEC_RES_8K) {
		/* Module9: rdma buffer */
		rdma_buf = get_vp9_rdma_bufsize(est_param);
	}

	/* Module10: rpm buffer */
	rpm_buf = get_vp9_rpm_bufsize(est_param);

	/* Module11: lmem buffer */
	lmem_buf = get_vp9_lmem_bufsize(est_param);

	/* Module12: mmu buffer */
	mmu_buf = get_vp9_mmu_bufsize(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("vp9 expect buf: wk %d, es %d, mv %d, prob %d, count %d, yuv %d, avbc_header %d, avbc_body %d, rdma %d, rpm %d, lmem %d, mmu %d\n",
			wk_buf, es_buf, mv_buf, prob_buf, count_buf, yuv_buf,
			avbc_header_buf, avbc_body_buf, rdma_buf, rpm_buf, lmem_buf,
			mmu_buf);
	}
	return (wk_buf + es_buf + mv_buf + prob_buf + count_buf + yuv_buf
			+ avbc_header_buf + avbc_body_buf + rdma_buf + rpm_buf
			+ lmem_buf + mmu_buf);
}

static inline int get_vp9_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_vp9_expected_buf_mm(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_vp9_mem_est_if = {
	.get_min_size			= get_vp9_min_bufsize,
	.get_expected_size			= get_vp9_expected_bufsize,
};

struct vdec_common_mem_est_if *get_vp9_mem_est_if(void)
{
	return &vdec_vp9_mem_est_if;
}

