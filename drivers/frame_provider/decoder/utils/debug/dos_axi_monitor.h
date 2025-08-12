/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DOS_AXI_MON__
#define __DOS_AXI_MON__

enum {
	ENABLE_PROTECT = 0x1,
	ENABLE_MONITOR = 0x2,
};

#define MAX_HW_IGN 4
#define MAX_SW_IGN 4

struct dos_axi_monitor_t {
	ulong protect_start;
	ulong protect_size;
	ulong res_start;
	u32 res_size;

	ulong monitor_start;
	ulong monitor_size;
	u32 monitor_id;

	u32 monitor_ign_id[MAX_HW_IGN];
	u32 sw_ign_id[MAX_SW_IGN];

	u32 axi_monitor_ctl;

	//status
	u32 monitor_err_addr;
	u32 monitor_err_id;
	/*
	int (*protect_cfg)(ulong start, u32 size, ulong res, u32 res_size);
	int (*monitor_cfg)(ulong start, u32 size, u32 port);
	int (*monitor_ign)(u32 *port, u32 num);
	*/
};

irqreturn_t dec_axi_monitor_isr(void);

u32 show_dos_axi_monitor_config(char *buf);

ssize_t axi_monitor_config_setup(const char *buf, size_t size);

bool is_axi_mon_enabled(void);

void show_axi_mon_reg(void);

void hw_axi_monitor_config(void);

#define AMRISC_LMEM_WRITE     0x08
#define SWAP_WRITE            0x12
#define VP9_PROCESS_WRITE     0x15
#define VP9_MAP_PROCESS_WRITE 0x16  //scatter input
#define AV1_CONTEXT_WRITE     0x1b
#define AV1_SEGMENT_WRITE     0x1c
#define AV1_TOP_WRITE         0x1d
#define AV1_GMC_WRITE         0x1e
#define IPP_LINEBUF_WR_CACHE  0x20
#define MPRED_WRITE           0x32
#define MPP_COMV_WRITE        0x50  //~ 0x5fs
#define COMPRESS_FIFO2_WRITE  0x60
#define COMPRESS_TILE_WRITE   0x61
#define COMPRESS_HEADER_WRITE 0x62
#define COMPRESS_BODY_WRITE   0x63
#define OW_SAO_WRITE_DATA0    0x74
#define OW_SAO_WRITE_DATA1    0x75
#define OW_SAO_WRITE_DATA2    0x76
#define OW_SAO_WRITE_DATA3    0x77
#define PSCALE_XIO_TOP_WRITE  0x80
#define XIO_LEFT_WRITE        0x81
#define VLD_MEM_WRITE         0x90
#define H264_TOP_BUFF_WRITE   0xa0
#define CO_MB_WRITE           0xb1
#define IQIDCT_CANVAS_WRITE   0xc0
#define MC_MBBOT_WRITE        0xd0
#define EXTIF_BUF0_WRITE      0xe0
#define EXTIF_BUF1_WRITE      0xe1
#define EXTIF_BUF2_WRITE      0xe2
#define EXTIF_BUF3_WRITE      0xe3
#define VC1_OST_WRITE         0xf0
#define XIO_XOT_WRITE         0x80
#define XIO_XOL_WRITE         0x81
#define CPI_WRITE0            0x90
#define CPI_WRITE1            0x91
#define EVAN_WRITE            0xa0
#endif
