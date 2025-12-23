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
#ifndef _MEDIA_FILE_H_
#define _MEDIA_FILE_H_
#include <linux/fs.h>
#include <linux/vmalloc.h>


#define VDEC_MODE_MMU_DW_MASK	(0x20)
#define VDEC_MODE_10BIT_MASK	(0x10000)
#define VDEC_MODE_DW_MASK	(0xffff)

enum vdec_dec_mode {
	DM_INVALID		= 0,
	DM_AVBC_ONLY		= 0,
	DM_YUV_1_1_AVBC		= 1,
	DM_YUV_1_4_AVBC_A	= 2,
	DM_YUV_1_4_AVBC_B	= 3,
	DM_YUV_1_2_AVBC		= 4,
	DM_YUV_1_8_AVBC		= 8,
	DM_YUV_ONLY		= 0x10,
	DM_AVBC_1_1		= 0x21,
	DM_AVBC_1_4		= 0x22,
	DM_AVBC_1_2		= 0x24,
	DM_YUV_AUTO_1_2_AVBC	= 0x100,
	DM_YUV_AUTO_1_4_AVBC	= 0x200,
	DM_YUV_AUTO_1_2_AVBC_B	= 0x300,
	/* (0~540] 1/1, (540~1080] 1/4, (1080~4K] 1/16 */
	DM_YUV_AUTO_14_12_AVBC	= 0x400,
	DM_YUV_1_1_10BIT_AVBC	= 0x10001,
	DM_YUV_1_4_10BIT_AVBC	= 0x10003,
	DM_YUV_1_2_10BIT_AVBC	= 0x10004,
	DM_YUV_1_8_10BIT_AVBC	= 0x10008,
	DM_YUV_P010_ONLY	= 0x10010,
	/* (0~1080] 1/1, (1080~4K] 1/16 */
	DM_YUV_14_11_10BIT_AVBC	= 0x10200,
};

#define PAGE_COUNT(x) (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)


typedef unsigned long dos_addr_t;

inline void *aml_media_mem_alloc(size_t size, gfp_t flags);
inline void aml_media_mem_free(const void *addr);

ssize_t media_write(struct file *, const void *, size_t, loff_t *);
ssize_t media_read(struct file *, void *, size_t, loff_t *);
struct file *media_open(const char *, int, umode_t);
int media_close(struct file *, fl_owner_t);

typedef int (*dhp_func)(void *, void *, void *, int);

/**
 * dhp_func_reg - Register a data handler proxy (DHP) function.
 *
 * This function allows for the registration of a callback function that
 * will be invoked for processing data within the DHP framework.
 *
 * @fn: Pointer to the function to be registered. The function should match
 *      the signature of the dhp_func type, which takes four void pointers
 *      and an integer as arguments and returns an integer.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int dhp_func_reg(dhp_func fn);

/**
 * dhp_func_unreg - Unregister the currently registered DHP function.
 *
 * This function removes the previously registered data handler function,
 * stopping any further calls to it within the DHP framework.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int dhp_func_unreg(void);

/**
 * dhp_func_request - Request a data processing operation in the DHP framework.
 *
 * This function initiates a data processing task by submitting the source
 * and destination buffers along with any relevant metadata. It queues the
 * request for processing by the DHP and returns an identifier for the request.
 *
 * @src: Pointer to the source data buffer where the input data is located.
 * @dst: Pointer to the destination data buffer where the processed output
 *       will be stored.
 * @meta: Pointer to metadata associated with the request, providing additional
 *        information or parameters necessary for processing.
 * @size: Size of the metadata in bytes, indicating how much metadata is being
 *        provided.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
int dhp_func_request(void *src, void *dst, void *meta, int size);

#endif
