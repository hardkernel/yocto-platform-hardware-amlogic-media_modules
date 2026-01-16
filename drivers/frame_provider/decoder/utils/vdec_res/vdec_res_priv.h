// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef _VDEC_RES_PRIV_H_
#define _VDEC_RES_PRIV_H_

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/amlogic/media/registers/cpu_version.h>
#include <linux/amlogic/media/resource_mgr/resourcemanage.h>
#include "../../../../common/media_utils/media_utils.h"
#include "../vdec.h"

#define PAGE_COUNT(x) 			(((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
/* for dtv, mpeg12, mpeg4, avs, vc1 */
#define YUV_BUF_2K 			(0x300000)
/* for dtv, mjpeg */
#define YUV_BUF_4K 			(0xd80000)

#define PREALLOC_BUF_SIZE		(24 * SZ_1M)

#define ES_BUF_SIZE_2K			(4 * SZ_1M)
#define ES_BUF_SIZE_4K			(12 * SZ_1M)
#define ES_BUF_SIZE_8K			(16 * SZ_1M)

#define AVBC_HEADER_BUF_SIZE_2K		65536
#define AVBC_HEADER_BUF_SIZE_4K		294912
#define AVBC_HEADER_BUF_SIZE_8K		1179648

#define ST_BUF_SIZE			10485760
#define FETCH_BUF_SIZE			2097152
#define VFRAME_INPUT_BUF_SIZE		4096

#define AVBC_BODY_8BIT_BUF_SIZE_480P	144
#define AVBC_BODY_8BIT_BUF_SIZE_720P	360
#define AVBC_BODY_8BIT_BUF_SIZE_1080P	768
#define AVBC_BODY_8BIT_BUF_SIZE_4K	3188
#define AVBC_BODY_8BIT_BUF_SIZE_8K	12240

#define AVBC_BODY_10BIT_BUF_SIZE_480P	180
#define AVBC_BODY_10BIT_BUF_SIZE_720P	456
#define AVBC_BODY_10BIT_BUF_SIZE_1080P	1024
#define AVBC_BODY_10BIT_BUF_SIZE_4K	4096
#define AVBC_BODY_10BIT_BUF_SIZE_8K	15300

struct vdec_res_buf {
	u32 buf_size;
	u32 align;
};

enum vdec_res_type {
	VDEC_RES_INVALID = -1,
	VDEC_RES_480P = 0,
	VDEC_RES_720P = 1,
	VDEC_RES_1080P = 2,
	VDEC_RES_4K = 3,
	VDEC_RES_8K = 4,
	VDEC_RES_MAX,
};

enum vdec_res_group_type {
	VDEC_RES_GROUP_INVALID = -1,
	VDEC_RES_GROUP_2K = 0,
	VDEC_RES_GROUP_4K = 1,
	VDEC_RES_GROUP_8K = 2,
	VDEC_RES_GROUP_MAX,
};

struct vdec_common_mem_est_if {
	int (*get_min_size)(struct resman_cb_est_param_t *est_param);

	int (*get_expected_size)(struct resman_cb_est_param_t *est_param);
};

static inline enum vdec_res_group_type get_res_group_type(enum vdec_res_type res_type)
{
	if (res_type <= VDEC_RES_INVALID || res_type >= VDEC_RES_MAX)
		return VDEC_RES_GROUP_INVALID;

	if (res_type <= VDEC_RES_1080P)
		return VDEC_RES_GROUP_2K;
	if (res_type <= VDEC_RES_4K)
		return VDEC_RES_GROUP_4K;

	return VDEC_RES_GROUP_8K;
}

bool is_2k_vdec_platform(void);
bool is_v4l2_pipeline(struct resman_cb_est_param_t *est_param);
bool is_amport_pipeline(struct resman_cb_est_param_t *est_param);
int get_common_es_buf_size(struct resman_cb_est_param_t *est_param);
int get_common_yuv_buf_size(struct resman_cb_est_param_t *est_param);
int get_common_st_buf_size(struct resman_cb_est_param_t *est_param);
int get_common_avbc_header_size(struct resman_cb_est_param_t *est_param);
int get_avbc_body_size(struct resman_cb_est_param_t *est_param);
int get_common_fetch_buf_size(struct resman_cb_est_param_t *est_param);
int get_common_vframe_input_buf_size(struct resman_cb_est_param_t *est_param);

struct vdec_common_mem_est_if *vdec_mem_est_if(int video_type);

struct vdec_common_mem_est_if *get_h264_mem_est_if(void);
struct vdec_common_mem_est_if *get_h265_mem_est_if(void);
struct vdec_common_mem_est_if *get_h266_mem_est_if(void);
struct vdec_common_mem_est_if *get_av1_mem_est_if(void);
struct vdec_common_mem_est_if *get_vp9_mem_est_if(void);
struct vdec_common_mem_est_if *get_mpeg12_mem_est_if(void);
struct vdec_common_mem_est_if *get_mpeg4_mem_est_if(void);
struct vdec_common_mem_est_if *get_mjpeg_mem_est_if(void);
struct vdec_common_mem_est_if *get_avs_mem_est_if(void);
struct vdec_common_mem_est_if *get_avs2_mem_est_if(void);
struct vdec_common_mem_est_if *get_avs3_mem_est_if(void);
struct vdec_common_mem_est_if *get_vc1_mem_est_if(void);

#endif /* _VDEC_RES_PRIV_H_ */

