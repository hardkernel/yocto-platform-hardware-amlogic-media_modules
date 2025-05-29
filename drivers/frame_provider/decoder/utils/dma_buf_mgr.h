/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef __DMA_BUF_MGR_H__
#define __DMA_BUF_MGR_H__

#include <linux/types.h>

// this sturct is coped from v4l framework and will be used in v4l framework.
struct dmx_dma_buf_sec_es_data {
	__u32 magic_num;
	__u8 pts_dts_flag;
	__u64 video_pts;
	__u64 video_dts;
	__u64 buf_start;
	__u64 buf_end;
	__u64 data_start;
	__u64 data_end;
	__u64 buf_rp;
	__u64 av_handle;
	__u32 token;
	__u64 extend_addr;
	__u32 extend_size;
};

struct dma_buf_info {
	__u32 version;
	__u64 paddr;
	__u32 size;
	__u32 handle;
	__u32 fd;
	struct dmx_dma_buf_sec_es_data dmxes;
	__u64 reserved[8];
};

struct dmabuf_manage_block {
	__u64 paddr;
	__u32 size;
	__u32 handle;
	struct dmx_dma_buf_sec_es_data dmxes;
};

#define DMA_BUF_MANAGE_IOC_MAGIC			'S'
#define DMA_BUF_MANAGE_EXPORT_DMA _IOWR(DMA_BUF_MANAGE_IOC_MAGIC, 1, struct dma_buf_info)

#define DMA_BUF_MANAGE_GET_LAST_RELEASED_DATA_INFO _IOWR(DMA_BUF_MANAGE_IOC_MAGIC, 2, struct dmx_dma_buf_sec_es_data)
int dma_buf_mgr_module_init(void);

void dma_buf_mgr_module_exit(void);

#endif