/*
 * Copyright 2019 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * based on nouveau_prime.c
 *
 * Authors: Alex Deucher
 */

/**
 * DOC: PRIME Buffer Sharing
 *
 * The following callback implementations are used for :ref:`sharing GEM buffer
 * objects between different devices via PRIME <prime_buffer_sharing>`.
 */

#include "amdgpu.h"
#include "amdgpu_display.h"
#include "amdgpu_gem.h"
#include "amdgpu_dma_buf.h"
#include <drm/amdgpu_drm.h>
#include <linux/dma-buf.h>
#include <linux/dma-fence-array.h>

#if !defined(HAVE_DMA_BUF_OPS_DYNAMIC_MAPPING)
/**
 * amdgpu_gem_prime_get_sg_table - &drm_driver.gem_prime_get_sg_table
 * implementation
 * @obj: GEM buffer object (BO)
 *
 * Returns:
 * A scatter/gather table for the pinned pages of the BO's memory.
 */
struct sg_table *amdgpu_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	int npages = bo->tbo.num_pages;

	return drm_prime_pages_to_sg(bo->tbo.ttm->pages, npages);
}
#endif

/**
 * amdgpu_gem_prime_vmap - &dma_buf_ops.vmap implementation
 * @obj: GEM BO
 *
 * Sets up an in-kernel virtual mapping of the BO's memory.
 *
 * Returns:
 * The virtual address of the mapping or an error pointer.
 */
void *amdgpu_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	int ret;

	ret = ttm_bo_kmap(&bo->tbo, 0, bo->tbo.num_pages,
			  &bo->dma_buf_vmap);
	if (ret)
		return ERR_PTR(ret);

	return bo->dma_buf_vmap.virtual;
}

/**
 * amdgpu_gem_prime_vunmap - &dma_buf_ops.vunmap implementation
 * @obj: GEM BO
 * @vaddr: Virtual address (unused)
 *
 * Tears down the in-kernel virtual mapping of the BO's memory.
 */
void amdgpu_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);

	ttm_bo_kunmap(&bo->dma_buf_vmap);
}

/**
 * amdgpu_gem_prime_mmap - &drm_driver.gem_prime_mmap implementation
 * @obj: GEM BO
 * @vma: Virtual memory area
 *
 * Sets up a userspace mapping of the BO's memory in the given
 * virtual memory area.
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
int amdgpu_gem_prime_mmap(struct drm_gem_object *obj,
			  struct vm_area_struct *vma)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	unsigned asize = amdgpu_bo_size(bo);
	int ret;

	if (!vma->vm_file)
		return -ENODEV;

	if (adev == NULL)
		return -ENODEV;

	/* Check for valid size. */
	if (asize < vma->vm_end - vma->vm_start)
		return -EINVAL;

	if (amdgpu_ttm_tt_get_usermm(bo->tbo.ttm) ||
	    (bo->flags & AMDGPU_GEM_CREATE_NO_CPU_ACCESS)) {
		return -EPERM;
	}
	vma->vm_pgoff += amdgpu_bo_mmap_offset(bo) >> PAGE_SHIFT;

	/* prime mmap does not need to check access, so allow here */
	ret = drm_vma_node_allow(&obj->vma_node, vma->vm_file->private_data);
	if (ret)
		return ret;

	ret = ttm_bo_mmap(vma->vm_file, vma, &adev->mman.bdev);
	drm_vma_node_revoke(&obj->vma_node, vma->vm_file->private_data);

	return ret;
}

