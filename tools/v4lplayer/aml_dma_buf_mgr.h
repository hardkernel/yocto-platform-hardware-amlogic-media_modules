/*
 * Copyright (c) 2025 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef _AML_DMA_BUF_MRG_H_
#define _AML_DMA_BUF_MRG_H_


int dma_buf_mgr_alloc(int block_size, void** mapped_vaddr, void** paddr, int* dma_fd);

void dma_buf_mgr_free(void);

int alloc_dma_stream_buf(void);
int dma_buf_stream_buf_write(char* buf, int size);
void dma_buf_stream_buf_free(int size);
#endif