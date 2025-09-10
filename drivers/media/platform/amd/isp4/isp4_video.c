// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#include <drm/amd/isp.h>
#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mc.h>

#include "isp4_interface.h"
#include "isp4_subdev.h"
#include "isp4_video.h"

#define ISP4VID_ISP_DRV_NAME "amd_isp_capture"
#define ISP4VID_MAX_PREVIEW_FPS 30
#define ISP4VID_DEFAULT_FMT isp4vid_formats[0]

#define ISP4VID_PAD_VIDEO_OUTPUT 0

/* timeperframe default */
#define ISP4VID_ISP_TPF_DEFAULT isp4vid_tpfs[0]

/* amdisp buffer for vb2 operations */
struct isp4vid_vb2_buf {
	struct device			*dev;
	void				*vaddr;
	unsigned long			size;
	refcount_t			refcount;
	struct dma_buf			*dbuf;
	void				*bo;
	u64				gpu_addr;
	struct vb2_vmarea_handler	handler;
};

static int isp4vid_vb2_mmap(void *buf_priv, struct vm_area_struct *vma);

static void isp4vid_vb2_put(void *buf_priv);

static const char *isp4vid_video_dev_name = "Preview";

/* Sizes must be in increasing order */
static const struct v4l2_frmsize_discrete isp4vid_frmsize[] = {
	{640, 360},
	{640, 480},
	{1280, 720},
	{1280, 960},
	{1920, 1080},
	{1920, 1440},
	{2560, 1440},
	{2880, 1620},
	{2880, 1624},
	{2888, 1808},
};

static const u32 isp4vid_formats[] = {
	V4L2_PIX_FMT_NV12,
	V4L2_PIX_FMT_YUYV
};

/* timeperframe list */
static const struct v4l2_fract isp4vid_tpfs[] = {
	{.numerator = 1, .denominator = ISP4VID_MAX_PREVIEW_FPS}
};

static void isp4vid_handle_frame_done(struct isp4vid_dev *isp_vdev,
				      const struct isp4if_img_buf_info *img_buf,
				      bool done_suc)
{
	struct isp4vid_capture_buffer *isp4vid_buf;
	enum vb2_buffer_state state;
	void *vbuf;

	mutex_lock(&isp_vdev->buf_list_lock);

	/* Get the first entry of the list */
	isp4vid_buf = list_first_entry_or_null(&isp_vdev->buf_list, typeof(*isp4vid_buf), list);
	if (!isp4vid_buf) {
		mutex_unlock(&isp_vdev->buf_list_lock);
		return;
	}

	vbuf = vb2_plane_vaddr(&isp4vid_buf->vb2.vb2_buf, 0);

	if (vbuf != img_buf->planes[0].sys_addr) {
		dev_err(isp_vdev->dev, "Invalid vbuf");
		mutex_unlock(&isp_vdev->buf_list_lock);
		return;
	}

	/* Remove this entry from the list */
	list_del(&isp4vid_buf->list);

	mutex_unlock(&isp_vdev->buf_list_lock);

	/* Fill the buffer */
	isp4vid_buf->vb2.vb2_buf.timestamp = ktime_get_ns();
	isp4vid_buf->vb2.sequence = isp_vdev->sequence++;
	isp4vid_buf->vb2.field = V4L2_FIELD_ANY;

	/* at most 2 planes */
	vb2_set_plane_payload(&isp4vid_buf->vb2.vb2_buf,
			      0, isp_vdev->format.sizeimage);

	state = done_suc ? VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR;
	vb2_buffer_done(&isp4vid_buf->vb2.vb2_buf, state);

	dev_dbg(isp_vdev->dev, "call vb2_buffer_done(size=%u)\n",
		isp_vdev->format.sizeimage);
}

s32 isp4vid_notify(void *cb_ctx, struct isp4vid_framedone_param *evt_param)
{
	struct isp4vid_dev *isp4vid_vdev = cb_ctx;

	if (evt_param->preview.status != ISP4VID_BUF_DONE_STATUS_ABSENT) {
		bool suc;

		suc = (evt_param->preview.status ==
		       ISP4VID_BUF_DONE_STATUS_SUCCESS);
		isp4vid_handle_frame_done(isp4vid_vdev,
					  &evt_param->preview.buf,
					  suc);
	}

	return 0;
}

static unsigned int isp4vid_vb2_num_users(void *buf_priv)
{
	struct isp4vid_vb2_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static int isp4vid_vb2_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct isp4vid_vb2_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		pr_err("fail no memory to map\n");
		return -EINVAL;
	}

	ret = remap_vmalloc_range(vma, buf->vaddr, 0);
	if (ret) {
		dev_err(buf->dev, "fail remap vmalloc mem, %d\n", ret);
		return ret;
	}

	/*
	 * Make sure that vm_areas for 2 buffers won't be merged together
	 */
	vm_flags_set(vma, VM_DONTEXPAND);

	/*
	 * Use common vm_area operations to track buffer refcount.
	 */
	vma->vm_private_data	= &buf->handler;
	vma->vm_ops		= &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	dev_dbg(buf->dev, "mmap isp user bo 0x%llx size %ld refcount %d\n",
		buf->gpu_addr, buf->size, refcount_read(&buf->refcount));

	return 0;
}