#if defined(AMDKCL_AMDGPU_DMABUF_OPS)
static int
__dma_resv_make_exclusive(struct dma_resv *obj)
{
	struct dma_fence **fences;
	unsigned int count;
	int r;

	if (!dma_resv_get_list(obj)) /* no shared fences to convert */
		return 0;

	r = dma_resv_get_fences_rcu(obj, NULL, &count, &fences);
	if (r)
		return r;

	if (count == 0) {
		/* Now that was unexpected. */
	} else if (count == 1) {
		dma_resv_add_excl_fence(obj, fences[0]);
		dma_fence_put(fences[0]);
		kfree(fences);
	} else {
		struct dma_fence_array *array;

		array = dma_fence_array_create(count, fences,
					       dma_fence_context_alloc(1), 0,
					       false);
		if (!array)
			goto err_fences_put;

		dma_resv_add_excl_fence(obj, &array->base);
		dma_fence_put(&array->base);
	}

	return 0;

err_fences_put:
	while (count--)
		dma_fence_put(fences[count]);
	kfree(fences);
	return -ENOMEM;
}

#if !defined(HAVE_DMA_BUF_OPS_DYNAMIC_MAPPING)
/**
 * amdgpu_dma_buf_map_attach - &dma_buf_ops.attach implementation
 * @dma_buf: Shared DMA buffer
 * @attach: DMA-buf attachment
 *
 * Makes sure that the shared DMA buffer can be accessed by the target device.
 * For now, simply pins it to the GTT domain, where it should be accessible by
 * all DMA devices.
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
static int amdgpu_dma_buf_map_attach(struct dma_buf *dma_buf,
#ifndef HAVE_DRM_GEM_MAP_ATTACH_2ARGS
					struct device *target_dev,
#endif
					struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	long r;

#ifdef HAVE_DRM_GEM_MAP_ATTACH_2ARGS
	r = drm_gem_map_attach(dma_buf, attach);
#else
	r = drm_gem_map_attach(dma_buf, target_dev, attach);
#endif
	if (r)
		return r;

	r = amdgpu_bo_reserve(bo, false);
	if (unlikely(r != 0))
		goto error_detach;


	if (attach->dev->driver != adev->dev->driver) {
		/*
		 * We only create shared fences for internal use, but importers
		 * of the dmabuf rely on exclusive fences for implicitly
		 * tracking write hazards. As any of the current fences may
		 * correspond to a write, we need to convert all existing
		 * fences on the reservation object into a single exclusive
		 * fence.
		 */
		r = __dma_resv_make_exclusive(amdkcl_ttm_resvp(&bo->tbo));
		if (r)
			goto error_unreserve;
	}

	/* pin buffer into GTT */
	r = amdgpu_bo_pin(bo, AMDGPU_GEM_DOMAIN_GTT);
	if (r)
		goto error_unreserve;

	if (attach->dev->driver != adev->dev->driver)
		bo->prime_shared_count++;

error_unreserve:
	amdgpu_bo_unreserve(bo);

error_detach:
	if (r)
		drm_gem_map_detach(dma_buf, attach);
	return r;
}

/**
 * amdgpu_dma_buf_map_detach - &dma_buf_ops.detach implementation
 * @dma_buf: Shared DMA buffer
 * @attach: DMA-buf attachment
 *
 * This is called when a shared DMA buffer no longer needs to be accessible by
 * another device. For now, simply unpins the buffer from GTT.
 */
static void amdgpu_dma_buf_map_detach(struct dma_buf *dma_buf,
				      struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	int ret = 0;

	ret = amdgpu_bo_reserve(bo, true);
	if (unlikely(ret != 0))
		goto error;

	amdgpu_bo_unpin(bo);
	if (attach->dev->driver != adev->dev->driver && bo->prime_shared_count)
		bo->prime_shared_count--;
	amdgpu_bo_unreserve(bo);

error:
	drm_gem_map_detach(dma_buf, attach);
}
#else
/**
 * amdgpu_dma_buf_attach - &dma_buf_ops.attach implementation
 *
 * @dmabuf: DMA-buf where we attach to
 * @attach: attachment to add
 *
 * Add the attachment as user to the exported DMA-buf.
 */
