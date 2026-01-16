// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mm.h>
#include "vdec_res_priv.h"

#define VC1_WK_BUF_SIZE			2097152
#define VC1_ST_BUF_SIZE			3145728
#define VC1_FETCH_BUF_SIZE		2097152
#define VC1_VFRAME_INPUT_BUF_SIZE	4096

#define VC1_DPB_NUM			10

static const struct vdec_res_buf vc1_wk_buf[] = {
	{PAGE_COUNT(VC1_WK_BUF_SIZE), 1},
};

static const struct vdec_res_buf vc1_st_buf[] = {
	{PAGE_COUNT(VC1_ST_BUF_SIZE), 1},
};

static const struct vdec_res_buf vc1_fetch_buf[] = {
	{PAGE_COUNT(VC1_FETCH_BUF_SIZE), 1},
};

static const struct vdec_res_buf vc1_vframe_input_buf[] = {
	{PAGE_COUNT(VC1_VFRAME_INPUT_BUF_SIZE), 1},
};

static inline int get_vc1_wk_buf(struct resman_cb_est_param_t *est_param)
{
	return vc1_wk_buf[0].buf_size;
}

static inline int get_vc1_st_buf(struct resman_cb_est_param_t *est_param)
{
	return vc1_st_buf[0].buf_size;
}

static inline int get_vc1_fetch_buf(struct resman_cb_est_param_t *est_param)
{
	return vc1_fetch_buf[0].buf_size;
}

static inline int get_vc1_vframe_input_buf(struct resman_cb_est_param_t *est_param)
{
	return vc1_vframe_input_buf[0].buf_size;
}

static inline int get_vc1_yuv_buf(struct resman_cb_est_param_t *est_param)
{
	int yuv_num = 0;
	if (est_param->margin_num > 0 && est_param->dpb_num > 0) {
		yuv_num = est_param->margin_num + est_param->dpb_num;
		if (est_param->is_interlace &&
			is_v4l2_pipeline(est_param))
			yuv_num /= 3;
	} else {
		yuv_num = VC1_DPB_NUM;
	}
	return get_common_yuv_buf_size(est_param) * yuv_num;
}

static inline int get_vc1_min_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int prealloc_buf = 0;

	/* Module1: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module2: wk buffer */
	wk_buf = get_vc1_wk_buf(est_param);

	/* Module3: prealloc buffer */
	/* prealloc_buf = get_vc1_prealloc_buf(est_param); */

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("vc1 min buf: wk %d, es %d, prealloc %d\n",
			wk_buf, es_buf, prealloc_buf);
	return (wk_buf + es_buf + prealloc_buf);
}

static inline int get_vc1_min_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int st_buf = 0;
	int yuv_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_vc1_wk_buf(est_param);

	/* Module2: stbuf, it's common size */
	st_buf = get_common_st_buf_size(est_param);

	/* Module3: yuv size */
	yuv_buf = get_common_yuv_buf_size(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("vc1 min buf dtv: wk %d, st %d, yuv %d\n",
			wk_buf, st_buf, yuv_buf);
	return (wk_buf + st_buf + yuv_buf);
}

static inline int get_vc1_min_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_vc1_min_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_vc1_min_buf_dtv(est_param);
	return 0;
}

static inline int get_vc1_expected_buf_mm(struct resman_cb_est_param_t *est_param)
{
	int wk_buf = 0;
	int es_buf = 0;
	int yuv_buf = 0;

	/* Module1: wk buffer */
	wk_buf = get_vc1_wk_buf(est_param);

	/* Module2: es buffer */
	es_buf = get_common_es_buf_size(est_param);

	/* Module3: yuv buffer */
	yuv_buf = get_vc1_yuv_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("vc1 expect buf: wk %d, es %d, yuv_buf %d\n",
			wk_buf, es_buf, yuv_buf);
	return (wk_buf + es_buf + yuv_buf);
}

static inline int get_vc1_expected_buf_dtv(struct resman_cb_est_param_t *est_param)
{
	int vframe_input_buf = 0;
	int wk_buf = 0;
	int yuv_buf = 0;
	/* Module1: vframe input buffer, it's common size */
	vframe_input_buf = get_vc1_vframe_input_buf(est_param);
	/* Module2: wk buffer */
	wk_buf = get_vc1_wk_buf(est_param);
	/* Module3: yuv size */
	yuv_buf = get_vc1_yuv_buf(est_param);

	if (vdec_get_debug_flags() & 0x10000000)
		pr_info("vc1 expect buf dtv: vframe_input %d, wk %d, yuv %d\n",
			vframe_input_buf, wk_buf, yuv_buf);
	return (vframe_input_buf + wk_buf +	yuv_buf);
}

static inline int get_vc1_expected_bufsize(struct resman_cb_est_param_t *est_param)
{
	if (is_v4l2_pipeline(est_param))
		return get_vc1_expected_buf_mm(est_param);
	else if (is_amport_pipeline(est_param))
		return get_vc1_expected_buf_dtv(est_param);
	return 0;
}

static struct vdec_common_mem_est_if vdec_vc1_mem_est_if = {
	.get_min_size		= get_vc1_min_bufsize,
	.get_expected_size	= get_vc1_expected_bufsize,
};

struct vdec_common_mem_est_if *get_vc1_mem_est_if(void)
{
	return &vdec_vc1_mem_est_if;
}