static void *isp4vid_vb2_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct isp4vid_vb2_buf *buf = buf_priv;

	if (!buf->vaddr) {
		dev_err(buf->dev,
			"fail for buf vaddr is null\n");
		return NULL;
	}
	return buf->vaddr;
}

static void isp4vid_vb2_detach_dmabuf(void *mem_priv)
{
	struct isp4vid_vb2_buf *buf = mem_priv;

	dev_dbg(buf->dev, "detach dmabuf of isp user bo 0x%llx size %ld",
		buf->gpu_addr, buf->size);

	kfree(buf);
}

static void *isp4vid_vb2_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
				       struct dma_buf *dbuf,
				       unsigned long size)
{
	struct isp4vid_vb2_buf *buf;

	if (dbuf->size < size) {
		dev_err(dev, "Invalid dmabuf size %ld %ld", dbuf->size, size);
		return ERR_PTR(-EFAULT);
	}

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	struct isp4vid_vb2_buf *dbg_buf = dbuf->priv;

	buf->dev = dev;
	buf->dbuf = dbuf;
	buf->size = size;

	dev_dbg(dev, "attach dmabuf of isp user bo 0x%llx size %ld",
		dbg_buf->gpu_addr, dbg_buf->size);

	return buf;
}

static void isp4vid_vb2_unmap_dmabuf(void *mem_priv)
{
	struct isp4vid_vb2_buf *buf = mem_priv;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(buf->vaddr);

	dev_dbg(buf->dev, "unmap dmabuf of isp user bo 0x%llx size %ld",
		buf->gpu_addr, buf->size);

	dma_buf_vunmap_unlocked(buf->dbuf, &map);
	buf->vaddr = NULL;
}

static int isp4vid_vb2_map_dmabuf(void *mem_priv)
{
	struct isp4vid_vb2_buf *mmap_buf = NULL;
	struct isp4vid_vb2_buf *buf = mem_priv;
	struct iosys_map map = {};
	int ret;

	ret = dma_buf_vmap_unlocked(buf->dbuf, &map);
	if (ret) {
		dev_err(buf->dev, "vmap_unlocked fail");
		return -EFAULT;
	}
	buf->vaddr = map.vaddr;

	mmap_buf = buf->dbuf->priv;
	buf->gpu_addr = mmap_buf->gpu_addr;

	dev_dbg(buf->dev, "map dmabuf of isp user bo 0x%llx size %ld",
		buf->gpu_addr, buf->size);

	return 0;
}

#ifdef CONFIG_HAS_DMA
struct isp4vid_vb2_amdgpu_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static int isp4vid_dmabuf_ops_attach(struct dma_buf *dbuf,
				     struct dma_buf_attachment *dbuf_attach)
{
	struct isp4vid_vb2_buf *buf = dbuf->priv;
	int num_pages = PAGE_ALIGN(buf->size) / PAGE_SIZE;
	struct isp4vid_vb2_amdgpu_attachment *attach;
	void *vaddr = buf->vaddr;
	struct scatterlist *sg;
	struct sg_table *sgt;
	int ret;
	int i;

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach)
		return -ENOMEM;

	sgt = &attach->sgt;
	ret = sg_alloc_table(sgt, num_pages, GFP_KERNEL);
	if (ret) {
		kfree(attach);
		return ret;
	}
	for_each_sgtable_sg(sgt, sg, i) {
		struct page *page = vmalloc_to_page(vaddr);

		if (!page) {
			sg_free_table(sgt);
			kfree(attach);
			return -ENOMEM;
		}
		sg_set_page(sg, page, PAGE_SIZE, 0);
		vaddr = ((char *)vaddr) + PAGE_SIZE;
	}

	attach->dma_dir = DMA_NONE;
	dbuf_attach->priv = attach;

	return 0;
}

static void isp4vid_dmabuf_ops_detach(struct dma_buf *dbuf,
				      struct dma_buf_attachment *dbuf_attach)
{
	struct isp4vid_vb2_amdgpu_attachment *attach = dbuf_attach->priv;
	struct sg_table *sgt;

	if (!attach) {
		pr_err("fail invalid attach handler\n");
		return;
	}

	sgt = &attach->sgt;

	/* release the scatterlist cache */
	if (attach->dma_dir != DMA_NONE)
		dma_unmap_sgtable(dbuf_attach->dev, sgt, attach->dma_dir, 0);
	sg_free_table(sgt);
	kfree(attach);
	dbuf_attach->priv = NULL;
}

static struct sg_table *isp4vid_dmabuf_ops_map(struct dma_buf_attachment *dbuf_attach,
					       enum dma_data_direction dma_dir)
{
	struct isp4vid_vb2_amdgpu_attachment *attach = dbuf_attach->priv;
	struct sg_table *sgt;