static int amdgpu_dma_buf_attach(struct dma_buf *dmabuf,
				 struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dmabuf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	int r;

	if (attach->dev->driver == adev->dev->driver)
		return 0;

	r = amdgpu_bo_reserve(bo, false);
	if (unlikely(r != 0))
		return r;

	/*
	 * We only create shared fences for internal use, but importers
	 * of the dmabuf rely on exclusive fences for implicitly
	 * tracking write hazards. As any of the current fences may
	 * correspond to a write, we need to convert all existing
	 * fences on the reservation object into a single exclusive
	 * fence.
	 */
	r = __dma_resv_make_exclusive(amdkcl_ttm_resvp(&bo->tbo));
	if (r)
		return r;

	bo->prime_shared_count++;
	amdgpu_bo_unreserve(bo);
	return 0;
}

/**
 * amdgpu_dma_buf_detach - &dma_buf_ops.detach implementation
 *
 * @dmabuf: DMA-buf where we remove the attachment from
 * @attach: the attachment to remove
 *
 * Called when an attachment is removed from the DMA-buf.
 */
static void amdgpu_dma_buf_detach(struct dma_buf *dmabuf,
				  struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dmabuf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);

	if (attach->dev->driver != adev->dev->driver && bo->prime_shared_count)
		bo->prime_shared_count--;
}

/**
 * amdgpu_dma_buf_map - &dma_buf_ops.map_dma_buf implementation
 * @attach: DMA-buf attachment
 * @dir: DMA direction
 *
 * Makes sure that the shared DMA buffer can be accessed by the target device.
 * For now, simply pins it to the GTT domain, where it should be accessible by
 * all DMA devices.
 *
 * Returns:
 * sg_table filled with the DMA addresses to use or ERR_PRT with negative error
 * code.
 */
static struct sg_table *amdgpu_dma_buf_map(struct dma_buf_attachment *attach,
					   enum dma_data_direction dir)
{
	struct dma_buf *dma_buf = attach->dmabuf;
	struct drm_gem_object *obj = dma_buf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	struct sg_table *sgt;
	long r;

	r = amdgpu_bo_pin(bo, AMDGPU_GEM_DOMAIN_GTT);
	if (r)
		return ERR_PTR(r);

	sgt = drm_prime_pages_to_sg(bo->tbo.ttm->pages, bo->tbo.num_pages);
	if (IS_ERR(sgt))
		return sgt;

	if (!dma_map_sg_attrs(attach->dev, sgt->sgl, sgt->nents, dir,
			      DMA_ATTR_SKIP_CPU_SYNC))
		goto error_free;

	return sgt;

error_free:
	sg_free_table(sgt);
	kfree(sgt);
	return ERR_PTR(-ENOMEM);
}

/**
 * amdgpu_dma_buf_unmap - &dma_buf_ops.unmap_dma_buf implementation
 * @attach: DMA-buf attachment
 * @sgt: sg_table to unmap
 * @dir: DMA direction
 *
 * This is called when a shared DMA buffer no longer needs to be accessible by
 * another device. For now, simply unpins the buffer from GTT.
 */
static void amdgpu_dma_buf_unmap(struct dma_buf_attachment *attach,
				 struct sg_table *sgt,
				 enum dma_data_direction dir)
{
	struct drm_gem_object *obj = attach->dmabuf->priv;
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);

	dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents, dir);
	sg_free_table(sgt);
	kfree(sgt);
	amdgpu_bo_unpin(bo);
}
#endif

/**
 * amdgpu_dma_buf_begin_cpu_access - &dma_buf_ops.begin_cpu_access implementation
 * @dma_buf: Shared DMA buffer
 * @direction: Direction of DMA transfer
 *
 * This is called before CPU access to the shared DMA buffer's memory. If it's
 * a read access, the buffer is moved to the GTT domain if possible, for optimal
 * CPU read performance.
 *
 * Returns:
 * 0 on success or a negative error code on failure.
 */
