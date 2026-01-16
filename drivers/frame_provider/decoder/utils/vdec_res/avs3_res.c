// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define AVS3_WK_BUF_SIZE	3801088
#define AVS3_MV_BUF_SIZE_480P	262144
#define AVS3_MV_BUF_SIZE_720P	262144
#define AVS3_MV_BUF_SIZE_1080P	262144
#define AVS3_MV_BUF_SIZE_4K	1179648
#define AVS3_MV_BUF_SIZE_8K	4718592
#define AVS3_CUVA_BUF_SIZE	4096
#define AVS3_RDMA_BUF_SIZE	16384
#define AVS3_RPM_BUF_SIZE	4096
#define AVS3_LMEM_BUF_SIZE	4096
#define AVS3_MMU_BUF_SIZE	73728

#define AVS3_MV_NUM_480P	16
#define AVS3_MV_NUM_720P	16
#define AVS3_MV_NUM_1080P	8
#define AVS3_MV_NUM_4K		8
#define AVS3_MV_NUM_8K		7
#define AVS3_DPB_NUM_480P	22
#define AVS3_DPB_NUM_720P	22
#define AVS3_DPB_NUM_1080P	22
#define AVS3_DPB_NUM_4K		22
#define AVS3_DPB_NUM_8K		13

static const struct vdec_res_buf avs3_wk_buf[] = {
	{PAGE_COUNT(AVS3_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs3_mv_buf[] = {
	{PAGE_COUNT(AVS3_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(AVS3_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(AVS3_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(AVS3_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(AVS3_MV_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf avs3_cuva_buf[] = {
	{PAGE_COUNT(AVS3_CUVA_BUF_SIZE), 1},
};
static const struct vdec_res_buf avs3_rdma_buf[] = {
	{PAGE_COUNT(AVS3_RDMA_BUF_SIZE), 1},
};
static const struct vdec_res_buf avs3_rpm_buf[] = {
	{PAGE_COUNT(AVS3_RPM_BUF_SIZE), 1},
};
static const struct vdec_res_buf avs3_lmem_buf[] = {
	{PAGE_COUNT(AVS3_LMEM_BUF_SIZE), 1},
};
static const struct vdec_res_buf avs3_mmu_buf[] = {
	{PAGE_COUNT(AVS3_MMU_BUF_SIZE), 1},
};
static const int avs3_mv_num[] = {
	AVS3_MV_NUM_480P,
	AVS3_MV_NUM_720P,
	AVS3_MV_NUM_1080P,
	AVS3_MV_NUM_4K,
	AVS3_MV_NUM_8K,
};
static const int avs3_dpb_num[] = {
	AVS3_DPB_NUM_480P,
	AVS3_DPB_NUM_720P,
	AVS3_DPB_NUM_1080P,
	AVS3_DPB_NUM_4K,
	AVS3_DPB_NUM_8K,
};

static inline int get_avs3_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_wk_buf[0].buf_size;
}

static inline int get_avs3_mv_buf(struct resman_cb_est_param_t *est_param)
{
	int mv_num = 0;

	if (est_param->dpb_num > 0) {
		mv_num = est_param->dpb_num;
	} else {
		mv_num = avs3_mv_num[est_param->res_type];
	}
	return avs3_mv_buf[est_param->res_type].buf_size * mv_num;
}

static inline int get_avs3_cuva_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_cuva_buf[0].buf_size;
}

static inline int get_avs3_rdma_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_rdma_buf[0].buf_size;
}

static inline int get_avs3_rpm_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_rpm_buf[0].buf_size;
}

static inline int get_avs3_lmem_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_lmem_buf[0].buf_size;
}

static inline int get_avs3_mmu_buf(struct resman_cb_est_param_t *est_param)
{
	return avs3_mmu_buf[0].buf_size;
}
static inline int get_avs3_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			yuv_num /= 3;
	} else {
		yuv_num = avs3_dpb_num[est_param->res_type];
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}
static inline int get_avs3_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		avbc_nums = est_param->margin_num + est_param->dpb_num;
	else
		avbc_nums = avs3_dpb_num[est_param->res_type];
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_avs3_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_avs3_wk_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs3 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_avs3_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int st_buf = 0;
	int avbc_body_buf = 0;
	int mv_buf = 0;
	/* Module1: wk buffer */
	wk_buf = get_avs3_wk_buf(est_param);
	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);
	/* Module3: avbc body size */
	avbc_body_buf = get_avbc_body_size(est_param);
	/* Module4: mv size */
	mv_buf = get_avs3_mv_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs3 min buf dtv: wk %d, avbc_body %d, st %d, mv %d\n",
			wk_buf, avbc_body_buf, st_buf, mv_buf);
	return (wk_buf + avbc_body_buf + st_buf + mv_buf);
}

static inline int get_avs3_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs3_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs3_min_buf_dtv(est_param);
	return 0;
}