	sgt = &attach->sgt;
	/* return previously mapped sg table */
	if (attach->dma_dir == dma_dir)
		return sgt;

	/* release any previous cache */
	if (attach->dma_dir != DMA_NONE) {
		dma_unmap_sgtable(dbuf_attach->dev, sgt, attach->dma_dir, 0);
		attach->dma_dir = DMA_NONE;
	}

	/* mapping to the client with new direction */
	if (dma_map_sgtable(dbuf_attach->dev, sgt, dma_dir, 0)) {
		dev_err(dbuf_attach->dev, "fail to map scatterlist");
		return ERR_PTR(-EIO);
	}

	attach->dma_dir = dma_dir;

	return sgt;
}

static void isp4vid_dmabuf_ops_unmap(struct dma_buf_attachment *dbuf_attach,
				     struct sg_table *sgt,
				     enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

static int isp4vid_dmabuf_ops_vmap(struct dma_buf *dbuf,
				   struct iosys_map *map)
{
	struct isp4vid_vb2_buf *buf = dbuf->priv;

	iosys_map_set_vaddr(map, buf->vaddr);

	return 0;
}

static void isp4vid_dmabuf_ops_release(struct dma_buf *dbuf)
{
	struct isp4vid_vb2_buf *buf = dbuf->priv;

	/* drop reference obtained in isp4vid_vb2_get_dmabuf */
	if (dbuf != buf->dbuf)
		isp4vid_vb2_put(buf);
	else
		kfree(buf);
}

static int isp4vid_dmabuf_ops_mmap(struct dma_buf *dbuf, struct vm_area_struct *vma)
{
	return isp4vid_vb2_mmap(dbuf->priv, vma);
}

static const struct dma_buf_ops isp4vid_dmabuf_ops = {
	.attach = isp4vid_dmabuf_ops_attach,
	.detach = isp4vid_dmabuf_ops_detach,
	.map_dma_buf = isp4vid_dmabuf_ops_map,
	.unmap_dma_buf = isp4vid_dmabuf_ops_unmap,
	.vmap = isp4vid_dmabuf_ops_vmap,
	.mmap = isp4vid_dmabuf_ops_mmap,
	.release = isp4vid_dmabuf_ops_release,
};

static struct dma_buf *isp4vid_get_dmabuf(struct isp4vid_vb2_buf *buf, unsigned long flags)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dbuf;

	if (WARN_ON(!buf->vaddr))
		return NULL;

	exp_info.ops = &isp4vid_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	return dbuf;
}

static struct dma_buf *isp4vid_vb2_get_dmabuf(struct vb2_buffer *vb, void *buf_priv,
					      unsigned long flags)
{
	struct isp4vid_vb2_buf *buf = buf_priv;
	struct dma_buf *dbuf;

	dbuf = isp4vid_get_dmabuf(buf, flags);
	if (!dbuf) {
		dev_err(buf->dev, "fail to get isp dma buffer\n");
		return NULL;
	}

	refcount_inc(&buf->refcount);

	dev_dbg(buf->dev, "buf exported, refcount %d\n",
		refcount_read(&buf->refcount));

	return dbuf;
}
#endif /* CONFIG_HAS_DMA */

static void isp4vid_vb2_put(void *buf_priv)
{
	struct isp4vid_vb2_buf *buf = buf_priv;

	dev_dbg(buf->dev,
		"release isp user bo 0x%llx size %ld refcount %d",
		buf->gpu_addr, buf->size,
		refcount_read(&buf->refcount));

	if (refcount_dec_and_test(&buf->refcount)) {
		isp_user_buffer_free(buf->bo);
		vfree(buf->vaddr);
		/*
		 * Putting the implicit dmabuf frees `buf`. Freeing `buf` must
		 * be done from the dmabuf release callback since dma_buf_put()
		 * isn't always synchronous; it's just an fput(), which may be
		 * deferred. Since the dmabuf release callback needs to access
		 * `buf`, this means `buf` cannot be freed until then.
		 */
		dma_buf_put(buf->dbuf);
	}
}

static void *isp4vid_vb2_alloc(struct vb2_buffer *vb, struct device *dev, unsigned long size)
{
	struct isp4vid_vb2_buf *buf;
	u64 gpu_addr;
	void *bo;
	int ret;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL | vb->vb2_queue->gfp_flags);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->size = size;
	buf->vaddr = vmalloc_user(buf->size);
	if (!buf->vaddr) {
		dev_err(dev, "fail to vmalloc buffer\n");
		goto free_buf;
	}

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = isp4vid_vb2_put;
	buf->handler.arg = buf;

	/* get implicit dmabuf */
	buf->dbuf = isp4vid_get_dmabuf(buf, 0);
	if (!buf->dbuf) {
		dev_err(dev, "fail to get implicit dmabuf\n");
		goto free_user_vmem;
	}

	/* create isp user BO and obtain gpu_addr */
	ret = isp_user_buffer_alloc(dev, buf->dbuf, &bo, &gpu_addr);
	if (ret) {
		dev_err(dev, "fail to create isp user BO\n");
		goto put_dmabuf;
	}

	buf->bo = bo;
	buf->gpu_addr = gpu_addr;

	refcount_set(&buf->refcount, 1);

	dev_dbg(dev, "allocated isp user bo 0x%llx size %ld refcount %d\n",
		buf->gpu_addr, buf->size, refcount_read(&buf->refcount));

	return buf;

