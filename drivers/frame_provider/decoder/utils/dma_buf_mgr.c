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
#include "dma_buf_mgr.h"

#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <linux/cdev.h>
#include <linux/version.h>
MODULE_IMPORT_NS(DMA_BUF);
#define dprintk(level, fmt, arg...)						\
do {									\
	if (dma_buf_mgr_dbg >= (level))						\
		pr_info("dmabuf_manage: %s: " fmt, __func__, ## arg);\
} while (0)

#define pr_dbg(fmt, args ...)  dprintk(6, fmt, ## args)
#define pr_error(fmt, args ...) dprintk(1, fmt, ## args)
#define pr_inf(fmt, args ...) dprintk(8, fmt, ## args)
#define pr_enter() pr_inf("enter")

struct kdmabuf_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static int dma_buf_mgr_dbg = 1;
module_param(dma_buf_mgr_dbg, int, 0644);

static struct dmx_dma_buf_sec_es_data last_release_data;

static int dmabuf_manage_attach(struct dma_buf *dbuf, struct dma_buf_attachment *attachment)
{
	struct kdmabuf_attachment *attach;
	struct dmabuf_manage_block *block = NULL;
	struct sg_table *sgt;
	struct page *page;
	phys_addr_t phys;
	int ret;
	int sgnum = 1;
	struct dmx_dma_buf_sec_es_data *es = (struct dmx_dma_buf_sec_es_data *)dbuf->priv;
	int len = 0;

	pr_enter();
	attach = (struct kdmabuf_attachment *)
		kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach) {
		pr_error("kzalloc failed\n");
		goto error;
	}
	block = container_of(es, struct dmabuf_manage_block, dmxes);
	phys = block->paddr;
	if (es->data_end < es->data_start)
		sgnum = 2;
	sgt = &attach->sgt;
	ret = sg_alloc_table(sgt, sgnum, GFP_KERNEL);
	if (ret) {
		pr_error("No memory for sgtable");
		goto error_alloc;
	}
	if (block->paddr != es->data_start) {
		pr_error("Invalid buffer info 0x%llx 0x%llx",
			block->paddr, es->data_start);
		goto error_attach;
    }
	if (sgnum == 2) {
		len = PAGE_ALIGN(es->buf_end - es->data_start);
		page = phys_to_page(phys);
		sg_set_page(sgt->sgl, page, len, 0);
		len = PAGE_ALIGN(es->data_end - es->buf_start);
		page = phys_to_page(es->buf_start);
		sg_set_page(sg_next(sgt->sgl), page, len, 0);
	} else {
		page = phys_to_page(phys);
		sg_set_page(sgt->sgl, page, PAGE_ALIGN(block->size), 0);
	}
	attach->dma_dir = DMA_NONE;
	attachment->priv = attach;
	return 0;
error_attach:
	sg_free_table(sgt);
error_alloc:
	kfree(attach);
error:
	return 0;
}

static void dmabuf_manage_detach(struct dma_buf *dbuf,
				struct dma_buf_attachment *attachment)
{
	struct kdmabuf_attachment *attach = attachment->priv;
	struct sg_table *sgt;

	pr_enter();
	if (!attach)
		return;
	sgt = &attach->sgt;
	sg_free_table(sgt);
	kfree(attach);
	attachment->priv = NULL;
}

static struct sg_table *dmabuf_manage_map_dma_buf(struct dma_buf_attachment *attachment,
	enum dma_data_direction dma_dir)
{
	struct kdmabuf_attachment *attach = attachment->priv;
	struct dmabuf_manage_block *block = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 136)
	struct mutex *lock = &attachment->dmabuf->lock;
#endif
	struct sg_table *sgt;

	pr_enter();
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 136)
	mutex_lock(lock);
	pr_dbg("mutex_lock\n");
#endif
	sgt = &attach->sgt;
	if (attach->dma_dir == dma_dir) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 136)
		mutex_unlock(lock);
#endif
		return sgt;
	}
	block = container_of((struct dmx_dma_buf_sec_es_data *)attachment->dmabuf->priv,
				struct dmabuf_manage_block, dmxes);
	sgt->sgl->dma_address = block->paddr;

	sg_dma_len(sgt->sgl) = PAGE_ALIGN(block->size);

	pr_dbg("nents %d, %llx, %d, %d\n", sgt->nents, block->paddr,
			sg_dma_len(sgt->sgl), block->size);
	attach->dma_dir = dma_dir;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 136)
	mutex_unlock(lock);
#endif
	return sgt;
}

