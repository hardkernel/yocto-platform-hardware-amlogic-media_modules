// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef _VDEC_RES_H_
#define _VDEC_RES_H_

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/amlogic/media/resource_mgr/resourcemanage.h>

int query_min_memory_func(struct resman_cb_est_param_t *est_param);
int query_expected_memory_func(struct resman_cb_est_param_t *est_param);
int query_current_memory_func(struct resman_cb_dec_status_t *dst_param);


#endif /* _VDEC_RES_H_ */