put_dmabuf:
	dma_buf_put(buf->dbuf);
free_user_vmem:
	vfree(buf->vaddr);
free_buf:
	ret = buf->vaddr ? -EINVAL : -ENOMEM;
	kfree(buf);
	return ERR_PTR(ret);
}

static const struct vb2_mem_ops isp4vid_vb2_memops = {
	.alloc		= isp4vid_vb2_alloc,
	.put		= isp4vid_vb2_put,
#ifdef CONFIG_HAS_DMA
	.get_dmabuf	= isp4vid_vb2_get_dmabuf,
#endif
	.map_dmabuf	= isp4vid_vb2_map_dmabuf,
	.unmap_dmabuf	= isp4vid_vb2_unmap_dmabuf,
	.attach_dmabuf	= isp4vid_vb2_attach_dmabuf,
	.detach_dmabuf	= isp4vid_vb2_detach_dmabuf,
	.vaddr		= isp4vid_vb2_vaddr,
	.mmap		= isp4vid_vb2_mmap,
	.num_users	= isp4vid_vb2_num_users,
};

static const struct v4l2_pix_format isp4vid_fmt_default = {
	.width = 1920,
	.height = 1080,
	.pixelformat = V4L2_PIX_FMT_NV12,
	.field = V4L2_FIELD_NONE,
	.colorspace = V4L2_COLORSPACE_SRGB,
};

static void isp4vid_capture_return_all_buffers(struct isp4vid_dev *isp_vdev,
					       enum vb2_buffer_state state)
{
	struct isp4vid_capture_buffer *vbuf, *node;

	mutex_lock(&isp_vdev->buf_list_lock);

	list_for_each_entry_safe(vbuf, node, &isp_vdev->buf_list, list) {
		list_del(&vbuf->list);
		vb2_buffer_done(&vbuf->vb2.vb2_buf, state);
	}
	mutex_unlock(&isp_vdev->buf_list_lock);

	dev_dbg(isp_vdev->dev, "call vb2_buffer_done(%d)\n", state);
}

static int isp4vid_vdev_link_validate(struct media_link *link)
{
	return 0;
}

static const struct media_entity_operations isp4vid_vdev_ent_ops = {
	.link_validate = isp4vid_vdev_link_validate,
};

static const struct v4l2_file_operations isp4vid_vdev_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.read = vb2_fop_read,
	.poll = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
};

static int isp4vid_ioctl_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);

	strscpy(cap->driver, ISP4VID_ISP_DRV_NAME, sizeof(cap->driver));
	snprintf(cap->card, sizeof(cap->card), "%s", ISP4VID_ISP_DRV_NAME);
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", ISP4VID_ISP_DRV_NAME);

	cap->capabilities |= (V4L2_CAP_STREAMING |
			      V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_IO_MC);

	dev_dbg(isp_vdev->dev, "%s|capabilities=0x%X",
		isp_vdev->vdev.name, cap->capabilities);

	return 0;
}

static int isp4vid_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);

	f->fmt.pix = isp_vdev->format;

	return 0;
}

static int isp4vid_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);
	struct v4l2_pix_format *format = &f->fmt.pix;
	unsigned int i;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	/*
	 * Check if the hardware supports the requested format, use the default
	 * format otherwise.
	 */
	for (i = 0; i < ARRAY_SIZE(isp4vid_formats); i++)
		if (isp4vid_formats[i] == format->pixelformat)
			break;

	if (i == ARRAY_SIZE(isp4vid_formats))
		format->pixelformat = ISP4VID_DEFAULT_FMT;

	switch (format->pixelformat) {
	case V4L2_PIX_FMT_NV12: {
		const struct v4l2_frmsize_discrete *fsz =
			v4l2_find_nearest_size(isp4vid_frmsize,
					       ARRAY_SIZE(isp4vid_frmsize),
					       width, height,
					       format->width, format->height);

		format->width = fsz->width;
		format->height = fsz->height;

		format->bytesperline = format->width;
		format->sizeimage = format->bytesperline *
				    format->height * 3 / 2;
	}
	break;
	case V4L2_PIX_FMT_YUYV: {
		const struct v4l2_frmsize_discrete *fsz =
			v4l2_find_nearest_size(isp4vid_frmsize,
					       ARRAY_SIZE(isp4vid_frmsize),
					       width, height,
					       format->width, format->height);

		format->width = fsz->width;
		format->height = fsz->height;

		format->bytesperline = format->width * 2;
		format->sizeimage = format->bytesperline * format->height;
	}
	break;
	default:
		dev_err(isp_vdev->dev, "%s|unsupported fmt=%u",
			isp_vdev->vdev.name, format->pixelformat);
		return -EINVAL;
	}

	if (format->field == V4L2_FIELD_ANY)
		format->field = isp4vid_fmt_default.field;

	if (format->colorspace == V4L2_COLORSPACE_DEFAULT)
		format->colorspace = isp4vid_fmt_default.colorspace;

	return 0;
}

