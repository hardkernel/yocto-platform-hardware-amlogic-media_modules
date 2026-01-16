// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define AVS2_WK_BUF_SIZE	2883584
#define AVS2_MV_BUF_SIZE_480P	262144
#define AVS2_MV_BUF_SIZE_720P	262144
#define AVS2_MV_BUF_SIZE_1080P	262144
#define AVS2_MV_BUF_SIZE_4K	1179648
#define AVS2_MV_BUF_SIZE_8K	4718592
#define AVS2_CUVA_BUF_SIZE	4096
#define AVS2_RDMA_BUF_SIZE	16384
#define AVS2_RPM_BUF_SIZE	4096
#define AVS2_LMEM_BUF_SIZE	4096
#define AVS2_MMU_BUF_SIZE	73728

#define AVS2_MV_NUM_480P	15
#define AVS2_MV_NUM_720P	15
#define AVS2_MV_NUM_1080P	7
#define AVS2_MV_NUM_4K		8
#define AVS2_MV_NUM_8K		9
#define AVS2_DPB_NUM_480P	22
#define AVS2_DPB_NUM_720P	22
#define AVS2_DPB_NUM_1080P	14
#define AVS2_DPB_NUM_4K		18
#define AVS2_DPB_NUM_8K		14

static const struct vdec_res_buf avs2_wk_buf[] = {
	{PAGE_COUNT(AVS2_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs2_mv_buf[] = {
	{PAGE_COUNT(AVS2_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(AVS2_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(AVS2_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(AVS2_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(AVS2_MV_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf avs2_cuva_buf[] = {
	{PAGE_COUNT(AVS2_CUVA_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs2_rdma_buf[] = {
	{PAGE_COUNT(AVS2_RDMA_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs2_rpm_buf[] = {
	{PAGE_COUNT(AVS2_RPM_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs2_lmem_buf[] = {
	{PAGE_COUNT(AVS2_LMEM_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs2_mmu_buf[] = {
	{PAGE_COUNT(AVS2_MMU_BUF_SIZE), 1},
};

static const int avs2_mv_num[] = {
	AVS2_MV_NUM_480P,
	AVS2_MV_NUM_720P,
	AVS2_MV_NUM_1080P,
	AVS2_MV_NUM_4K,
	AVS2_MV_NUM_8K,
};
static const int avs2_dpb_num[] = {
	AVS2_DPB_NUM_480P,
	AVS2_DPB_NUM_720P,
	AVS2_DPB_NUM_1080P,
	AVS2_DPB_NUM_4K,
	AVS2_DPB_NUM_8K,
};

static inline int get_avs2_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return avs2_wk_buf[0].buf_size;
}

static inline int get_avs2_mv_buf(struct resman_cb_est_param_t *est_param)
{
	int mv_num = 0;
	if (est_param->dpb_num > 0) {
		mv_num = est_param->dpb_num - 1;
	} else {
		mv_num = avs2_mv_num[est_param->res_type];
	}
	return avs2_mv_buf[est_param->res_type].buf_size * mv_num;
}

static inline int get_avs2_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;

	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
	} else {
		yuv_num = avs2_dpb_num[est_param->res_type];
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_avs2_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		avbc_nums = est_param->margin_num + est_param->dpb_num;
	else
		avbc_nums = avs2_dpb_num[est_param->res_type];
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_avs2_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_avs2_wk_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs2 min buf: wk %d, es %d\n",
			wk_buf, es_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_avs2_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int mv_buf = 0;
	int st_buf = 0;
	int avbc_body_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_avs2_wk_buf(est_param);

	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);

	/* Module3: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);

	/* Module4: mv size */
	mv_buf = get_avs2_mv_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs2 min buf dtv: wk %d, avbc_body %d, st %d, mv %d\n",
			wk_buf, avbc_body_buf, st_buf, mv_buf);
	return (wk_buf + avbc_body_buf + st_buf + mv_buf);
}

static inline int get_avs2_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs2_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs2_min_buf_dtv(est_param);
	return 0;
}

static inline int get_avs2_cuva_bufsize(struct resman_cb_est_param_t *est_param)
{
	return avs2_cuva_buf[0].buf_size;
}

static inline int get_avs2_rdma_bufsize(struct resman_cb_est_param_t *est_param)
{
	return avs2_rdma_buf[0].buf_size;
}

static inline int get_avs2_rpm_bufsize(struct resman_cb_est_param_t *est_param)
{
	return avs2_rpm_buf[0].buf_size;
}

static inline int get_avs2_lmem_bufsize(struct resman_cb_est_param_t *est_param)
{
	return avs2_lmem_buf[0].buf_size;
}

static inline int get_avs2_mmu_bufsize(struct resman_cb_est_param_t *est_param)
{
	return avs2_mmu_buf[0].buf_size;
}

static inline int get_avs2_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int mv_buf = 0;
	int cuva_buf = 0;
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
	wk_buf = get_avs2_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_avs2_mv_buf(est_param);

	/* Module4: cuva buffer */
	cuva_buf = get_avs2_cuva_bufsize(est_param);

	/* Module5: yuv buffer */
	yuv_buf = get_avs2_yuv_buf(est_param);

	/* Module6: avbc header buffer */
	avbc_header_buf = get_avs2_avbc_header_buf(est_param);

	/* Module7: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);

	if (est_param->res_type == VDEC_RES_8K)
		/* Module8: rdma buffer */
		rdma_buf = get_avs2_rdma_bufsize(est_param);

	/* Module9: rpm buffer */
	rpm_buf = get_avs2_rpm_bufsize(est_param);

	/* Module10: lmem buffer */
	lmem_buf = get_avs2_lmem_bufsize(est_param);

	/* Module11: mmu buffer */
	mmu_buf = get_avs2_mmu_bufsize(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("avs2 expect buf: wk %d, es %d, mv %d, cuva %d, yuv %d\n",
			wk_buf, es_buf, mv_buf, cuva_buf, yuv_buf);
		pr_info("avbc_header %d, avbc_body %d, rdma %d, rpm %d\n",
			avbc_header_buf, avbc_body_buf, rdma_buf, rpm_buf);
		pr_info("lmem %d, mmu %d\n",lmem_buf, mmu_buf);
	}


	return (wk_buf + es_buf + mv_buf + cuva_buf + yuv_buf + avbc_header_buf
			+ avbc_body_buf + rdma_buf + rpm_buf + lmem_buf + mmu_buf);
}

static inline int get_avs2_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int mv_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int cuva_buf = 0;
	int mmu_buf = 0;
	/* Module1: wk buffer */
	wk_buf = get_avs2_wk_buf(est_param);
	/* Module2: mv buffer */
	mv_buf = get_avs2_mv_buf(est_param);
	/* Module3: yuv buffer and avbc header buffer */
	yuv_buf = get_avs2_yuv_buf(est_param);
	/* Module4: avbc header buffer */
	avbc_header_buf = get_avs2_avbc_header_buf(est_param);
	/* Module5: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);
	/* Module6: cuva buffer */
	cuva_buf = get_avs2_cuva_bufsize(est_param);
	/* Module7: mmu buffer */
	mmu_buf = get_avs2_mmu_bufsize(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("avs2 expect buf: wk %d, mv %d, yuv %d, avbc_header %d\n",
			wk_buf, mv_buf, yuv_buf, avbc_header_buf);
		pr_info("avbc_body %d, cuva %d, mmu %d\n",
			avbc_body_buf, cuva_buf, mmu_buf);
	}
	return (wk_buf + mv_buf + yuv_buf + avbc_header_buf
			+ avbc_body_buf + cuva_buf + mmu_buf);
}

static inline int get_avs2_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs2_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs2_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_avs2_mem_est_if = {
	.get_min_size		= get_avs2_min_bufsize,
	.get_expected_size	= get_avs2_expected_bufsize,
};

struct vdec_common_mem_est_if *get_avs2_mem_est_if(void)
{
	return &vdec_avs2_mem_est_if;
}


