/*
 * Copyright (c) 2025 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "aml_dma_buf_mgr.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "vcodec_utils.h"

#define STREAM_BUF_SIZE (15*1024*1024)

#define DMA_BUF_MANAGER_MAX_BUFFER_LEN 320
#define VIDEODEC_DATA_MAX_LEN 256
#define DEMUX_ES_MAGIC_NUM 0x5a5a5a5a
#define DMABUF_MANAGE_IOC_MAGIC              'S'

#define DMABUF_MANAGE_EXTEND_EXPORT_DMABUF _IOWR(DMABUF_MANAGE_IOC_MAGIC, 201, struct dmabuf_manage_extend_buffer)
#define DMABUF_MANAGE_EXTEND_GET_DMABUFINFO _IOWR(DMABUF_MANAGE_IOC_MAGIC, 202, struct dmabuf_manage_extend_buffer)
#define DMABUF_MANAGE_EXTEND_ALLOCDMABUF   _IOWR(DMABUF_MANAGE_IOC_MAGIC, 204, struct dmabuf_manage_extend_buffer)
#define DMABUFMANAGE_DEV "/dev/secmem"


struct dmabuf_videodec_es_data {
	uint32_t data_type;
	uint8_t  data[VIDEODEC_DATA_MAX_LEN];
	uint32_t data_len;
};

struct dmabuf_manage_extend_buffer {
	uint64_t type;
	uint64_t fd;
	uint64_t paddr;
	uint64_t size;
	uint64_t handle;
	uint64_t flags;
	uint64_t extend;
	union {
		struct dmabuf_videodec_es_data vdecdata;
		uint8_t data[DMA_BUF_MANAGER_MAX_BUFFER_LEN];
    } buffer;
};

struct secmem_extend_block {
	uint64_t paddr;
	uint64_t size;
	uint64_t handle;
};

struct dma_ring_buf {
	uint64_t paddr; // buf_start addr in phy
	uint64_t vaddr; // buf_start addr in virt
	uint64_t size;  // buf total size
	uint64_t start; // data start addr , read point
	uint64_t end;  // data end addr , write point
	uint64_t payload;  // current valid data size
};

struct dmx_dma_buf_sec_es_data {
	uint32_t magic_num;
	uint8_t pts_dts_flag;
	uint64_t video_pts;
	uint64_t video_dts;
	uint64_t buf_start;
	uint64_t buf_end;
	uint64_t data_start;
	uint64_t data_end;
	uint64_t buf_rp;
	uint64_t av_handle;
	uint32_t token;
	uint64_t extend_addr;
	uint32_t extend_size;
};

struct dma_buf_info {
	uint32_t version;
	uint64_t paddr;
	uint32_t size;
	uint32_t handle;
	uint32_t fd;
	struct dmx_dma_buf_sec_es_data dmxes;
	uint64_t reserved[8];
};

#define DMA_BUF_MANAGE_IOC_MAGIC			'S'
#define DMA_BUF_MANAGE_EXPORT_DMA _IOWR(DMA_BUF_MANAGE_IOC_MAGIC, 1, struct dma_buf_info)




static int mem_fd = -1;
static int dma_mgr_buf_fd = -1;
static int dma_mgr_dev_fd = -1;
static struct dma_ring_buf ring_buf;

void dma_buf_mgr_free(void) {
	if (mem_fd > 0) {
		close(mem_fd);
		mem_fd = -1;
	}
	if (dma_mgr_buf_fd > 0) {
		close(dma_mgr_buf_fd);
		dma_mgr_buf_fd = -1;
	}
	if (dma_mgr_dev_fd > 0) {
		close(dma_mgr_dev_fd);
		dma_mgr_dev_fd = -1;
	}
	if (ring_buf.vaddr) {
		munmap((void*)ring_buf.vaddr, STREAM_BUF_SIZE);
		ring_buf.vaddr = 0;
	}
}

int dma_buf_mgr_alloc(int block_size, void** mapped_vaddr, void** paddr, int* dma_fd) {
	struct dmabuf_manage_extend_buffer info;
	struct secmem_extend_block block;

	if (mem_fd < 0) {
		mem_fd = open(DMABUFMANAGE_DEV, O_RDWR | O_TRUNC);
	}

	if (mem_fd < 0) {
		debug_print(DEBUG_ERROR, "open mem dev failed\n");
		return -1;
	}

	memset(&info, 0, sizeof(info));
	info.type = 3; // DMA_BUF_TYPE_DMABUF;
	info.size = block_size;
	info.flags = 0;

	*dma_fd = ioctl(mem_fd, DMABUF_MANAGE_EXTEND_ALLOCDMABUF, &info);
	if ( *dma_fd < 0) {
		debug_print(DEBUG_ERROR, "alloc dma buf error\n");
		goto error;
	}

	memset(&info, 0, sizeof(info));
	info.fd = *dma_fd;
	info.type = 3;
	if (ioctl(mem_fd, DMABUF_MANAGE_EXTEND_GET_DMABUFINFO, (unsigned long)(&info)) < 0) {
		debug_print(DEBUG_ERROR, "get dma buf info error\n");
		goto error;
	}
	*paddr = (void*)info.paddr;
	memset(&block, 0, sizeof(block));
	block.paddr = info.paddr;
	block.size = block_size;
	block.handle = 1;

	memset(&info, 0, sizeof(info));
	info.type = 3;
	info.paddr = block.paddr;
	info.size = block.size;
	info.handle = block.handle;

	memcpy(info.buffer.data, &block, sizeof(block));

	*dma_fd = ioctl(mem_fd, DMABUF_MANAGE_EXTEND_EXPORT_DMABUF, &info);
	if ( *dma_fd < 0) {
		debug_print(DEBUG_ERROR, "export dma buf error\n");
		goto error;
	}

	*mapped_vaddr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, *dma_fd, 0);
	if (*mapped_vaddr == MAP_FAILED) {
		debug_print(DEBUG_ERROR, "%s dma mmap failed, %s, %p,  len:%d\n", __func__, strerror(errno), *mapped_vaddr, block_size);
		goto error;
	}

	return 0;

error:
	dma_buf_mgr_free();
	return -1;
}



int alloc_dma_stream_buf() {
	int ret = -1;
	void* paddr;
	void* vaddr;
	dma_mgr_dev_fd = open("/dev/dma_buf_mgr_dev",  O_CLOEXEC);
	if (dma_mgr_dev_fd < 0) {
		debug_print(DEBUG_ERROR, "[%s] dev_fd failed\n", __func__);
		return ret;
	}

	ret = dma_buf_mgr_alloc(STREAM_BUF_SIZE, &vaddr, &paddr, &dma_mgr_buf_fd);

	if (ret != 0) {
		debug_print(DEBUG_ERROR, "[%s] mgr_alloc failed\n", __func__);
		return ret;
	}
	ring_buf.paddr = (uint64_t)paddr;
	ring_buf.size = STREAM_BUF_SIZE;
	ring_buf.vaddr = (uint64_t)vaddr;
	ring_buf.start = (uint64_t)vaddr;
	ring_buf.end = (uint64_t)vaddr;
	ring_buf.payload = 0;
	return 0;
}

int dma_buf_stream_buf_write(char* buf, int size) {
	int ret = -1;
	struct dma_buf_info info = {0};
	if (size <= 0) {
		debug_print(DEBUG_ERROR, "[%s] no data write\n", __func__);
		return -1;
	}
	// if (size > ring_buf.size - ring_buf.payload) {
	//     debug_print(DEBUG_ERROR, "[%s] buf full\n", __func__);
	//     return -1;
	// }

	info.version = 1;
	info.dmxes.buf_start = ring_buf.paddr;
	info.dmxes.buf_end = ring_buf.paddr + STREAM_BUF_SIZE - 1;
	info.size = size;
	info.paddr = ring_buf.paddr + ring_buf.end - ring_buf.vaddr;

	info.dmxes.magic_num =  DEMUX_ES_MAGIC_NUM;
	info.dmxes.data_start = info.paddr;
	// can not get read point, it's meaningless to use 'ring_buf.start',
	// in other word, we cannot know whether buf is full or not, so write buf must sleep.

	// if (ring_buf.start >= ring_buf.end) {
	//     memcpy((void*)ring_buf.end, buf, size);
	//     ring_buf.end += size;
	//     info.dmxes.data_end = ring_buf.paddr + ring_buf.end - ring_buf.vaddr - 1;
	// } else
	if (ring_buf.end + size <= ring_buf.vaddr + STREAM_BUF_SIZE) {
		memcpy((void*)ring_buf.end, buf, size);
		ring_buf.end += size;
		info.dmxes.data_end = ring_buf.paddr + ring_buf.end - ring_buf.vaddr - 1;
		if (ring_buf.end == ring_buf.vaddr + STREAM_BUF_SIZE) {
			ring_buf.end = ring_buf.vaddr;
		}
	}else {
		int first_cp_size = ring_buf.vaddr + STREAM_BUF_SIZE - ring_buf.end;
		memcpy((void*)ring_buf.end, buf, first_cp_size);
		memcpy((void*)ring_buf.vaddr, buf + first_cp_size, size - first_cp_size);
		ring_buf.end = ring_buf.vaddr + size - first_cp_size;
		info.dmxes.data_end = ring_buf.paddr + ring_buf.end - ring_buf.vaddr - 1;
	}
	ring_buf.payload += size;
	ret = ioctl(dma_mgr_dev_fd, DMA_BUF_MANAGE_EXPORT_DMA, &info);
	if (ret < 0 ) {
		debug_print(DEBUG_ERROR, "[%s] ioctl failed\n", __func__);
		return ret;
	}
	return ret;
}

// no use function, maybe used in future
void dma_buf_stream_buf_free(int size) {
	int offset = ring_buf.vaddr +  STREAM_BUF_SIZE - ring_buf.start;
	offset = offset - size;
	ring_buf.start =  offset > 0 ? ring_buf.start + size : ring_buf.vaddr - offset;
	ring_buf.payload -= size;
}