static int isp4vid_set_fmt_2_isp(struct v4l2_subdev *sdev, struct v4l2_pix_format *pix_fmt)
{
	struct v4l2_subdev_format fmt = {};

	switch (pix_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12:
		fmt.format.code = MEDIA_BUS_FMT_YUYV8_1_5X8;
		break;
	case V4L2_PIX_FMT_YUYV:
		fmt.format.code = MEDIA_BUS_FMT_YUYV8_1X16;
		break;
	default:
		return -EINVAL;
	};
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.pad = ISP4VID_PAD_VIDEO_OUTPUT;
	fmt.format.width = pix_fmt->width;
	fmt.format.height = pix_fmt->height;
	return v4l2_subdev_call(sdev, pad, set_fmt, NULL, &fmt);
}

static int isp4vid_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);
	int ret;

	/* Do not change the format while stream is on */
	if (vb2_is_busy(&isp_vdev->vbq))
		return -EBUSY;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	ret = isp4vid_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	dev_dbg(isp_vdev->dev, "%s|width height:%ux%u->%ux%u",
		isp_vdev->vdev.name,
		isp_vdev->format.width, isp_vdev->format.height,
		f->fmt.pix.width, f->fmt.pix.height);
	dev_dbg(isp_vdev->dev, "%s|pixelformat:0x%x-0x%x",
		isp_vdev->vdev.name, isp_vdev->format.pixelformat,
		f->fmt.pix.pixelformat);
	dev_dbg(isp_vdev->dev, "%s|bytesperline:%u->%u",
		isp_vdev->vdev.name, isp_vdev->format.bytesperline,
		f->fmt.pix.bytesperline);
	dev_dbg(isp_vdev->dev, "%s|sizeimage:%u->%u",
		isp_vdev->vdev.name, isp_vdev->format.sizeimage,
		f->fmt.pix.sizeimage);

	isp_vdev->format = f->fmt.pix;
	ret = isp4vid_set_fmt_2_isp(isp_vdev->isp_sdev, &isp_vdev->format);

	return ret;
}

static int isp4vid_enum_fmt_vid_cap(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);

	switch (f->index) {
	case 0:
		f->pixelformat = V4L2_PIX_FMT_NV12;
		break;
	case 1:
		f->pixelformat = V4L2_PIX_FMT_YUYV;
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(isp_vdev->dev, "%s|index=%d, pixelformat=0x%X",
		isp_vdev->vdev.name, f->index, f->pixelformat);

	return 0;
}

static int isp4vid_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(isp4vid_formats); i++) {
		if (isp4vid_formats[i] == fsize->pixel_format)
			break;
	}
	if (i == ARRAY_SIZE(isp4vid_formats))
		return -EINVAL;

	if (fsize->index < ARRAY_SIZE(isp4vid_frmsize)) {
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete = isp4vid_frmsize[fsize->index];
		dev_dbg(isp_vdev->dev, "%s|size[%d]=%dx%d",
			isp_vdev->vdev.name, fsize->index,
			fsize->discrete.width, fsize->discrete.height);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int isp4vid_ioctl_enum_frameintervals(struct file *file, void *priv,
					     struct v4l2_frmivalenum *fival)
{
	struct isp4vid_dev *isp_vdev = video_drvdata(file);
	int i;

	if (fival->index >= ARRAY_SIZE(isp4vid_tpfs))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(isp4vid_formats); i++)
		if (isp4vid_formats[i] == fival->pixel_format)
			break;
	if (i == ARRAY_SIZE(isp4vid_formats))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(isp4vid_frmsize); i++)
		if (isp4vid_frmsize[i].width == fival->width &&
		    isp4vid_frmsize[i].height == fival->height)
			break;
	if (i == ARRAY_SIZE(isp4vid_frmsize))
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete = isp4vid_tpfs[fival->index];
	v4l2_simplify_fraction(&fival->discrete.numerator,
			       &fival->discrete.denominator, 8, 333);

	dev_dbg(isp_vdev->dev, "%s|interval[%d]=%d/%d",
		isp_vdev->vdev.name, fival->index,
		fival->discrete.numerator,
		fival->discrete.denominator);

	return 0;
}