static int amdgpu_dma_buf_begin_cpu_access(struct dma_buf *dma_buf,
					   enum dma_data_direction direction)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(dma_buf->priv);
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	struct ttm_operation_ctx ctx = { true, false };
	u32 domain = amdgpu_display_supported_domains(adev, bo->flags);
	int ret;
	bool reads = (direction == DMA_BIDIRECTIONAL ||
		      direction == DMA_FROM_DEVICE);

	if (!reads || !(domain & AMDGPU_GEM_DOMAIN_GTT))
		return 0;

	/* move to gtt */
	ret = amdgpu_bo_reserve(bo, false);
	if (unlikely(ret != 0))
		return ret;

	if (!bo->pin_count && (bo->allowed_domains & AMDGPU_GEM_DOMAIN_GTT)) {
		amdgpu_bo_placement_from_domain(bo, AMDGPU_GEM_DOMAIN_GTT);
		ret = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
	}

	amdgpu_bo_unreserve(bo);
	return ret;
}

const struct dma_buf_ops amdgpu_dmabuf_ops = {
#if !defined(HAVE_DMA_BUF_OPS_DYNAMIC_MAPPING)
	.attach = amdgpu_dma_buf_map_attach,
	.detach = amdgpu_dma_buf_map_detach,
	.map_dma_buf = drm_gem_map_dma_buf,
	.unmap_dma_buf = drm_gem_unmap_dma_buf,
#else
	.dynamic_mapping = true,
	.attach = amdgpu_dma_buf_attach,
	.detach = amdgpu_dma_buf_detach,
	.map_dma_buf = amdgpu_dma_buf_map,
	.unmap_dma_buf = amdgpu_dma_buf_unmap,
#endif
	.release = drm_gem_dmabuf_release,
	.begin_cpu_access = amdgpu_dma_buf_begin_cpu_access,
	.mmap = drm_gem_dmabuf_mmap,
	.vmap = drm_gem_dmabuf_vmap,
	.vunmap = drm_gem_dmabuf_vunmap,
};
#endif

/**
 * amdgpu_gem_prime_export - &drm_driver.gem_prime_export implementation
 * @gobj: GEM BO
 * @flags: Flags such as DRM_CLOEXEC and DRM_RDWR.
 *
 * The main work is done by the &drm_gem_prime_export helper.
 *
 * Returns:
 * Shared DMA buffer representing the GEM BO from the given device.
 */
#ifdef HAVE_DRM_DRV_GEM_PRIME_EXPORT_PI
struct dma_buf *amdgpu_gem_prime_export(struct drm_gem_object *gobj,
#else
struct dma_buf *amdgpu_gem_prime_export(struct drm_device *dev,
					struct drm_gem_object *gobj,
#endif
					int flags)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(gobj);
	struct dma_buf *buf;

	if (amdgpu_ttm_tt_get_usermm(bo->tbo.ttm) ||
	    bo->flags & AMDGPU_GEM_CREATE_VM_ALWAYS_VALID)
		return ERR_PTR(-EPERM);

#ifdef HAVE_DRM_DRV_GEM_PRIME_EXPORT_PI
	buf = drm_gem_prime_export(gobj, flags);
#else
	buf = drm_gem_prime_export(dev, gobj, flags);
#endif
	if (!IS_ERR(buf)) {
		buf->file->f_mapping = gobj->dev->anon_inode->i_mapping;
#if defined(AMDKCL_AMDGPU_DMABUF_OPS)
		buf->ops = &amdgpu_dmabuf_ops;
#endif
	}

	return buf;
}

#ifdef HAVE_DRM_DRIVER_GEM_PRIME_RES_OBJ
/**
 * amdgpu_gem_prime_res_obj - &drm_driver.gem_prime_res_obj implementation
 * @obj: GEM BO
 *
 * Returns:
 * The BO's reservation object.
 */
