/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __VDEC_DW__
#define __VDEC_DW__

#include "../../../amvdec_ports/utils/aml_output_mode_define.h"

#define SUPPORT_DW_MODE_MAX  14
#define SUPPORT_AVBC_FMT_MAX 7

struct dos_hw_dw_mode {
	enum vformat_e fmt;
	int hw_dw_mode[32];
};

struct dos_hw_output_mode {
	enum vformat_e fmt;
	struct output_mode mode[3];
};

struct dos_hw_dw_mode* get_dos_dw_mode_all(void);
struct dos_hw_output_mode* get_dos_output_mode_all(void);
int *get_dos_dw_mode(enum vformat_e fmt);

void vdec_dos_dw_mode_init(void);
void pr_dw_infos(void);

#endif