static void dmabuf_manage_unmap_dma_buf(struct dma_buf_attachment *attachment,
	struct sg_table *sgt,
	enum dma_data_direction dma_dir)
{
	pr_enter();
}

static void dmabuf_manage_buf_release(struct dma_buf *dbuf)
{
	struct dmabuf_manage_block *block = NULL;
	struct dmx_dma_buf_sec_es_data *es = (struct dmx_dma_buf_sec_es_data *)dbuf->priv;
	memcpy(&last_release_data, es, sizeof(struct dmx_dma_buf_sec_es_data));
	pr_enter();
	block = container_of(es, struct dmabuf_manage_block, dmxes);

	pr_dbg("dma release handle:0x%x, paddr:0x%llx, size:0x%x\n",
		block->handle, block->paddr, block->size);
	kfree(block);
}

static int dmabuf_manage_mmap(struct dma_buf *dbuf, struct vm_area_struct *vma)
{
	struct dmabuf_manage_block *block = NULL;
	struct dmx_dma_buf_sec_es_data *es = (struct dmx_dma_buf_sec_es_data *)dbuf->priv;
	unsigned long addr = vma->vm_start;
	int len = 0;
	int ret = -EFAULT;

	pr_enter();
	block = container_of(es, struct dmabuf_manage_block, dmxes);
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (block->paddr != es->data_start) {
		pr_error("Invalid buffer info %llx %llx",
			block->paddr, es->data_start);
		goto error;
	}
	if (es->data_end >= es->data_start) {
		len = PAGE_ALIGN(es->data_end - es->data_start);
		ret = remap_pfn_range(vma, addr,
				page_to_pfn(phys_to_page(block->paddr)),
				len, vma->vm_page_prot);
	} else {
		len = PAGE_ALIGN(es->buf_end - es->data_start);
		ret = remap_pfn_range(vma, addr,
				page_to_pfn(phys_to_page(block->paddr)),
				len, vma->vm_page_prot);
		if (ret) {
			pr_error("remap failed %d", ret);
			goto error;
		}
		addr += len;
		if (addr >= vma->vm_end)
			return ret;
		len = PAGE_ALIGN(es->data_end - es->buf_start);
		ret = remap_pfn_range(vma, addr,
				page_to_pfn(phys_to_page(es->buf_start)),
				len, vma->vm_page_prot);
	}
	error:
	return ret;
}

static struct dma_buf_ops dmabuf_manage_ops = {
	.attach = dmabuf_manage_attach,
	.detach = dmabuf_manage_detach,
	.map_dma_buf = dmabuf_manage_map_dma_buf,
	.unmap_dma_buf = dmabuf_manage_unmap_dma_buf,
	.release = dmabuf_manage_buf_release,
	.mmap = dmabuf_manage_mmap
};

static struct dma_buf *get_dmabuf(struct dmabuf_manage_block *block,
					unsigned long flags)
{
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	pr_dbg("export handle:0x%x, paddr:0x%llx, size:0x%x\n",
		block->handle, block->paddr, block->size);
	exp_info.ops = &dmabuf_manage_ops;
	exp_info.size = block->size;
	exp_info.flags = flags;
	exp_info.priv = (void *)&block->dmxes;
	exp_info.exp_name = "dmabuf_manage";

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR_OR_NULL(dbuf))
		return NULL;
	return dbuf;
}

int dma_buf_get_fd(unsigned long args)
{
	int ret = -EINVAL;

	struct dmabuf_manage_block *block;
	struct dma_buf *dbuf;
	int fd = -1;
	int fd_flags = O_CLOEXEC;

	struct dma_buf_info *info;
	info = (struct dma_buf_info *)kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_error("kmalloc  dma_buf_info failed\n");
		return  ret;
	}
	ret = copy_from_user((void *)info, (void __user *)args, sizeof(struct dma_buf_info));
	if (ret != 0) {
		pr_error("copy_from user error\n");
		goto error_copy;
	}

	if (info->version != 1) {
		pr_error("[%s] error, only support version 1 now.\n", __func__);
		kfree(info);
		return -EINVAL;
	}

	pr_enter();
	if (info->paddr != info->dmxes.data_start) {
			pr_error("Invalid buffer info 0x%llx 0x%llx",
			info->paddr, info->dmxes.data_start);
		goto error_copy;
	}
	block = kzalloc(sizeof(*block), GFP_KERNEL);
	if (!block) {
		pr_error("kmalloc failed\n");
		goto error_copy;
	}
	memcpy(&block->dmxes, &info->dmxes, sizeof(block->dmxes));
	block->paddr = info->paddr;
	block->size = PAGE_ALIGN(info->size);
	block->handle = info->handle;
	dbuf = get_dmabuf(block, fd_flags);
	if (!dbuf) {
		pr_error("get_dmabuf failed\n");
		goto error_alloc_object;
    }
	fd = dma_buf_fd(dbuf, fd_flags);
	if (fd < 0) {
		pr_error("dma_buf_fd failed, ret:%d, dbuf:%px, file:%px\n",
				fd, dbuf, dbuf->file);
		goto error_fd;
	}
	pr_dbg("output fd:%d\n", fd);
	kfree(info);
	return fd;