struct reservation_object *amdgpu_gem_prime_res_obj(struct drm_gem_object *obj)
{
       struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);

       return amdkcl_ttm_resvp(&bo->tbo);
}
#endif

#if !defined(HAVE_DMA_BUF_OPS_DYNAMIC_MAPPING)
/**
 * amdgpu_gem_prime_import_sg_table - &drm_driver.gem_prime_import_sg_table
 * implementation
 * @dev: DRM device
 * @attach: DMA-buf attachment
 * @sg: Scatter/gather table
 *
 * Imports shared DMA buffer memory exported by another device.
 *
 * Returns:
 * A new GEM BO of the given DRM device, representing the memory
 * described by the given DMA-buf attachment and scatter/gather table.
 */
struct drm_gem_object *
amdgpu_gem_prime_import_sg_table(struct drm_device *dev,
				 struct dma_buf_attachment *attach,
				 struct sg_table *sg)
{
	struct dma_resv *resv = attach->dmabuf->resv;
	struct amdgpu_device *adev = dev->dev_private;
	struct amdgpu_bo *bo;
	struct amdgpu_bo_param bp;
	int ret;

	memset(&bp, 0, sizeof(bp));
	bp.size = attach->dmabuf->size;
	bp.byte_align = PAGE_SIZE;
	bp.domain = AMDGPU_GEM_DOMAIN_CPU;
	bp.flags = 0;
	bp.type = ttm_bo_type_sg;
	bp.resv = resv;
	dma_resv_lock(resv, NULL);
	ret = amdgpu_bo_create(adev, &bp, &bo);
	if (ret)
		goto error;

	bo->tbo.sg = sg;
	bo->tbo.ttm->sg = sg;
	bo->allowed_domains = AMDGPU_GEM_DOMAIN_GTT;
	bo->preferred_domains = AMDGPU_GEM_DOMAIN_GTT;
#if defined(AMDKCL_AMDGPU_DMABUF_OPS)
	if (attach->dmabuf->ops != &amdgpu_dmabuf_ops)
#endif
		bo->prime_shared_count = 1;

	dma_resv_unlock(resv);
	return &bo->tbo.base;

error:
	dma_resv_unlock(resv);
	return ERR_PTR(ret);
}
#endif

#if defined(AMDKCL_AMDGPU_DMABUF_OPS)
#if !defined(HAVE_DMA_BUF_OPS_DYNAMIC_MAPPING)
/**
 * amdgpu_gem_prime_import - &drm_driver.gem_prime_import implementation
 * @dev: DRM device
 * @dma_buf: Shared DMA buffer
 *
 * The main work is done by the &drm_gem_prime_import helper, which in turn
 * uses &amdgpu_gem_prime_import_sg_table.
 *
 * Returns:
 * GEM BO representing the shared DMA buffer for the given device.
 */
struct drm_gem_object *amdgpu_gem_prime_import(struct drm_device *dev,
					    struct dma_buf *dma_buf)
{
	struct drm_gem_object *obj;

	if (dma_buf->ops == &amdgpu_dmabuf_ops) {
		obj = dma_buf->priv;
		if (obj->dev == dev) {
			/*
			 * Importing dmabuf exported from out own gem increases
			 * refcount on gem itself instead of f_count of dmabuf.
			 */
			drm_gem_object_get(obj);
			return obj;
		}
	}

	return drm_gem_prime_import(dev, dma_buf);
}
#else
/**
 * amdgpu_dma_buf_create_obj - create BO for DMA-buf import
 *
 * @dev: DRM device
 * @dma_buf: DMA-buf
 *
 * Creates an empty SG BO for DMA-buf import.
 *
 * Returns:
 * A new GEM BO of the given DRM device, representing the memory
 * described by the given DMA-buf attachment and scatter/gather table.
 */
