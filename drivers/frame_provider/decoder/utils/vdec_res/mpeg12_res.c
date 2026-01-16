// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define MPEG12_WK_BUF_SIZE	266240
#define MPEG12_AUX_BUF_SIZE	8192

#define MPEG12_DPB_NUM		14

static const struct vdec_res_buf mpeg12_wk_buf[] = {
	{PAGE_COUNT(MPEG12_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf mpeg12_aux_buf[] = {
	{PAGE_COUNT(MPEG12_AUX_BUF_SIZE), 1},
};

static inline int get_mpeg12_aux_buf(struct resman_cb_est_param_t *est_param)
{
	return mpeg12_aux_buf[0].buf_size;
}

static inline int get_mpeg12_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return mpeg12_wk_buf[0].buf_size;
}

static inline int get_mpeg12_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			yuv_num /= 3;
	} else {
		yuv_num = MPEG12_DPB_NUM;
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_mpeg12_prealloc_buf(struct resman_cb_est_param_t *est_param)
{
	int cpu_type = get_cpu_type();
	if (cpu_type == MESON_CPU_MAJOR_ID_T6D) {
		/* to do............... */
		return 0;
	}

	if (cpu_type == MESON_CPU_MAJOR_ID_T6W) {
		/* to do............... */
		return 0;
	}

	return 0;
}

static inline int get_mpeg12_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_mpeg12_wk_buf(est_param);

	/* Module3: prealloc buffer */
	prealloc_buf = get_mpeg12_prealloc_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("mpeg12 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_mpeg12_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int st_buf = 0;
	int yuv_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_mpeg12_wk_buf(est_param);

	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);

	/* Module3: yuv size */
	yuv_buf = get_common_yuv_buf_size(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("mpeg12 min buf dtv: wk %d, st %d, yuv %d\n",
			wk_buf, st_buf, yuv_buf);
	return (wk_buf + st_buf + yuv_buf);
}

static inline int get_mpeg12_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_mpeg12_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_mpeg12_min_buf_dtv(est_param);
	return 0;
}

static inline int get_mpeg12_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int yuv_buf = 0;
	int aux_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_mpeg12_wk_buf(est_param);

	/* Module3: yuv buffer */
	yuv_buf = get_mpeg12_yuv_buf(est_param);

	/* Module4: aux buffer */
	aux_buf = get_mpeg12_aux_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("mpeg12 expect buf: es %d, wk %d, yuv %d, aux %d\n",
			es_buf, wk_buf, yuv_buf, aux_buf);
	return (es_buf + wk_buf + yuv_buf + aux_buf);
}

static inline int get_mpeg12_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int vframe_input_buf = 0;
	int wk_buf = 0;
	int yuv_buf = 0;
	int aux_buf = 0;
	/* Module1: vframe input buffer, it's common size */
	vframe_input_buf = get_common_vframe_input_buf_size(est_param);
	/* Module2: wk buffer */
	wk_buf = get_mpeg12_wk_buf(est_param);
	/* Module3: yuv size */
	yuv_buf = get_mpeg12_yuv_buf(est_param);
	/* Module4: aux buffer */
	aux_buf = get_mpeg12_aux_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("mpeg12 expect buf dtv: vframe_input %d, wk %d, yuv %d, aux %d\n",
			vframe_input_buf, wk_buf, yuv_buf, aux_buf);
	return (vframe_input_buf + wk_buf +
		yuv_buf + aux_buf);
}

static inline int get_mpeg12_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_mpeg12_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_mpeg12_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_mpeg12_mem_est_if = {
	.get_min_size		= get_mpeg12_min_bufsize,
	.get_expected_size	= get_mpeg12_expected_bufsize,
};

struct vdec_common_mem_est_if *get_mpeg12_mem_est_if(void)
{
	return &vdec_mpeg12_mem_est_if;
}