error_fd:
	dma_buf_put(dbuf);
error_alloc_object:
	pr_error("kfree block:%px\n", block);
	kfree(block);
error_copy:
	kfree(info);
	return -EFAULT;
}

long dma_buf_get_released_data_info(unsigned long args) {
	return copy_to_user((void __user *)args, (void *)&last_release_data, sizeof(struct dmx_dma_buf_sec_es_data));
}

static int dumy_open(struct inode* inode, struct file* file) {
	return 0;
}

static ssize_t dumy_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
	return 0;
}


static ssize_t dumy_write(struct file *filep, const char *buffer,
	size_t len, loff_t *offset) {
	return 0;
}

static int dumy_release(struct inode* inode, struct file* file) {
	return 0;
}


static long dma_buf_mgr_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
	long ret = -EINVAL;

	switch (cmd) {
		case DMA_BUF_MANAGE_EXPORT_DMA:
			return dma_buf_get_fd(arg);
		case DMA_BUF_MANAGE_GET_LAST_RELEASED_DATA_INFO:
			return dma_buf_get_released_data_info(arg);
		default:
			break;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long dma_buf_mgr_compat_ioctl(struct file* filp, unsigned int cmd, unsigned long args) {
	unsigned long ret;
	args = (unsigned long)compat_ptr(args);
	ret = dma_buf_mgr_ioctl(filp, cmd, args);
	return ret;
}
#endif
static struct file_operations dma_buf_mgr_ops = {
	.owner = THIS_MODULE,
	.open = dumy_open,
	.read = dumy_read,
	.write = dumy_write,
	.release = dumy_release,
	.unlocked_ioctl = dma_buf_mgr_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dma_buf_mgr_compat_ioctl,
#endif
};


static int  dev_no;
static struct cdev dev;
static struct class* dma_buf_mgr_class;
static struct device* dma_buf_mgr_dev;
#define DEVICE_NAME "dma_buf_mgr_dev"
#define CLASS_NAME  "dma_buf_mgr_cls"

int dma_buf_mgr_module_init(void) {

	int ret;
	ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
	if (ret != 0) {
		pr_err("[%s] failed to create dev\n", __func__);
		return ret;
	}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 3, 13)
	dma_buf_mgr_class = class_create(THIS_MODULE, CLASS_NAME);
#else
	dma_buf_mgr_class = class_create(CLASS_NAME);
#endif

	if (IS_ERR(dma_buf_mgr_class)) {
		pr_err("[%s] failed to create class\n", __func__);
		ret = PTR_ERR(dma_buf_mgr_class);
		goto error_class;
	}

	cdev_init(&dev, &dma_buf_mgr_ops);
	dev.owner = THIS_MODULE;

	ret = cdev_add(&dev, dev_no, 1);

	if (ret != 0) {
		pr_err("[%s] err to add dev\n", __func__);
		goto error_create;
	}

	dma_buf_mgr_dev = device_create(dma_buf_mgr_class, NULL, MKDEV(MAJOR(dev_no), 0), NULL, DEVICE_NAME);
	if (IS_ERR(dma_buf_mgr_dev)) {
		pr_error("[%s] device_create failed\n", __func__);
		ret = PTR_ERR(dma_buf_mgr_dev);
		goto error_dev;
	}

	pr_dbg("[%s] init done\n", __func__);
	return 0;

error_dev:
	cdev_del(&dev);
error_create:
	device_destroy(dma_buf_mgr_class, dev_no);
error_class:
	unregister_chrdev_region(dev_no, 0);

	return ret;

}
EXPORT_SYMBOL(dma_buf_mgr_module_init);

void dma_buf_mgr_module_exit(void) {
	device_destroy(dma_buf_mgr_class, dev_no);
	cdev_del(&dev);
	class_destroy(dma_buf_mgr_class);
	unregister_chrdev_region(dev_no, 0);
}
EXPORT_SYMBOL(dma_buf_mgr_module_exit);