static struct drm_gem_object *
amdgpu_dma_buf_create_obj(struct drm_device *dev, struct dma_buf *dma_buf)
{
	struct dma_resv *resv = dma_buf->resv;
	struct amdgpu_device *adev = dev->dev_private;
	struct amdgpu_bo *bo;
	struct amdgpu_bo_param bp;
	int ret;

	memset(&bp, 0, sizeof(bp));
	bp.size = dma_buf->size;
	bp.byte_align = PAGE_SIZE;
	bp.domain = AMDGPU_GEM_DOMAIN_CPU;
	bp.flags = 0;
	bp.type = ttm_bo_type_sg;
	bp.resv = resv;
	dma_resv_lock(resv, NULL);
	ret = amdgpu_bo_create(adev, &bp, &bo);
	if (ret)
		goto error;

	bo->allowed_domains = AMDGPU_GEM_DOMAIN_GTT;
	bo->preferred_domains = AMDGPU_GEM_DOMAIN_GTT;
	if (dma_buf->ops != &amdgpu_dmabuf_ops)
		bo->prime_shared_count = 1;

	dma_resv_unlock(resv);
	return &bo->tbo.base;

error:
	dma_resv_unlock(resv);
	return ERR_PTR(ret);
}

/**
 * amdgpu_gem_prime_import - &drm_driver.gem_prime_import implementation
 * @dev: DRM device
 * @dma_buf: Shared DMA buffer
 *
 * Import a dma_buf into a the driver and potentially create a new GEM object.
 *
 * Returns:
 * GEM BO representing the shared DMA buffer for the given device.
 */
struct drm_gem_object *amdgpu_gem_prime_import(struct drm_device *dev,
					       struct dma_buf *dma_buf)
{
	struct dma_buf_attachment *attach;
	struct drm_gem_object *obj;

	if (dma_buf->ops == &amdgpu_dmabuf_ops) {
		obj = dma_buf->priv;
		if (obj->dev == dev) {
			/*
			 * Importing dmabuf exported from out own gem increases
			 * refcount on gem itself instead of f_count of dmabuf.
			 */
			drm_gem_object_get(obj);
			return obj;
		}
	}

	obj = amdgpu_dma_buf_create_obj(dev, dma_buf);
	if (IS_ERR(obj))
		return obj;

	attach = dma_buf_dynamic_attach(dma_buf, dev->dev, true);
	if (IS_ERR(attach)) {
		drm_gem_object_put(obj);
		return ERR_CAST(attach);
	}

	get_dma_buf(dma_buf);
	obj->import_attach = attach;
	return obj;
}
#endif
#else
int amdgpu_gem_prime_pin(struct drm_gem_object *obj)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	long ret = 0;

	ret = amdgpu_bo_reserve(bo, false);
	if (unlikely(ret != 0))
		return ret;

	/*
	 * Wait for all shared fences to complete before we switch to future
	 * use of exclusive fence on this prime shared bo.
	 */
	ret = dma_resv_wait_timeout_rcu(bo->tbo.resv, true, false,
						  MAX_SCHEDULE_TIMEOUT);
	if (unlikely(ret < 0)) {
		DRM_DEBUG_PRIME("Fence wait failed: %li\n", ret);
		amdgpu_bo_unreserve(bo);
		return ret;
	}

	/* pin buffer into GTT */
	ret = amdgpu_bo_pin(bo, AMDGPU_GEM_DOMAIN_GTT);
	if (likely(ret == 0))
		bo->prime_shared_count++;

	amdgpu_bo_unreserve(bo);
	return ret;
}

void amdgpu_gem_prime_unpin(struct drm_gem_object *obj)
{
	struct amdgpu_bo *bo = gem_to_amdgpu_bo(obj);
	int ret = 0;

	ret = amdgpu_bo_reserve(bo, true);
	if (unlikely(ret != 0))
		return;

	amdgpu_bo_unpin(bo);
	if (bo->prime_shared_count)
		bo->prime_shared_count--;
	amdgpu_bo_unreserve(bo);
}
#endif