static inline int get_avs3_expected_buf_mm(struct resman_cb_est_param_t *est_param)
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
	wk_buf = get_avs3_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_avs3_mv_buf(est_param);

	/* Module4: cuva buffer */
	cuva_buf = get_avs3_cuva_buf(est_param);

	/* Module5: yuv buffer */
	yuv_buf = get_avs3_yuv_buf(est_param);

	/* Module6: avbc header buffer */
	avbc_header_buf = get_avs3_avbc_header_buf(est_param);

	/* Module7: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);

	/* Module8: rdma buffer */
	rdma_buf = get_avs3_rdma_buf(est_param);

	/* Module9: rpm buffer */
	rpm_buf = get_avs3_rpm_buf(est_param);

	/* Module10: lmem buffer */
	lmem_buf = get_avs3_lmem_buf(est_param);

	/* Module11: mmu buffer */
	mmu_buf = get_avs3_mmu_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("avs3 expect buf: wk %d, es %d, mv %d, cuva %d, yuv %d\n",
			wk_buf, es_buf, mv_buf, cuva_buf, yuv_buf);
		pr_info("avbc_header %d, avbc_body %d, rdma %d, rpm %d\n",
			avbc_header_buf, avbc_body_buf, rdma_buf, rpm_buf);
		pr_info("lmem %d, mmu %d\n", lmem_buf, mmu_buf);
	}
	return (wk_buf + es_buf + mv_buf + cuva_buf + yuv_buf + avbc_header_buf
			+ avbc_body_buf + rdma_buf + rpm_buf + lmem_buf + mmu_buf);
}

static inline int get_avs3_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int mv_buf = 0;
	int cuva_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int mmu_buf = 0;
	/* Module1: wk buffer */
	wk_buf = get_avs3_wk_buf(est_param);
	/* Module2: mv buffer */
	mv_buf = get_avs3_mv_buf(est_param);
	/* Module3: cuva buffer */
	cuva_buf = get_avs3_cuva_buf(est_param);
	/* Module4: yuv buffer */
	yuv_buf = get_avs3_yuv_buf(est_param);
	/* Module5: avbc header buffer */
	avbc_header_buf = get_avs3_avbc_header_buf(est_param);
	/* Module6: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);
	/* Module7: mmu buffer */
	mmu_buf = get_avs3_mmu_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("avs3 expect buf dtv: wk %d, mv %d, cuva %d, yuv %d\n",
			wk_buf, mv_buf, cuva_buf, yuv_buf);
		pr_info("avbc_header %d, avbc_body %d, mmu %d\n",
			avbc_header_buf, avbc_body_buf, mmu_buf);
	}
	return (wk_buf + mv_buf + cuva_buf + yuv_buf + avbc_header_buf +
		avbc_body_buf + mmu_buf);
}

static inline int get_avs3_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs3_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs3_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_avs3_mem_est_if = {
	.get_min_size		= get_avs3_min_bufsize,
	.get_expected_size	= get_avs3_expected_bufsize,
};

struct vdec_common_mem_est_if *get_avs3_mem_est_if(void)
{
	return &vdec_avs3_mem_est_if;
}