static int isp4vid_ioctl_g_param(struct file *file, void *priv, struct v4l2_streamparm *param)
{
	struct v4l2_captureparm *capture = &param->parm.capture;
	struct isp4vid_dev *isp_vdev = video_drvdata(file);

	if (param->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	capture->capability   = V4L2_CAP_TIMEPERFRAME;
	capture->timeperframe = isp_vdev->timeperframe;
	capture->readbuffers  = 0;

	dev_dbg(isp_vdev->dev, "%s|timeperframe=%d/%d", isp_vdev->vdev.name,
		capture->timeperframe.numerator,
		capture->timeperframe.denominator);

	return 0;
}

static const struct v4l2_ioctl_ops isp4vid_vdev_ioctl_ops = {
	/* VIDIOC_QUERYCAP handler */
	.vidioc_querycap = isp4vid_ioctl_querycap,

	/* VIDIOC_ENUM_FMT handlers */
	.vidioc_enum_fmt_vid_cap = isp4vid_enum_fmt_vid_cap,

	/* VIDIOC_G_FMT handlers */
	.vidioc_g_fmt_vid_cap = isp4vid_g_fmt_vid_cap,

	/* VIDIOC_S_FMT handlers */
	.vidioc_s_fmt_vid_cap = isp4vid_s_fmt_vid_cap,

	/* VIDIOC_TRY_FMT handlers */
	.vidioc_try_fmt_vid_cap = isp4vid_try_fmt_vid_cap,

	/* Buffer handlers */
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,

	/* Stream on/off */
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,

	/* Stream type-dependent parameter ioctls */
	.vidioc_g_parm        = isp4vid_ioctl_g_param,
	.vidioc_s_parm        = isp4vid_ioctl_g_param,

	.vidioc_enum_framesizes = isp4vid_enum_framesizes,
	.vidioc_enum_frameintervals = isp4vid_ioctl_enum_frameintervals,

};

static unsigned int isp4vid_get_image_size(struct v4l2_pix_format *fmt)
{
	switch (fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12:
		return fmt->width * fmt->height * 3 / 2;
	case V4L2_PIX_FMT_YUYV:
		return fmt->width * fmt->height * 2;
	default:
		return 0;
	}
};

static int isp4vid_qops_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				    unsigned int *nplanes, unsigned int sizes[],
				    struct device *alloc_devs[])
{
	struct isp4vid_dev *isp_vdev = vb2_get_drv_priv(vq);
	unsigned int q_num_bufs = vb2_get_num_buffers(vq);

	if (*nplanes > 1) {
		dev_err(isp_vdev->dev,
			"fail to setup queue, no mplane supported %u\n",
			*nplanes);
		return -EINVAL;
	};

	if (*nplanes == 1) {
		unsigned int size;

		size = isp4vid_get_image_size(&isp_vdev->format);
		if (sizes[0] < size) {
			dev_err(isp_vdev->dev,
				"fail for small plane size %u, %u expected\n",
				sizes[0], size);
			return -EINVAL;
		}
	}

	if (q_num_bufs + *nbuffers < ISP4IF_MAX_STREAM_BUF_COUNT)
		*nbuffers = ISP4IF_MAX_STREAM_BUF_COUNT - q_num_bufs;

	switch (isp_vdev->format.pixelformat) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_YUYV: {
		*nplanes = 1;
		sizes[0] = max(sizes[0], isp_vdev->format.sizeimage);
		isp_vdev->format.sizeimage = sizes[0];
	}
	break;
	default:
		dev_err(isp_vdev->dev, "%s|unsupported fmt=%u\n",
			isp_vdev->vdev.name, isp_vdev->format.pixelformat);
		return -EINVAL;
	}

	dev_dbg(isp_vdev->dev, "%s|*nbuffers=%u *nplanes=%u sizes[0]=%u\n",
		isp_vdev->vdev.name,
		*nbuffers, *nplanes, sizes[0]);

	return 0;
}

