// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define AV1_WK_BUF_SIZE		5701632
#define AV1_MV_BUF_SIZE_480P	65536
#define AV1_MV_BUF_SIZE_720P	131072
#define AV1_MV_BUF_SIZE_1080P	196608
#define AV1_MV_BUF_SIZE_4K	655360
#define AV1_MV_BUF_SIZE_8K	2490368
#define AV1_UCODE_LOG_BUF_SIZE	1048576
#define AV1_CODEC_BUF_SIZE	131072
#define AV1_RDMA_BUF_SIZE	16384
#define AV1_RPM_BUF_SIZE	4096
#define AV1_AUX_BUF_SIZE	16384
#define AV1_LMEM_BUF_SIZE	4096
#define AV1_MMU_BUF_SIZE	20480

#define AV1_MV_NUM		9
#define AV1_DPB_NUM		15

static const struct vdec_res_buf av1_wk_buf[] = {
	{PAGE_COUNT(AV1_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_mv_buf[] = {
	{PAGE_COUNT(AV1_MV_BUF_SIZE_480P), 1},
	{PAGE_COUNT(AV1_MV_BUF_SIZE_720P), 1},
	{PAGE_COUNT(AV1_MV_BUF_SIZE_1080P), 1},
	{PAGE_COUNT(AV1_MV_BUF_SIZE_4K), 1},
	{PAGE_COUNT(AV1_MV_BUF_SIZE_8K), 1},
};

static const struct vdec_res_buf av1_ucode_log_buf[] = {
	{PAGE_COUNT(AV1_UCODE_LOG_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_codec_buf[] = {
	{PAGE_COUNT(AV1_CODEC_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_rdma_buf[] = {
	{PAGE_COUNT(AV1_RDMA_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_rpm_buf[] = {
	{PAGE_COUNT(AV1_RPM_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_aux_buf[] = {
	{PAGE_COUNT(AV1_AUX_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_lmem_buf[] = {
	{PAGE_COUNT(AV1_LMEM_BUF_SIZE), 1},
};

static const struct vdec_res_buf av1_mmu_buf[] = {
	{PAGE_COUNT(AV1_MMU_BUF_SIZE), 1},
};

static inline int get_av1_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return av1_wk_buf[0].buf_size;
}

static inline int get_av1_mv_buf(struct resman_cb_est_param_t *est_param)
{
	int mv_num = 0;

	if (est_param->dpb_num > 0) {
		mv_num = est_param->dpb_num - 1;
	} else {
		mv_num = AV1_MV_NUM;
	}
	return av1_mv_buf[est_param->res_type].buf_size * mv_num;
}

static inline int get_av1_ucode_log_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_ucode_log_buf[0].buf_size;
}

static inline int get_av1_codec_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_codec_buf[0].buf_size;
}

static inline int get_av1_rdma_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_rdma_buf[0].buf_size;
}

static inline int get_av1_rpm_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_rpm_buf[0].buf_size;
}

static inline int get_av1_aux_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_aux_buf[0].buf_size;
}

static inline int get_av1_lmem_bufsize(struct resman_cb_est_param_t *est_param)
{
	return av1_lmem_buf[0].buf_size;
}

static inline int get_av1_mmu_buf(struct resman_cb_est_param_t *est_param)
{
	return av1_mmu_buf[0].buf_size;
}

static inline int get_av1_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
	} else {
		yuv_num = AV1_DPB_NUM;
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_av1_avbc_header_buf(struct resman_cb_est_param_t *est_param)
{
	int avbc_nums = 0;

	if (est_param->margin_num > 0 && est_param->dpb_num > 0)
		avbc_nums = est_param->margin_num + est_param->dpb_num;
	else
		avbc_nums = AV1_DPB_NUM;
	return get_common_avbc_header_size(est_param) * avbc_nums;
}

static inline int get_av1_prealloc_buf(struct resman_cb_est_param_t *est_param)
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

static inline int get_av1_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_av1_wk_buf(est_param);

	/* Module3: prealloc buffer */
	prealloc_buf = get_av1_prealloc_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("av1 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_av1_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_av1_min_buf_mm(est_param);
	return 0;
}

static inline int get_av1_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int mv_buf = 0;
	int yuv_buf = 0;
	int avbc_header_buf = 0;
	int avbc_body_buf = 0;
	int ucode_log_buf = 0;
	int codec_buf = 0;
	int rdma_buf = 0;
	int rpm_buf = 0;
	int aux_buf = 0;
	int lmem_buf = 0;
	int mmu_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_av1_wk_buf(est_param);

	/* Module3: mv buffer */
	mv_buf = get_av1_mv_buf(est_param);

	/* Module4: yuv buffer */
	yuv_buf = get_av1_yuv_buf(est_param);

	/* Module5: avbc header buffer */
	avbc_header_buf = get_av1_avbc_header_buf(est_param);

	/* Module6: avbc body buffer */
	avbc_body_buf = get_avbc_body_size(est_param);

	/* Module7: ucode log buffer */
	ucode_log_buf = get_av1_ucode_log_bufsize(est_param);

	/* Module8: codec buffer */
	codec_buf = get_av1_codec_bufsize(est_param);

	/* Module9: rdma buffer */
	rdma_buf = get_av1_rdma_bufsize(est_param);
	/* Module10: rpm buffer */
	rpm_buf = get_av1_rpm_bufsize(est_param);

	/* Module11: aux buffer */
	aux_buf = get_av1_aux_bufsize(est_param);

	/* Module12: lmem buffer */
	lmem_buf = get_av1_lmem_bufsize(est_param);

	/* Module13: mmu buffer */
	mmu_buf = get_av1_mmu_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
	{
		pr_info("av1 expect buf: wk %d, es %d, mv %d, yuv %d, avbc_header %d\n",
			wk_buf, es_buf, mv_buf, yuv_buf, avbc_header_buf);
		pr_info("avbc_body %d, ucode_log %d, codec %d, rdma %d, rpm %d\n",
			avbc_body_buf, ucode_log_buf, codec_buf, rdma_buf, rpm_buf);
		pr_info("aux %d, lmem %d, mmu %d\n",
			aux_buf, lmem_buf, mmu_buf);
	}

	return (wk_buf + es_buf + mv_buf + yuv_buf +
			avbc_header_buf + avbc_body_buf + ucode_log_buf + codec_buf
			+ rdma_buf + rpm_buf + aux_buf + lmem_buf + mmu_buf);
}

static inline int get_av1_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_av1_expected_buf_mm(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_av1_mem_est_if = {
	.get_min_size		= get_av1_min_bufsize,
	.get_expected_size	= get_av1_expected_bufsize,
};

struct vdec_common_mem_est_if *get_av1_mem_est_if(void)
{
	return &vdec_av1_mem_est_if;
}


