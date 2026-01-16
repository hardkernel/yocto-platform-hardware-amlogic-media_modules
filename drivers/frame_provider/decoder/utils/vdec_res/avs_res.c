// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define AVS_WK_BUF_SIZE		4194304
#define AVS_AUX_BUF_SIZE	8192
#define AVS_LMEM_BUF_SIZE	4096

#define AVS_DPB_NUM_480P	11
#define AVS_DPB_NUM_720P	11
#define AVS_DPB_NUM_1080P	7

static const struct vdec_res_buf avs_wk_buf[] = {
	{PAGE_COUNT(AVS_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs_aux_buf[] = {
	{PAGE_COUNT(AVS_AUX_BUF_SIZE), 1},
};

static const struct vdec_res_buf avs_lmem_buf[] = {
	{PAGE_COUNT(AVS_LMEM_BUF_SIZE), 1},
};

static const int avs_dpb_num[] = {
	AVS_DPB_NUM_480P,
	AVS_DPB_NUM_720P,
	AVS_DPB_NUM_1080P,
	0,0,
};

static inline int get_avs_aux_buf(struct resman_cb_est_param_t *est_param)
{
	return avs_aux_buf[0].buf_size;
}

static inline int get_avs_lmem_buf(struct resman_cb_est_param_t *est_param)
{
	return avs_lmem_buf[0].buf_size;
}

static inline int get_avs_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return avs_wk_buf[0].buf_size;
}

static inline int get_avs_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;

	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			yuv_num /= 3;
	} else {
		yuv_num = avs_dpb_num[est_param->res_type];
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_avs_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_avs_wk_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs min buf: wk %d, es %d\n",
			wk_buf, es_buf);
	return (wk_buf + es_buf);
}

static inline int get_avs_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int st_buf = 0;
	int yuv_buf = 0;

	/* Module1: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);

	/* Module2: yuv size */
	yuv_buf = get_common_yuv_buf_size(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs min buf dtv: st %d, yuv %d\n",
			st_buf, yuv_buf);
	return (st_buf + yuv_buf);
}

static inline int get_avs_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs_min_buf_dtv(est_param);
	return 0;
}

static inline int get_avs_expected_bufsize_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int aux_buf = 0;
	int lmem_buf = 0;
	int es_buf = 0;
	int yuv_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_avs_wk_buf(est_param);

	/* Module3: aux buffer */
	aux_buf = get_avs_aux_buf(est_param);

	/* Module4: lmem buffer */
	lmem_buf = get_avs_lmem_buf(est_param);

	/* Module5: yuv size */
	yuv_buf = get_avs_yuv_buf(est_param);
	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs expect buf: es %d, wk %d, aux %d, lmem %d, yuv %d\n",
			es_buf, wk_buf, aux_buf, lmem_buf, yuv_buf);
	return (es_buf + wk_buf + aux_buf + lmem_buf + yuv_buf);
}

static inline int get_avs_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int yuv_buf = 0;
	int aux_buf = 0;
	int lmem_buf = 0;
	/* Module1: wk buffer */
	wk_buf = get_avs_wk_buf(est_param);
	/* Module2: yuv size */
	yuv_buf = get_avs_yuv_buf(est_param);
	/* Module3: aux buffer */
	aux_buf = get_avs_aux_buf(est_param);
	/* Module4: lmem buffer */
	lmem_buf = get_avs_lmem_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("avs expect buf dtv: wk %d, yuv %d, aux %d, lmem %d\n",
			wk_buf, yuv_buf, aux_buf, lmem_buf);
	return (wk_buf + yuv_buf + aux_buf + lmem_buf);
}

static inline int get_avs_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_avs_expected_bufsize_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_avs_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_avs_mem_est_if = {
	.get_min_size		= get_avs_min_bufsize,
	.get_expected_size	= get_avs_expected_bufsize,
};

struct vdec_common_mem_est_if *get_avs_mem_est_if(void)
{
	return &vdec_avs_mem_est_if;
}