static void isp4vid_qops_buffer_queue(struct vb2_buffer *vb)
{
	struct isp4vid_capture_buffer *buf =
		container_of(vb, struct isp4vid_capture_buffer, vb2.vb2_buf);
	struct isp4vid_dev *isp_vdev = vb2_get_drv_priv(vb->vb2_queue);

	struct isp4vid_vb2_buf *priv_buf = vb->planes[0].mem_priv;
	struct isp4if_img_buf_info *img_buf = &buf->img_buf;

	dev_dbg(isp_vdev->dev, "%s|index=%u", isp_vdev->vdev.name, vb->index);

	dev_dbg(isp_vdev->dev, "queue isp user bo 0x%llx size=%lu",
		priv_buf->gpu_addr,
		priv_buf->size);

	switch (isp_vdev->format.pixelformat) {
	case V4L2_PIX_FMT_NV12: {
		u32 y_size = isp_vdev->format.sizeimage / 3 * 2;
		u32 uv_size = isp_vdev->format.sizeimage / 3;

		img_buf->planes[0].len = y_size;
		img_buf->planes[0].sys_addr = priv_buf->vaddr;
		img_buf->planes[0].mc_addr = priv_buf->gpu_addr;

		dev_dbg(isp_vdev->dev, "img_buf[0]: mc=0x%llx size=%u",
			img_buf->planes[0].mc_addr,
			img_buf->planes[0].len);

		img_buf->planes[1].len = uv_size;
		img_buf->planes[1].sys_addr =
			(void *)((u64)priv_buf->vaddr + y_size);
		img_buf->planes[1].mc_addr = priv_buf->gpu_addr + y_size;

		dev_dbg(isp_vdev->dev, "img_buf[1]: mc=0x%llx size=%u",
			img_buf->planes[1].mc_addr,
			img_buf->planes[1].len);

		img_buf->planes[2].len = 0;
	}
	break;
	case V4L2_PIX_FMT_YUYV: {
		img_buf->planes[0].len = isp_vdev->format.sizeimage;
		img_buf->planes[0].sys_addr = priv_buf->vaddr;
		img_buf->planes[0].mc_addr = priv_buf->gpu_addr;

		dev_dbg(isp_vdev->dev, "img_buf[0]: mc=0x%llx size=%u",
			img_buf->planes[0].mc_addr,
			img_buf->planes[0].len);

		img_buf->planes[1].len = 0;
		img_buf->planes[2].len = 0;
	}
	break;
	default:
		dev_err(isp_vdev->dev, "%s|unsupported fmt=%u",
			isp_vdev->vdev.name, isp_vdev->format.pixelformat);
		return;
	}

	if (isp_vdev->stream_started)
		isp_vdev->ops->send_buffer(isp_vdev->isp_sdev, img_buf);

	mutex_lock(&isp_vdev->buf_list_lock);
	list_add_tail(&buf->list, &isp_vdev->buf_list);
	mutex_unlock(&isp_vdev->buf_list_lock);
}

static int isp4vid_qops_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct isp4vid_dev *isp_vdev = vb2_get_drv_priv(vq);
	struct isp4vid_capture_buffer *isp_buf;
	struct media_entity *entity;
	struct v4l2_subdev *subdev;
	struct media_pad *pad;
	int ret = 0;

	isp_vdev->sequence = 0;
	ret = v4l2_pipeline_pm_get(&isp_vdev->vdev.entity);
	if (ret) {
		dev_err(isp_vdev->dev, "power up isp fail %d\n", ret);
		goto release_buffers;
	}

	entity = &isp_vdev->vdev.entity;
	while (1) {
		pad = &entity->pads[0];
		if (!(pad->flags & MEDIA_PAD_FL_SINK))
			break;

		pad = media_pad_remote_pad_first(pad);
		if (!pad || !is_media_entity_v4l2_subdev(pad->entity))
			break;

		entity = pad->entity;
		subdev = media_entity_to_v4l2_subdev(entity);

		ret = v4l2_subdev_call(subdev, video, s_stream, 1);
		if (ret < 0 && ret != -ENOIOCTLCMD) {
			dev_dbg(isp_vdev->dev, "fail start streaming: %s %d\n",
				subdev->name, ret);
			goto release_buffers;
		}
	}

	list_for_each_entry(isp_buf, &isp_vdev->buf_list, list) {
		isp_vdev->ops->send_buffer(isp_vdev->isp_sdev,
					   &isp_buf->img_buf);
	}

	/* Start the media pipeline */
	ret = video_device_pipeline_start(&isp_vdev->vdev, &isp_vdev->pipe);
	if (ret) {
		dev_err(isp_vdev->dev, "video_device_pipeline_start fail:%d",
			ret);
		goto release_buffers;
	}
	isp_vdev->stream_started = true;

	return 0;

release_buffers:
	isp4vid_capture_return_all_buffers(isp_vdev, VB2_BUF_STATE_QUEUED);
	return ret;
}

static void isp4vid_qops_stop_streaming(struct vb2_queue *vq)
{
	struct isp4vid_dev *isp_vdev = vb2_get_drv_priv(vq);
	struct v4l2_subdev *subdev;
	struct media_entity *entity;
	struct media_pad *pad;
	int ret;

	entity = &isp_vdev->vdev.entity;
	while (1) {
		pad = &entity->pads[0];
		if (!(pad->flags & MEDIA_PAD_FL_SINK))
			break;

		pad = media_pad_remote_pad_first(pad);
		if (!pad || !is_media_entity_v4l2_subdev(pad->entity))
			break;

		entity = pad->entity;
		subdev = media_entity_to_v4l2_subdev(entity);

		ret = v4l2_subdev_call(subdev, video, s_stream, 0);

		if (ret < 0 && ret != -ENOIOCTLCMD)
			dev_dbg(isp_vdev->dev, "fail stop streaming: %s %d\n",
				subdev->name, ret);
	}

	isp_vdev->stream_started = false;
	v4l2_pipeline_pm_put(&isp_vdev->vdev.entity);

	/* Stop the media pipeline */
	video_device_pipeline_stop(&isp_vdev->vdev);

	/* Release all active buffers */
	isp4vid_capture_return_all_buffers(isp_vdev, VB2_BUF_STATE_ERROR);
}

static int isp4vid_fill_buffer_size(struct isp4vid_dev *isp_vdev)
{
	int ret = 0;

	switch (isp_vdev->format.pixelformat) {
	case V4L2_PIX_FMT_NV12:
		isp_vdev->format.bytesperline = isp_vdev->format.width;
		isp_vdev->format.sizeimage = isp_vdev->format.bytesperline *
					     isp_vdev->format.height * 3 / 2;
		break;
	case V4L2_PIX_FMT_YUYV:
		isp_vdev->format.bytesperline = isp_vdev->format.width;
		isp_vdev->format.sizeimage = isp_vdev->format.bytesperline *
					     isp_vdev->format.height * 2;
		break;
	default:
		dev_err(isp_vdev->dev, "fail for invalid default format 0x%x",
			isp_vdev->format.pixelformat);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct vb2_ops isp4vid_qops = {
	.queue_setup = isp4vid_qops_queue_setup,
	.buf_queue = isp4vid_qops_buffer_queue,
	.start_streaming = isp4vid_qops_start_streaming,
	.stop_streaming = isp4vid_qops_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

int isp4vid_dev_init(struct isp4vid_dev *isp_vdev, struct v4l2_subdev *isp_sdev,
		     const struct isp4vid_ops *ops)
{
	const char *vdev_name = isp4vid_video_dev_name;
	struct v4l2_device *v4l2_dev;
	struct video_device *vdev;
	struct vb2_queue *q;
	int ret;

	if (!isp_vdev || !isp_sdev || !isp_sdev->v4l2_dev)
		return -EINVAL;

	v4l2_dev = isp_sdev->v4l2_dev;
	vdev = &isp_vdev->vdev;

	isp_vdev->isp_sdev = isp_sdev;
	isp_vdev->dev = v4l2_dev->dev;
	isp_vdev->ops = ops;

	/* Initialize the vb2_queue struct */
	mutex_init(&isp_vdev->vbq_lock);
	q = &isp_vdev->vbq;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->buf_struct_size = sizeof(struct isp4vid_capture_buffer);
	q->min_queued_buffers = 2;
	q->ops = &isp4vid_qops;
	q->drv_priv = isp_vdev;
	q->mem_ops = &isp4vid_vb2_memops;
	q->lock = &isp_vdev->vbq_lock;
	q->dev = v4l2_dev->dev;
	ret = vb2_queue_init(q);
	if (ret) {
		dev_err(v4l2_dev->dev, "vb2_queue_init error:%d", ret);
		return ret;
	}
	/* Initialize buffer list and its lock */
	mutex_init(&isp_vdev->buf_list_lock);
	INIT_LIST_HEAD(&isp_vdev->buf_list);

	/* Set default frame format */
	isp_vdev->format = isp4vid_fmt_default;
	isp_vdev->timeperframe = ISP4VID_ISP_TPF_DEFAULT;
	v4l2_simplify_fraction(&isp_vdev->timeperframe.numerator,
			       &isp_vdev->timeperframe.denominator, 8, 333);

	ret = isp4vid_fill_buffer_size(isp_vdev);
	if (ret) {
		dev_err(v4l2_dev->dev, "fail to fill buffer size: %d\n", ret);
		return ret;
	}

	ret = isp4vid_set_fmt_2_isp(isp_sdev, &isp_vdev->format);
	if (ret) {
		dev_err(v4l2_dev->dev, "fail init format :%d\n", ret);
		return ret;
	}

	/* Initialize the video_device struct */
	isp_vdev->vdev.entity.name = vdev_name;
	isp_vdev->vdev.entity.function = MEDIA_ENT_F_IO_V4L;
	isp_vdev->vdev_pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&isp_vdev->vdev.entity, 1,
				     &isp_vdev->vdev_pad);

	if (ret) {
		dev_err(v4l2_dev->dev, "init media entity pad fail:%d\n", ret);
		return ret;
	}

	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE |
			    V4L2_CAP_STREAMING | V4L2_CAP_IO_MC;
	vdev->entity.ops = &isp4vid_vdev_ent_ops;
	vdev->release = video_device_release_empty;
	vdev->fops = &isp4vid_vdev_fops;
	vdev->ioctl_ops = &isp4vid_vdev_ioctl_ops;
	vdev->lock = NULL;
	vdev->queue = q;
	vdev->v4l2_dev = v4l2_dev;
	vdev->vfl_dir = VFL_DIR_RX;
	strscpy(vdev->name, vdev_name, sizeof(vdev->name));
	video_set_drvdata(vdev, isp_vdev);

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		dev_err(v4l2_dev->dev, "register video device fail:%d\n", ret);

	return ret;
}

void isp4vid_dev_deinit(struct isp4vid_dev *isp_vdev)
{
	video_unregister_device(&isp_vdev->vdev);
}
