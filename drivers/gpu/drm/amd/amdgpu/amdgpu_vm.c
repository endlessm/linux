/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
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
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#include <linux/dma-fence-array.h>
#include <linux/interval_tree_generic.h>
#include <drm/drmP.h>
#include <drm/amdgpu_drm.h>
#include "amdgpu.h"
#include "amdgpu_trace.h"

/*
 * GPUVM
 * GPUVM is similar to the legacy gart on older asics, however
 * rather than there being a single global gart table
 * for the entire GPU, there are multiple VM page tables active
 * at any given time.  The VM page tables can contain a mix
 * vram pages and system memory pages and system memory pages
 * can be mapped as snooped (cached system pages) or unsnooped
 * (uncached system pages).
 * Each VM has an ID associated with it and there is a page table
 * associated with each VMID.  When execting a command buffer,
 * the kernel tells the the ring what VMID to use for that command
 * buffer.  VMIDs are allocated dynamically as commands are submitted.
 * The userspace drivers maintain their own address space and the kernel
 * sets up their pages tables accordingly when they submit their
 * command buffers and a VMID is assigned.
 * Cayman/Trinity support up to 8 active VMs at any given time;
 * SI supports 16.
 */

#define START(node) ((node)->start)
#define LAST(node) ((node)->last)

INTERVAL_TREE_DEFINE(struct amdgpu_bo_va_mapping, rb, uint64_t, __subtree_last,
		     START, LAST, static, amdgpu_vm_it)

#undef START
#undef LAST

/* Local structure. Encapsulate some VM table update parameters to reduce
 * the number of function parameters
 */
struct amdgpu_pte_update_params {
	/* amdgpu device we do this update for */
	struct amdgpu_device *adev;
	/* optional amdgpu_vm we do this update for */
	struct amdgpu_vm *vm;
	/* address where to copy page table entries from */
	uint64_t src;
	/* indirect buffer to fill with commands */
	struct amdgpu_ib *ib;
	/* Function which actually does the update */
	void (*func)(struct amdgpu_pte_update_params *params, uint64_t pe,
		     uint64_t addr, unsigned count, uint32_t incr,
		     uint64_t flags);
	/* The next two are used during VM update by CPU
	 *  DMA addresses to use for mapping
	 *  Kernel pointer of PD/PT BO that needs to be updated
	 */
	dma_addr_t *pages_addr;
	void *kptr;
};

/* Helper to disable partial resident texture feature from a fence callback */
struct amdgpu_prt_cb {
	struct amdgpu_device *adev;
	struct dma_fence_cb cb;
};

/**
 * amdgpu_vm_num_entries - return the number of entries in a PD/PT
 *
 * @adev: amdgpu_device pointer
 *
 * Calculate the number of entries in a page directory or page table.
 */
static unsigned amdgpu_vm_num_entries(struct amdgpu_device *adev,
				      unsigned level)
{
	if (level == 0)
		/* For the root directory */
		return adev->vm_manager.max_pfn >>
			(adev->vm_manager.block_size *
			 adev->vm_manager.num_level);
	else if (level == adev->vm_manager.num_level)
		/* For the page tables on the leaves */
		return AMDGPU_VM_PTE_COUNT(adev);
	else
		/* Everything in between */
		return 1 << adev->vm_manager.block_size;
}

/**
 * amdgpu_vm_bo_size - returns the size of the BOs in bytes
 *
 * @adev: amdgpu_device pointer
 *
 * Calculate the size of the BO for a page directory or page table in bytes.
 */
static unsigned amdgpu_vm_bo_size(struct amdgpu_device *adev, unsigned level)
{
	return AMDGPU_GPU_PAGE_ALIGN(amdgpu_vm_num_entries(adev, level) * 8);
}

/**
 * amdgpu_vm_get_pd_bo - add the VM PD to a validation list
 *
 * @vm: vm providing the BOs
 * @validated: head of validation list
 * @entry: entry to add
 *
 * Add the page directory to the list of BOs to
 * validate for command submission.
 */
void amdgpu_vm_get_pd_bo(struct amdgpu_vm *vm,
			 struct list_head *validated,
			 struct amdgpu_bo_list_entry *entry)
{
	entry->robj = vm->root.base.bo;
	entry->priority = 0;
	entry->tv.bo = &entry->robj->tbo;
	entry->tv.shared = true;
	entry->user_pages = NULL;
	list_add(&entry->tv.head, validated);
}

/**
 * amdgpu_vm_validate_pt_bos - validate the page table BOs
 *
 * @adev: amdgpu device pointer
 * @vm: vm providing the BOs
 * @validate: callback to do the validation
 * @param: parameter for the validation callback
 *
 * Validate the page table BOs on command submission if neccessary.
 */
int amdgpu_vm_validate_pt_bos(struct amdgpu_device *adev, struct amdgpu_vm *vm,
			      int (*validate)(void *p, struct amdgpu_bo *bo),
			      void *param)
{
	struct ttm_bo_global *glob = adev->mman.bdev.glob;
	int r;

	spin_lock(&vm->status_lock);
	while (!list_empty(&vm->evicted)) {
		struct amdgpu_vm_bo_base *bo_base;
		struct amdgpu_bo *bo;

		bo_base = list_first_entry(&vm->evicted,
					   struct amdgpu_vm_bo_base,
					   vm_status);
		spin_unlock(&vm->status_lock);

		bo = bo_base->bo;
		BUG_ON(!bo);
		if (bo->parent) {
			r = validate(param, bo);
			if (r)
				return r;

			spin_lock(&glob->lru_lock);
			ttm_bo_move_to_lru_tail(&bo->tbo);
			if (bo->shadow)
				ttm_bo_move_to_lru_tail(&bo->shadow->tbo);
			spin_unlock(&glob->lru_lock);
		}

		if (bo->tbo.type == ttm_bo_type_kernel &&
		    vm->use_cpu_for_update) {
			r = amdgpu_bo_kmap(bo, NULL);
			if (r)
				return r;
		}

		spin_lock(&vm->status_lock);
		if (bo->tbo.type != ttm_bo_type_kernel)
			list_move(&bo_base->vm_status, &vm->moved);
		else
			list_move(&bo_base->vm_status, &vm->relocated);
	}
	spin_unlock(&vm->status_lock);

	return 0;
}

/**
 * amdgpu_vm_ready - check VM is ready for updates
 *
 * @vm: VM to check
 *
 * Check if all VM PDs/PTs are ready for updates
 */
bool amdgpu_vm_ready(struct amdgpu_vm *vm)
{
	bool ready;

	spin_lock(&vm->status_lock);
	ready = list_empty(&vm->evicted);
	spin_unlock(&vm->status_lock);

	return ready;
}

/**
 * amdgpu_vm_alloc_levels - allocate the PD/PT levels
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @saddr: start of the address range
 * @eaddr: end of the address range
 *
 * Make sure the page directories and page tables are allocated
 */
static int amdgpu_vm_alloc_levels(struct amdgpu_device *adev,
				  struct amdgpu_vm *vm,
				  struct amdgpu_vm_pt *parent,
				  uint64_t saddr, uint64_t eaddr,
				  unsigned level)
{
	unsigned shift = (adev->vm_manager.num_level - level) *
		adev->vm_manager.block_size;
	unsigned pt_idx, from, to;
	int r;
	u64 flags;
	uint64_t init_value = 0;

	if (!parent->entries) {
		unsigned num_entries = amdgpu_vm_num_entries(adev, level);

		parent->entries = kvmalloc_array(num_entries,
						   sizeof(struct amdgpu_vm_pt),
						   GFP_KERNEL | __GFP_ZERO);
		if (!parent->entries)
			return -ENOMEM;
		memset(parent->entries, 0 , sizeof(struct amdgpu_vm_pt));
	}

	from = saddr >> shift;
	to = eaddr >> shift;
	if (from >= amdgpu_vm_num_entries(adev, level) ||
	    to >= amdgpu_vm_num_entries(adev, level))
		return -EINVAL;

	if (to > parent->last_entry_used)
		parent->last_entry_used = to;

	++level;
	saddr = saddr & ((1 << shift) - 1);
	eaddr = eaddr & ((1 << shift) - 1);

	flags = AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS |
			AMDGPU_GEM_CREATE_VRAM_CLEARED;
	if (vm->use_cpu_for_update)
		flags |= AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	else
		flags |= (AMDGPU_GEM_CREATE_NO_CPU_ACCESS |
				AMDGPU_GEM_CREATE_SHADOW);

	if (vm->pte_support_ats) {
		init_value = AMDGPU_PTE_SYSTEM;
		if (level != adev->vm_manager.num_level - 1)
			init_value |= AMDGPU_PDE_PTE;
	}

	/* walk over the address space and allocate the page tables */
	for (pt_idx = from; pt_idx <= to; ++pt_idx) {
		struct reservation_object *resv = vm->root.base.bo->tbo.resv;
		struct amdgpu_vm_pt *entry = &parent->entries[pt_idx];
		struct amdgpu_bo *pt;

		if (!entry->base.bo) {
			r = amdgpu_bo_create(adev,
					     amdgpu_vm_bo_size(adev, level),
					     AMDGPU_GPU_PAGE_SIZE, true,
					     AMDGPU_GEM_DOMAIN_VRAM,
					     flags,
					     NULL, resv, init_value, &pt);
			if (r)
				return r;

			if (vm->use_cpu_for_update) {
				r = amdgpu_bo_kmap(pt, NULL);
				if (r) {
					amdgpu_bo_unref(&pt);
					return r;
				}
			}

			/* Keep a reference to the root directory to avoid
			* freeing them up in the wrong order.
			*/
			pt->parent = amdgpu_bo_ref(parent->base.bo);

			entry->base.vm = vm;
			entry->base.bo = pt;
			list_add_tail(&entry->base.bo_list, &pt->va);
			spin_lock(&vm->status_lock);
			list_add(&entry->base.vm_status, &vm->relocated);
			spin_unlock(&vm->status_lock);
			entry->addr = 0;
		}

		if (level < adev->vm_manager.num_level) {
			uint64_t sub_saddr = (pt_idx == from) ? saddr : 0;
			uint64_t sub_eaddr = (pt_idx == to) ? eaddr :
				((1 << shift) - 1);
			r = amdgpu_vm_alloc_levels(adev, vm, entry, sub_saddr,
						   sub_eaddr, level);
			if (r)
				return r;
		}
	}

	return 0;
}

/**
 * amdgpu_vm_alloc_pts - Allocate page tables.
 *
 * @adev: amdgpu_device pointer
 * @vm: VM to allocate page tables for
 * @saddr: Start address which needs to be allocated
 * @size: Size from start address we need.
 *
 * Make sure the page tables are allocated.
 */
int amdgpu_vm_alloc_pts(struct amdgpu_device *adev,
			struct amdgpu_vm *vm,
			uint64_t saddr, uint64_t size)
{
	uint64_t last_pfn;
	uint64_t eaddr;

	/* validate the parameters */
	if (saddr & AMDGPU_GPU_PAGE_MASK || size & AMDGPU_GPU_PAGE_MASK)
		return -EINVAL;

	eaddr = saddr + size - 1;
	last_pfn = eaddr / AMDGPU_GPU_PAGE_SIZE;
	if (last_pfn >= adev->vm_manager.max_pfn) {
		dev_err(adev->dev, "va above limit (0x%08llX >= 0x%08llX)\n",
			last_pfn, adev->vm_manager.max_pfn);
		return -EINVAL;
	}

	saddr /= AMDGPU_GPU_PAGE_SIZE;
	eaddr /= AMDGPU_GPU_PAGE_SIZE;

	return amdgpu_vm_alloc_levels(adev, vm, &vm->root, saddr, eaddr, 0);
}

/**
 * amdgpu_vm_had_gpu_reset - check if reset occured since last use
 *
 * @adev: amdgpu_device pointer
 * @id: VMID structure
 *
 * Check if GPU reset occured since last use of the VMID.
 */
static bool amdgpu_vm_had_gpu_reset(struct amdgpu_device *adev,
				    struct amdgpu_vm_id *id)
{
	return id->current_gpu_reset_count !=
		atomic_read(&adev->gpu_reset_counter);
}

static bool amdgpu_vm_reserved_vmid_ready(struct amdgpu_vm *vm, unsigned vmhub)
{
	return !!vm->reserved_vmid[vmhub];
}

/* idr_mgr->lock must be held */
static int amdgpu_vm_grab_reserved_vmid_locked(struct amdgpu_vm *vm,
					       struct amdgpu_ring *ring,
					       struct amdgpu_sync *sync,
					       struct dma_fence *fence,
					       struct amdgpu_job *job)
{
	struct amdgpu_device *adev = ring->adev;
	unsigned vmhub = ring->funcs->vmhub;
	uint64_t fence_context = adev->fence_context + ring->idx;
	struct amdgpu_vm_id *id = vm->reserved_vmid[vmhub];
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];
	struct dma_fence *updates = sync->last_vm_update;
	int r = 0;
	struct dma_fence *flushed, *tmp;
	bool needs_flush = vm->use_cpu_for_update;

	flushed  = id->flushed_updates;
	if ((amdgpu_vm_had_gpu_reset(adev, id)) ||
	    (atomic64_read(&id->owner) != vm->client_id) ||
	    (job->vm_pd_addr != id->pd_gpu_addr) ||
	    (updates && (!flushed || updates->context != flushed->context ||
			dma_fence_is_later(updates, flushed))) ||
	    (!id->last_flush || (id->last_flush->context != fence_context &&
				 !dma_fence_is_signaled(id->last_flush)))) {
		needs_flush = true;
		/* to prevent one context starved by another context */
		id->pd_gpu_addr = 0;
		tmp = amdgpu_sync_peek_fence(&id->active, ring);
		if (tmp) {
			r = amdgpu_sync_fence(adev, sync, tmp);
			return r;
		}
	}

	/* Good we can use this VMID. Remember this submission as
	* user of the VMID.
	*/
	r = amdgpu_sync_fence(ring->adev, &id->active, fence);
	if (r)
		goto out;

	if (updates && (!flushed || updates->context != flushed->context ||
			dma_fence_is_later(updates, flushed))) {
		dma_fence_put(id->flushed_updates);
		id->flushed_updates = dma_fence_get(updates);
	}
	id->pd_gpu_addr = job->vm_pd_addr;
	atomic64_set(&id->owner, vm->client_id);
	job->vm_needs_flush = needs_flush;
	if (needs_flush) {
		dma_fence_put(id->last_flush);
		id->last_flush = NULL;
	}
	job->vm_id = id - id_mgr->ids;
	trace_amdgpu_vm_grab_id(vm, ring, job);
out:
	return r;
}

/**
 * amdgpu_vm_grab_id - allocate the next free VMID
 *
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 * @sync: sync object where we add dependencies
 * @fence: fence protecting ID from reuse
 *
 * Allocate an id for the vm, adding fences to the sync obj as necessary.
 */
int amdgpu_vm_grab_id(struct amdgpu_vm *vm, struct amdgpu_ring *ring,
		      struct amdgpu_sync *sync, struct dma_fence *fence,
		      struct amdgpu_job *job)
{
	struct amdgpu_device *adev = ring->adev;
	unsigned vmhub = ring->funcs->vmhub;
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];
	uint64_t fence_context = adev->fence_context + ring->idx;
	struct dma_fence *updates = sync->last_vm_update;
	struct amdgpu_vm_id *id, *idle;
	struct dma_fence **fences;
	unsigned i;
	int r = 0;

	mutex_lock(&id_mgr->lock);
	if (amdgpu_vm_reserved_vmid_ready(vm, vmhub)) {
		r = amdgpu_vm_grab_reserved_vmid_locked(vm, ring, sync, fence, job);
		mutex_unlock(&id_mgr->lock);
		return r;
	}
	fences = kmalloc_array(sizeof(void *), id_mgr->num_ids, GFP_KERNEL);
	if (!fences) {
		mutex_unlock(&id_mgr->lock);
		return -ENOMEM;
	}
	/* Check if we have an idle VMID */
	i = 0;
	list_for_each_entry(idle, &id_mgr->ids_lru, list) {
		fences[i] = amdgpu_sync_peek_fence(&idle->active, ring);
		if (!fences[i])
			break;
		++i;
	}

	/* If we can't find a idle VMID to use, wait till one becomes available */
	if (&idle->list == &id_mgr->ids_lru) {
		u64 fence_context = adev->vm_manager.fence_context + ring->idx;
		unsigned seqno = ++adev->vm_manager.seqno[ring->idx];
		struct dma_fence_array *array;
		unsigned j;

		for (j = 0; j < i; ++j)
			dma_fence_get(fences[j]);

		array = dma_fence_array_create(i, fences, fence_context,
					   seqno, true);
		if (!array) {
			for (j = 0; j < i; ++j)
				dma_fence_put(fences[j]);
			kfree(fences);
			r = -ENOMEM;
			goto error;
		}


		r = amdgpu_sync_fence(ring->adev, sync, &array->base);
		dma_fence_put(&array->base);
		if (r)
			goto error;

		mutex_unlock(&id_mgr->lock);
		return 0;

	}
	kfree(fences);

	job->vm_needs_flush = vm->use_cpu_for_update;
	/* Check if we can use a VMID already assigned to this VM */
	list_for_each_entry_reverse(id, &id_mgr->ids_lru, list) {
		struct dma_fence *flushed;
		bool needs_flush = vm->use_cpu_for_update;

		/* Check all the prerequisites to using this VMID */
		if (amdgpu_vm_had_gpu_reset(adev, id))
			continue;

		if (atomic64_read(&id->owner) != vm->client_id)
			continue;

		if (job->vm_pd_addr != id->pd_gpu_addr)
			continue;

		if (!id->last_flush ||
		    (id->last_flush->context != fence_context &&
		     !dma_fence_is_signaled(id->last_flush)))
			needs_flush = true;

		flushed  = id->flushed_updates;
		if (updates && (!flushed || dma_fence_is_later(updates, flushed)))
			needs_flush = true;

		/* Concurrent flushes are only possible starting with Vega10 */
		if (adev->asic_type < CHIP_VEGA10 && needs_flush)
			continue;

		/* Good we can use this VMID. Remember this submission as
		 * user of the VMID.
		 */
		r = amdgpu_sync_fence(ring->adev, &id->active, fence);
		if (r)
			goto error;

		if (updates && (!flushed || dma_fence_is_later(updates, flushed))) {
			dma_fence_put(id->flushed_updates);
			id->flushed_updates = dma_fence_get(updates);
		}

		if (needs_flush)
			goto needs_flush;
		else
			goto no_flush_needed;

	};

	/* Still no ID to use? Then use the idle one found earlier */
	id = idle;

	/* Remember this submission as user of the VMID */
	r = amdgpu_sync_fence(ring->adev, &id->active, fence);
	if (r)
		goto error;

	id->pd_gpu_addr = job->vm_pd_addr;
	dma_fence_put(id->flushed_updates);
	id->flushed_updates = dma_fence_get(updates);
	atomic64_set(&id->owner, vm->client_id);

needs_flush:
	job->vm_needs_flush = true;
	dma_fence_put(id->last_flush);
	id->last_flush = NULL;

no_flush_needed:
	list_move_tail(&id->list, &id_mgr->ids_lru);

	job->vm_id = id - id_mgr->ids;
	trace_amdgpu_vm_grab_id(vm, ring, job);

error:
	mutex_unlock(&id_mgr->lock);
	return r;
}

static void amdgpu_vm_free_reserved_vmid(struct amdgpu_device *adev,
					  struct amdgpu_vm *vm,
					  unsigned vmhub)
{
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];

	mutex_lock(&id_mgr->lock);
	if (vm->reserved_vmid[vmhub]) {
		list_add(&vm->reserved_vmid[vmhub]->list,
			&id_mgr->ids_lru);
		vm->reserved_vmid[vmhub] = NULL;
		atomic_dec(&id_mgr->reserved_vmid_num);
	}
	mutex_unlock(&id_mgr->lock);
}

static int amdgpu_vm_alloc_reserved_vmid(struct amdgpu_device *adev,
					 struct amdgpu_vm *vm,
					 unsigned vmhub)
{
	struct amdgpu_vm_id_manager *id_mgr;
	struct amdgpu_vm_id *idle;
	int r = 0;

	id_mgr = &adev->vm_manager.id_mgr[vmhub];
	mutex_lock(&id_mgr->lock);
	if (vm->reserved_vmid[vmhub])
		goto unlock;
	if (atomic_inc_return(&id_mgr->reserved_vmid_num) >
	    AMDGPU_VM_MAX_RESERVED_VMID) {
		DRM_ERROR("Over limitation of reserved vmid\n");
		atomic_dec(&id_mgr->reserved_vmid_num);
		r = -EINVAL;
		goto unlock;
	}
	/* Select the first entry VMID */
	idle = list_first_entry(&id_mgr->ids_lru, struct amdgpu_vm_id, list);
	list_del_init(&idle->list);
	vm->reserved_vmid[vmhub] = idle;
	mutex_unlock(&id_mgr->lock);

	return 0;
unlock:
	mutex_unlock(&id_mgr->lock);
	return r;
}

/**
 * amdgpu_vm_check_compute_bug - check whether asic has compute vm bug
 *
 * @adev: amdgpu_device pointer
 */
void amdgpu_vm_check_compute_bug(struct amdgpu_device *adev)
{
	const struct amdgpu_ip_block *ip_block;
	bool has_compute_vm_bug;
	struct amdgpu_ring *ring;
	int i;

	has_compute_vm_bug = false;

	ip_block = amdgpu_get_ip_block(adev, AMD_IP_BLOCK_TYPE_GFX);
	if (ip_block) {
		/* Compute has a VM bug for GFX version < 7.
		   Compute has a VM bug for GFX 8 MEC firmware version < 673.*/
		if (ip_block->version->major <= 7)
			has_compute_vm_bug = true;
		else if (ip_block->version->major == 8)
			if (adev->gfx.mec_fw_version < 673)
				has_compute_vm_bug = true;
	}

	for (i = 0; i < adev->num_rings; i++) {
		ring = adev->rings[i];
		if (ring->funcs->type == AMDGPU_RING_TYPE_COMPUTE)
			/* only compute rings */
			ring->has_compute_vm_bug = has_compute_vm_bug;
		else
			ring->has_compute_vm_bug = false;
	}
}

bool amdgpu_vm_need_pipeline_sync(struct amdgpu_ring *ring,
				  struct amdgpu_job *job)
{
	struct amdgpu_device *adev = ring->adev;
	unsigned vmhub = ring->funcs->vmhub;
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];
	struct amdgpu_vm_id *id;
	bool gds_switch_needed;
	bool vm_flush_needed = job->vm_needs_flush || ring->has_compute_vm_bug;

	if (job->vm_id == 0)
		return false;
	id = &id_mgr->ids[job->vm_id];
	gds_switch_needed = ring->funcs->emit_gds_switch && (
		id->gds_base != job->gds_base ||
		id->gds_size != job->gds_size ||
		id->gws_base != job->gws_base ||
		id->gws_size != job->gws_size ||
		id->oa_base != job->oa_base ||
		id->oa_size != job->oa_size);

	if (amdgpu_vm_had_gpu_reset(adev, id))
		return true;

	return vm_flush_needed || gds_switch_needed;
}

static bool amdgpu_vm_is_large_bar(struct amdgpu_device *adev)
{
	return (adev->mc.real_vram_size == adev->mc.visible_vram_size);
}

/**
 * amdgpu_vm_flush - hardware flush the vm
 *
 * @ring: ring to use for flush
 * @vm_id: vmid number to use
 * @pd_addr: address of the page directory
 *
 * Emit a VM flush when it is necessary.
 */
int amdgpu_vm_flush(struct amdgpu_ring *ring, struct amdgpu_job *job, bool need_pipe_sync)
{
	struct amdgpu_device *adev = ring->adev;
	unsigned vmhub = ring->funcs->vmhub;
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];
	struct amdgpu_vm_id *id = &id_mgr->ids[job->vm_id];
	bool gds_switch_needed = ring->funcs->emit_gds_switch && (
		id->gds_base != job->gds_base ||
		id->gds_size != job->gds_size ||
		id->gws_base != job->gws_base ||
		id->gws_size != job->gws_size ||
		id->oa_base != job->oa_base ||
		id->oa_size != job->oa_size);
	bool vm_flush_needed = job->vm_needs_flush;
	unsigned patch_offset = 0;
	int r;

	if (amdgpu_vm_had_gpu_reset(adev, id)) {
		gds_switch_needed = true;
		vm_flush_needed = true;
	}

	if (!vm_flush_needed && !gds_switch_needed && !need_pipe_sync)
		return 0;

	if (ring->funcs->init_cond_exec)
		patch_offset = amdgpu_ring_init_cond_exec(ring);

	if (need_pipe_sync)
		amdgpu_ring_emit_pipeline_sync(ring);

	if (ring->funcs->emit_vm_flush && vm_flush_needed) {
		struct dma_fence *fence;

		trace_amdgpu_vm_flush(ring, job->vm_id, job->vm_pd_addr);
		amdgpu_ring_emit_vm_flush(ring, job->vm_id, job->vm_pd_addr);

		r = amdgpu_fence_emit(ring, &fence);
		if (r)
			return r;

		mutex_lock(&id_mgr->lock);
		dma_fence_put(id->last_flush);
		id->last_flush = fence;
		id->current_gpu_reset_count = atomic_read(&adev->gpu_reset_counter);
		mutex_unlock(&id_mgr->lock);
	}

	if (ring->funcs->emit_gds_switch && gds_switch_needed) {
		id->gds_base = job->gds_base;
		id->gds_size = job->gds_size;
		id->gws_base = job->gws_base;
		id->gws_size = job->gws_size;
		id->oa_base = job->oa_base;
		id->oa_size = job->oa_size;
		amdgpu_ring_emit_gds_switch(ring, job->vm_id, job->gds_base,
					    job->gds_size, job->gws_base,
					    job->gws_size, job->oa_base,
					    job->oa_size);
	}

	if (ring->funcs->patch_cond_exec)
		amdgpu_ring_patch_cond_exec(ring, patch_offset);

	/* the double SWITCH_BUFFER here *cannot* be skipped by COND_EXEC */
	if (ring->funcs->emit_switch_buffer) {
		amdgpu_ring_emit_switch_buffer(ring);
		amdgpu_ring_emit_switch_buffer(ring);
	}
	return 0;
}

/**
 * amdgpu_vm_reset_id - reset VMID to zero
 *
 * @adev: amdgpu device structure
 * @vm_id: vmid number to use
 *
 * Reset saved GDW, GWS and OA to force switch on next flush.
 */
void amdgpu_vm_reset_id(struct amdgpu_device *adev, unsigned vmhub,
			unsigned vmid)
{
	struct amdgpu_vm_id_manager *id_mgr = &adev->vm_manager.id_mgr[vmhub];
	struct amdgpu_vm_id *id = &id_mgr->ids[vmid];

	atomic64_set(&id->owner, 0);
	id->gds_base = 0;
	id->gds_size = 0;
	id->gws_base = 0;
	id->gws_size = 0;
	id->oa_base = 0;
	id->oa_size = 0;
}

/**
 * amdgpu_vm_reset_all_id - reset VMID to zero
 *
 * @adev: amdgpu device structure
 *
 * Reset VMID to force flush on next use
 */
void amdgpu_vm_reset_all_ids(struct amdgpu_device *adev)
{
	unsigned i, j;

	for (i = 0; i < AMDGPU_MAX_VMHUBS; ++i) {
		struct amdgpu_vm_id_manager *id_mgr =
			&adev->vm_manager.id_mgr[i];

		for (j = 1; j < id_mgr->num_ids; ++j)
			amdgpu_vm_reset_id(adev, i, j);
	}
}

/**
 * amdgpu_vm_bo_find - find the bo_va for a specific vm & bo
 *
 * @vm: requested vm
 * @bo: requested buffer object
 *
 * Find @bo inside the requested vm.
 * Search inside the @bos vm list for the requested vm
 * Returns the found bo_va or NULL if none is found
 *
 * Object has to be reserved!
 */
struct amdgpu_bo_va *amdgpu_vm_bo_find(struct amdgpu_vm *vm,
				       struct amdgpu_bo *bo)
{
	struct amdgpu_bo_va *bo_va;

	list_for_each_entry(bo_va, &bo->va, base.bo_list) {
		if (bo_va->base.vm == vm) {
			return bo_va;
		}
	}
	return NULL;
}

/**
 * amdgpu_vm_do_set_ptes - helper to call the right asic function
 *
 * @params: see amdgpu_pte_update_params definition
 * @pe: addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Traces the parameters and calls the right asic functions
 * to setup the page table using the DMA.
 */
static void amdgpu_vm_do_set_ptes(struct amdgpu_pte_update_params *params,
				  uint64_t pe, uint64_t addr,
				  unsigned count, uint32_t incr,
				  uint64_t flags)
{
	trace_amdgpu_vm_set_ptes(pe, addr, count, incr, flags);

	if (count < 3) {
		amdgpu_vm_write_pte(params->adev, params->ib, pe,
				    addr | flags, count, incr);

	} else {
		amdgpu_vm_set_pte_pde(params->adev, params->ib, pe, addr,
				      count, incr, flags);
	}
}

/**
 * amdgpu_vm_do_copy_ptes - copy the PTEs from the GART
 *
 * @params: see amdgpu_pte_update_params definition
 * @pe: addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Traces the parameters and calls the DMA function to copy the PTEs.
 */
static void amdgpu_vm_do_copy_ptes(struct amdgpu_pte_update_params *params,
				   uint64_t pe, uint64_t addr,
				   unsigned count, uint32_t incr,
				   uint64_t flags)
{
	uint64_t src = (params->src + (addr >> 12) * 8);


	trace_amdgpu_vm_copy_ptes(pe, src, count);

	amdgpu_vm_copy_pte(params->adev, params->ib, pe, src, count);
}

/**
 * amdgpu_vm_map_gart - Resolve gart mapping of addr
 *
 * @pages_addr: optional DMA address to use for lookup
 * @addr: the unmapped addr
 *
 * Look up the physical address of the page that the pte resolves
 * to and return the pointer for the page table entry.
 */
static uint64_t amdgpu_vm_map_gart(const dma_addr_t *pages_addr, uint64_t addr)
{
	uint64_t result;

	/* page table offset */
	result = pages_addr[addr >> PAGE_SHIFT];

	/* in case cpu page size != gpu page size*/
	result |= addr & (~PAGE_MASK);

	result &= 0xFFFFFFFFFFFFF000ULL;

	return result;
}

/**
 * amdgpu_vm_cpu_set_ptes - helper to update page tables via CPU
 *
 * @params: see amdgpu_pte_update_params definition
 * @pe: kmap addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Write count number of PT/PD entries directly.
 */
static void amdgpu_vm_cpu_set_ptes(struct amdgpu_pte_update_params *params,
				   uint64_t pe, uint64_t addr,
				   unsigned count, uint32_t incr,
				   uint64_t flags)
{
	unsigned int i;
	uint64_t value;

	trace_amdgpu_vm_set_ptes(pe, addr, count, incr, flags);

	for (i = 0; i < count; i++) {
		value = params->pages_addr ?
			amdgpu_vm_map_gart(params->pages_addr, addr) :
			addr;
		amdgpu_gart_set_pte_pde(params->adev, (void *)(uintptr_t)pe,
					i, value, flags);
		addr += incr;
	}
}

static int amdgpu_vm_wait_pd(struct amdgpu_device *adev, struct amdgpu_vm *vm,
			     void *owner)
{
	struct amdgpu_sync sync;
	int r;

	amdgpu_sync_create(&sync);
	amdgpu_sync_resv(adev, &sync, vm->root.base.bo->tbo.resv, owner);
	r = amdgpu_sync_wait(&sync, true);
	amdgpu_sync_free(&sync);

	return r;
}

/*
 * amdgpu_vm_update_level - update a single level in the hierarchy
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @parent: parent directory
 *
 * Makes sure all entries in @parent are up to date.
 * Returns 0 for success, error for failure.
 */
static int amdgpu_vm_update_level(struct amdgpu_device *adev,
				  struct amdgpu_vm *vm,
				  struct amdgpu_vm_pt *parent)
{
	struct amdgpu_bo *shadow;
	struct amdgpu_ring *ring = NULL;
	uint64_t pd_addr, shadow_addr = 0;
	uint64_t last_pde = ~0, last_pt = ~0, last_shadow = ~0;
	unsigned count = 0, pt_idx, ndw = 0;
	struct amdgpu_job *job;
	struct amdgpu_pte_update_params params;
	struct dma_fence *fence = NULL;
	uint32_t incr;

	int r;

	if (!parent->entries)
		return 0;

	memset(&params, 0, sizeof(params));
	params.adev = adev;
	shadow = parent->base.bo->shadow;

	if (vm->use_cpu_for_update) {
		pd_addr = (unsigned long)amdgpu_bo_kptr(parent->base.bo);
		r = amdgpu_vm_wait_pd(adev, vm, AMDGPU_FENCE_OWNER_VM);
		if (unlikely(r))
			return r;

		params.func = amdgpu_vm_cpu_set_ptes;
	} else {
		ring = container_of(vm->entity.sched, struct amdgpu_ring,
				    sched);

		/* padding, etc. */
		ndw = 64;

		/* assume the worst case */
		ndw += parent->last_entry_used * 6;

		pd_addr = amdgpu_bo_gpu_offset(parent->base.bo);

		if (shadow) {
			shadow_addr = amdgpu_bo_gpu_offset(shadow);
			ndw *= 2;
		} else {
			shadow_addr = 0;
		}

		r = amdgpu_job_alloc_with_ib(adev, ndw * 4, &job);
		if (r)
			return r;

		params.ib = &job->ibs[0];
		params.func = amdgpu_vm_do_set_ptes;
	}


	/* walk over the address space and update the directory */
	for (pt_idx = 0; pt_idx <= parent->last_entry_used; ++pt_idx) {
		struct amdgpu_vm_pt *entry = &parent->entries[pt_idx];
		struct amdgpu_bo *bo = entry->base.bo;
		uint64_t pde, pt;

		if (bo == NULL)
			continue;

		spin_lock(&vm->status_lock);
		list_del_init(&entry->base.vm_status);
		spin_unlock(&vm->status_lock);

		pt = amdgpu_bo_gpu_offset(bo);
		pt = amdgpu_gart_get_vm_pde(adev, pt);
		/* Don't update huge pages here */
		if ((parent->entries[pt_idx].addr & AMDGPU_PDE_PTE) ||
		    parent->entries[pt_idx].addr == (pt | AMDGPU_PTE_VALID))
			continue;

		parent->entries[pt_idx].addr = pt | AMDGPU_PTE_VALID;

		pde = pd_addr + pt_idx * 8;
		incr = amdgpu_bo_size(bo);
		if (((last_pde + 8 * count) != pde) ||
		    ((last_pt + incr * count) != pt) ||
		    (count == AMDGPU_VM_MAX_UPDATE_SIZE)) {

			if (count) {
				if (shadow)
					params.func(&params,
						    last_shadow,
						    last_pt, count,
						    incr,
						    AMDGPU_PTE_VALID);

				params.func(&params, last_pde,
					    last_pt, count, incr,
					    AMDGPU_PTE_VALID);
			}

			count = 1;
			last_pde = pde;
			last_shadow = shadow_addr + pt_idx * 8;
			last_pt = pt;
		} else {
			++count;
		}
	}

	if (count) {
		if (vm->root.base.bo->shadow)
			params.func(&params, last_shadow, last_pt,
				    count, incr, AMDGPU_PTE_VALID);

		params.func(&params, last_pde, last_pt,
			    count, incr, AMDGPU_PTE_VALID);
	}

	if (!vm->use_cpu_for_update) {
		if (params.ib->length_dw == 0) {
			amdgpu_job_free(job);
		} else {
			amdgpu_ring_pad_ib(ring, params.ib);
			amdgpu_sync_resv(adev, &job->sync,
					 parent->base.bo->tbo.resv,
					 AMDGPU_FENCE_OWNER_VM);
			if (shadow)
				amdgpu_sync_resv(adev, &job->sync,
						 shadow->tbo.resv,
						 AMDGPU_FENCE_OWNER_VM);

			WARN_ON(params.ib->length_dw > ndw);
			r = amdgpu_job_submit(job, ring, &vm->entity,
					AMDGPU_FENCE_OWNER_VM, &fence);
			if (r)
				goto error_free;

			amdgpu_bo_fence(parent->base.bo, fence, true);
			dma_fence_put(vm->last_update);
			vm->last_update = fence;
		}
	}

	return 0;

error_free:
	amdgpu_job_free(job);
	return r;
}

/*
 * amdgpu_vm_invalidate_level - mark all PD levels as invalid
 *
 * @parent: parent PD
 *
 * Mark all PD level as invalid after an error.
 */
static void amdgpu_vm_invalidate_level(struct amdgpu_vm *vm,
				       struct amdgpu_vm_pt *parent)
{
	unsigned pt_idx;

	/*
	 * Recurse into the subdirectories. This recursion is harmless because
	 * we only have a maximum of 5 layers.
	 */
	for (pt_idx = 0; pt_idx <= parent->last_entry_used; ++pt_idx) {
		struct amdgpu_vm_pt *entry = &parent->entries[pt_idx];

		if (!entry->base.bo)
			continue;

		entry->addr = ~0ULL;
		spin_lock(&vm->status_lock);
		if (list_empty(&entry->base.vm_status))
			list_add(&entry->base.vm_status, &vm->relocated);
		spin_unlock(&vm->status_lock);
		amdgpu_vm_invalidate_level(vm, entry);
	}
}

/*
 * amdgpu_vm_update_directories - make sure that all directories are valid
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 *
 * Makes sure all directories are up to date.
 * Returns 0 for success, error for failure.
 */
int amdgpu_vm_update_directories(struct amdgpu_device *adev,
				 struct amdgpu_vm *vm)
{
	int r;

	spin_lock(&vm->status_lock);
	while (!list_empty(&vm->relocated)) {
		struct amdgpu_vm_bo_base *bo_base;
		struct amdgpu_bo *bo;

		bo_base = list_first_entry(&vm->relocated,
					   struct amdgpu_vm_bo_base,
					   vm_status);
		spin_unlock(&vm->status_lock);

		bo = bo_base->bo->parent;
		if (bo) {
			struct amdgpu_vm_bo_base *parent;
			struct amdgpu_vm_pt *pt;

			parent = list_first_entry(&bo->va,
						  struct amdgpu_vm_bo_base,
						  bo_list);
			pt = container_of(parent, struct amdgpu_vm_pt, base);

			r = amdgpu_vm_update_level(adev, vm, pt);
			if (r) {
				amdgpu_vm_invalidate_level(vm, &vm->root);
				return r;
			}
			spin_lock(&vm->status_lock);
		} else {
			spin_lock(&vm->status_lock);
			list_del_init(&bo_base->vm_status);
		}
	}
	spin_unlock(&vm->status_lock);

	if (vm->use_cpu_for_update) {
		/* Flush HDP */
		mb();
		amdgpu_gart_flush_gpu_tlb(adev, 0);
	}

	return r;
}

/**
 * amdgpu_vm_find_entry - find the entry for an address
 *
 * @p: see amdgpu_pte_update_params definition
 * @addr: virtual address in question
 * @entry: resulting entry or NULL
 * @parent: parent entry
 *
 * Find the vm_pt entry and it's parent for the given address.
 */
void amdgpu_vm_get_entry(struct amdgpu_pte_update_params *p, uint64_t addr,
			 struct amdgpu_vm_pt **entry,
			 struct amdgpu_vm_pt **parent)
{
	unsigned idx, level = p->adev->vm_manager.num_level;

	*parent = NULL;
	*entry = &p->vm->root;
	while ((*entry)->entries) {
		idx = addr >> (p->adev->vm_manager.block_size * level--);
		idx %= amdgpu_bo_size((*entry)->base.bo) / 8;
		*parent = *entry;
		*entry = &(*entry)->entries[idx];
	}

	if (level)
		*entry = NULL;
}

/**
 * amdgpu_vm_handle_huge_pages - handle updating the PD with huge pages
 *
 * @p: see amdgpu_pte_update_params definition
 * @entry: vm_pt entry to check
 * @parent: parent entry
 * @nptes: number of PTEs updated with this operation
 * @dst: destination address where the PTEs should point to
 * @flags: access flags fro the PTEs
 *
 * Check if we can update the PD with a huge page.
 */
static void amdgpu_vm_handle_huge_pages(struct amdgpu_pte_update_params *p,
					struct amdgpu_vm_pt *entry,
					struct amdgpu_vm_pt *parent,
					unsigned nptes, uint64_t dst,
					uint64_t flags)
{
	bool use_cpu_update = (p->func == amdgpu_vm_cpu_set_ptes);
	uint64_t pd_addr, pde;

	/* In the case of a mixed PT the PDE must point to it*/
	if (p->adev->asic_type < CHIP_VEGA10 ||
	    nptes != AMDGPU_VM_PTE_COUNT(p->adev) ||
	    p->src ||
	    !(flags & AMDGPU_PTE_VALID)) {

		dst = amdgpu_bo_gpu_offset(entry->base.bo);
		dst = amdgpu_gart_get_vm_pde(p->adev, dst);
		flags = AMDGPU_PTE_VALID;
	} else {
		/* Set the huge page flag to stop scanning at this PDE */
		flags |= AMDGPU_PDE_PTE;
	}

	if (entry->addr == (dst | flags))
		return;

	entry->addr = (dst | flags);

	if (use_cpu_update) {
		/* In case a huge page is replaced with a system
		 * memory mapping, p->pages_addr != NULL and
		 * amdgpu_vm_cpu_set_ptes would try to translate dst
		 * through amdgpu_vm_map_gart. But dst is already a
		 * GPU address (of the page table). Disable
		 * amdgpu_vm_map_gart temporarily.
		 */
		dma_addr_t *tmp;

		tmp = p->pages_addr;
		p->pages_addr = NULL;

		pd_addr = (unsigned long)amdgpu_bo_kptr(parent->base.bo);
		pde = pd_addr + (entry - parent->entries) * 8;
		amdgpu_vm_cpu_set_ptes(p, pde, dst, 1, 0, flags);

		p->pages_addr = tmp;
	} else {
		if (parent->base.bo->shadow) {
			pd_addr = amdgpu_bo_gpu_offset(parent->base.bo->shadow);
			pde = pd_addr + (entry - parent->entries) * 8;
			amdgpu_vm_do_set_ptes(p, pde, dst, 1, 0, flags);
		}
		pd_addr = amdgpu_bo_gpu_offset(parent->base.bo);
		pde = pd_addr + (entry - parent->entries) * 8;
		amdgpu_vm_do_set_ptes(p, pde, dst, 1, 0, flags);
	}
}

/**
 * amdgpu_vm_update_ptes - make sure that page tables are valid
 *
 * @params: see amdgpu_pte_update_params definition
 * @vm: requested vm
 * @start: start of GPU address range
 * @end: end of GPU address range
 * @dst: destination address to map to, the next dst inside the function
 * @flags: mapping flags
 *
 * Update the page tables in the range @start - @end.
 * Returns 0 for success, -EINVAL for failure.
 */
static int amdgpu_vm_update_ptes(struct amdgpu_pte_update_params *params,
				  uint64_t start, uint64_t end,
				  uint64_t dst, uint64_t flags)
{
	struct amdgpu_device *adev = params->adev;
	const uint64_t mask = AMDGPU_VM_PTE_COUNT(adev) - 1;

	uint64_t addr, pe_start;
	struct amdgpu_bo *pt;
	unsigned nptes;
	bool use_cpu_update = (params->func == amdgpu_vm_cpu_set_ptes);

	/* walk over the address space and update the page tables */
	for (addr = start; addr < end; addr += nptes,
	     dst += nptes * AMDGPU_GPU_PAGE_SIZE) {
		struct amdgpu_vm_pt *entry, *parent;

		amdgpu_vm_get_entry(params, addr, &entry, &parent);
		if (!entry)
			return -ENOENT;

		if ((addr & ~mask) == (end & ~mask))
			nptes = end - addr;
		else
			nptes = AMDGPU_VM_PTE_COUNT(adev) - (addr & mask);

		amdgpu_vm_handle_huge_pages(params, entry, parent,
					    nptes, dst, flags);
		/* We don't need to update PTEs for huge pages */
		if (entry->addr & AMDGPU_PDE_PTE)
			continue;

		pt = entry->base.bo;
		if (use_cpu_update) {
			pe_start = (unsigned long)amdgpu_bo_kptr(pt);
		} else {
			if (pt->shadow) {
				pe_start = amdgpu_bo_gpu_offset(pt->shadow);
				pe_start += (addr & mask) * 8;
				params->func(params, pe_start, dst, nptes,
					     AMDGPU_GPU_PAGE_SIZE, flags);
			}
			pe_start = amdgpu_bo_gpu_offset(pt);
		}

		pe_start += (addr & mask) * 8;
		params->func(params, pe_start, dst, nptes,
			     AMDGPU_GPU_PAGE_SIZE, flags);
	}

	return 0;
}

/*
 * amdgpu_vm_frag_ptes - add fragment information to PTEs
 *
 * @params: see amdgpu_pte_update_params definition
 * @vm: requested vm
 * @start: first PTE to handle
 * @end: last PTE to handle
 * @dst: addr those PTEs should point to
 * @flags: hw mapping flags
 * Returns 0 for success, -EINVAL for failure.
 */
static int amdgpu_vm_frag_ptes(struct amdgpu_pte_update_params	*params,
				uint64_t start, uint64_t end,
				uint64_t dst, uint64_t flags)
{
	/**
	 * The MC L1 TLB supports variable sized pages, based on a fragment
	 * field in the PTE. When this field is set to a non-zero value, page
	 * granularity is increased from 4KB to (1 << (12 + frag)). The PTE
	 * flags are considered valid for all PTEs within the fragment range
	 * and corresponding mappings are assumed to be physically contiguous.
	 *
	 * The L1 TLB can store a single PTE for the whole fragment,
	 * significantly increasing the space available for translation
	 * caching. This leads to large improvements in throughput when the
	 * TLB is under pressure.
	 *
	 * The L2 TLB distributes small and large fragments into two
	 * asymmetric partitions. The large fragment cache is significantly
	 * larger. Thus, we try to use large fragments wherever possible.
	 * Userspace can support this by aligning virtual base address and
	 * allocation size to the fragment size.
	 */
	unsigned max_frag = params->adev->vm_manager.fragment_size;
	int r;

	/* system pages are non continuously */
	if (params->src || !(flags & AMDGPU_PTE_VALID))
		return amdgpu_vm_update_ptes(params, start, end, dst, flags);

	while (start != end) {
		uint64_t frag_flags, frag_end;
		unsigned frag;

		/* This intentionally wraps around if no bit is set */
		frag = min((unsigned)ffs(start) - 1,
			   (unsigned)fls64(end - start) - 1);
		if (frag >= max_frag) {
			frag_flags = AMDGPU_PTE_FRAG(max_frag);
			frag_end = end & ~((1ULL << max_frag) - 1);
		} else {
			frag_flags = AMDGPU_PTE_FRAG(frag);
			frag_end = start + (1 << frag);
		}

		r = amdgpu_vm_update_ptes(params, start, frag_end, dst,
					  flags | frag_flags);
		if (r)
			return r;

		dst += (frag_end - start) * AMDGPU_GPU_PAGE_SIZE;
		start = frag_end;
	}

	return 0;
}

/**
 * amdgpu_vm_bo_update_mapping - update a mapping in the vm page table
 *
 * @adev: amdgpu_device pointer
 * @exclusive: fence we need to sync to
 * @pages_addr: DMA addresses to use for mapping
 * @vm: requested vm
 * @start: start of mapped range
 * @last: last mapped entry
 * @flags: flags for the entries
 * @addr: addr to set the area to
 * @fence: optional resulting fence
 *
 * Fill in the page table entries between @start and @last.
 * Returns 0 for success, -EINVAL for failure.
 */
static int amdgpu_vm_bo_update_mapping(struct amdgpu_device *adev,
				       struct dma_fence *exclusive,
				       dma_addr_t *pages_addr,
				       struct amdgpu_vm *vm,
				       uint64_t start, uint64_t last,
				       uint64_t flags, uint64_t addr,
				       struct dma_fence **fence)
{
	struct amdgpu_ring *ring;
	void *owner = AMDGPU_FENCE_OWNER_VM;
	unsigned nptes, ncmds, ndw;
	struct amdgpu_job *job;
	struct amdgpu_pte_update_params params;
	struct dma_fence *f = NULL;
	int r;

	memset(&params, 0, sizeof(params));
	params.adev = adev;
	params.vm = vm;

	/* sync to everything on unmapping */
	if (!(flags & AMDGPU_PTE_VALID))
		owner = AMDGPU_FENCE_OWNER_UNDEFINED;

	if (vm->use_cpu_for_update) {
		/* params.src is used as flag to indicate system Memory */
		if (pages_addr)
			params.src = ~0;

		/* Wait for PT BOs to be free. PTs share the same resv. object
		 * as the root PD BO
		 */
		r = amdgpu_vm_wait_pd(adev, vm, owner);
		if (unlikely(r))
			return r;

		params.func = amdgpu_vm_cpu_set_ptes;
		params.pages_addr = pages_addr;
		return amdgpu_vm_frag_ptes(&params, start, last + 1,
					   addr, flags);
	}

	ring = container_of(vm->entity.sched, struct amdgpu_ring, sched);

	nptes = last - start + 1;

	/*
	 * reserve space for two commands every (1 << BLOCK_SIZE)
	 *  entries or 2k dwords (whatever is smaller)
         *
         * The second command is for the shadow pagetables.
	 */
	ncmds = ((nptes >> min(adev->vm_manager.block_size, 11u)) + 1) * 2;

	/* padding, etc. */
	ndw = 64;

	/* one PDE write for each huge page */
	ndw += ((nptes >> adev->vm_manager.block_size) + 1) * 6;

	if (pages_addr) {
		/* copy commands needed */
		ndw += ncmds * 7;

		/* and also PTEs */
		ndw += nptes * 2;

		params.func = amdgpu_vm_do_copy_ptes;

	} else {
		/* set page commands needed */
		ndw += ncmds * 10;

		/* extra commands for begin/end fragments */
		ndw += 2 * 10 * adev->vm_manager.fragment_size;

		params.func = amdgpu_vm_do_set_ptes;
	}

	r = amdgpu_job_alloc_with_ib(adev, ndw * 4, &job);
	if (r)
		return r;

	params.ib = &job->ibs[0];

	if (pages_addr) {
		uint64_t *pte;
		unsigned i;

		/* Put the PTEs at the end of the IB. */
		i = ndw - nptes * 2;
		pte= (uint64_t *)&(job->ibs->ptr[i]);
		params.src = job->ibs->gpu_addr + i * 4;

		for (i = 0; i < nptes; ++i) {
			pte[i] = amdgpu_vm_map_gart(pages_addr, addr + i *
						    AMDGPU_GPU_PAGE_SIZE);
			pte[i] |= flags;
		}
		addr = 0;
	}

	r = amdgpu_sync_fence(adev, &job->sync, exclusive);
	if (r)
		goto error_free;

	r = amdgpu_sync_resv(adev, &job->sync, vm->root.base.bo->tbo.resv,
			     owner);
	if (r)
		goto error_free;

	r = reservation_object_reserve_shared(vm->root.base.bo->tbo.resv);
	if (r)
		goto error_free;

	r = amdgpu_vm_frag_ptes(&params, start, last + 1, addr, flags);
	if (r)
		goto error_free;

	amdgpu_ring_pad_ib(ring, params.ib);
	WARN_ON(params.ib->length_dw > ndw);
	r = amdgpu_job_submit(job, ring, &vm->entity,
			      AMDGPU_FENCE_OWNER_VM, &f);
	if (r)
		goto error_free;

	amdgpu_bo_fence(vm->root.base.bo, f, true);
	dma_fence_put(*fence);
	*fence = f;
	return 0;

error_free:
	amdgpu_job_free(job);
	amdgpu_vm_invalidate_level(vm, &vm->root);
	return r;
}

/**
 * amdgpu_vm_bo_split_mapping - split a mapping into smaller chunks
 *
 * @adev: amdgpu_device pointer
 * @exclusive: fence we need to sync to
 * @pages_addr: DMA addresses to use for mapping
 * @vm: requested vm
 * @mapping: mapped range and flags to use for the update
 * @flags: HW flags for the mapping
 * @nodes: array of drm_mm_nodes with the MC addresses
 * @fence: optional resulting fence
 *
 * Split the mapping into smaller chunks so that each update fits
 * into a SDMA IB.
 * Returns 0 for success, -EINVAL for failure.
 */
static int amdgpu_vm_bo_split_mapping(struct amdgpu_device *adev,
				      struct dma_fence *exclusive,
				      dma_addr_t *pages_addr,
				      struct amdgpu_vm *vm,
				      struct amdgpu_bo_va_mapping *mapping,
				      uint64_t flags,
				      struct drm_mm_node *nodes,
				      struct dma_fence **fence)
{
	uint64_t pfn, start = mapping->start;
	int r;

	/* normally,bo_va->flags only contians READABLE and WIRTEABLE bit go here
	 * but in case of something, we filter the flags in first place
	 */
	if (!(mapping->flags & AMDGPU_PTE_READABLE))
		flags &= ~AMDGPU_PTE_READABLE;
	if (!(mapping->flags & AMDGPU_PTE_WRITEABLE))
		flags &= ~AMDGPU_PTE_WRITEABLE;

	flags &= ~AMDGPU_PTE_EXECUTABLE;
	flags |= mapping->flags & AMDGPU_PTE_EXECUTABLE;

	flags &= ~AMDGPU_PTE_MTYPE_MASK;
	flags |= (mapping->flags & AMDGPU_PTE_MTYPE_MASK);

	if ((mapping->flags & AMDGPU_PTE_PRT) &&
	    (adev->asic_type >= CHIP_VEGA10)) {
		flags |= AMDGPU_PTE_PRT;
		flags &= ~AMDGPU_PTE_VALID;
	}

	trace_amdgpu_vm_bo_update(mapping);

	pfn = mapping->offset >> PAGE_SHIFT;
	if (nodes) {
		while (pfn >= nodes->size) {
			pfn -= nodes->size;
			++nodes;
		}
	}

	do {
		uint64_t max_entries;
		uint64_t addr, last;

		if (nodes) {
			addr = nodes->start << PAGE_SHIFT;
			max_entries = (nodes->size - pfn) *
				(PAGE_SIZE / AMDGPU_GPU_PAGE_SIZE);
		} else {
			addr = 0;
			max_entries = S64_MAX;
		}

		if (pages_addr) {
			max_entries = min(max_entries, 16ull * 1024ull);
			addr = 0;
		} else if (flags & AMDGPU_PTE_VALID) {
			addr += adev->vm_manager.vram_base_offset;
		}
		addr += pfn << PAGE_SHIFT;

		last = min((uint64_t)mapping->last, start + max_entries - 1);
		r = amdgpu_vm_bo_update_mapping(adev, exclusive, pages_addr, vm,
						start, last, flags, addr,
						fence);
		if (r)
			return r;

		pfn += last - start + 1;
		if (nodes && nodes->size == pfn) {
			pfn = 0;
			++nodes;
		}
		start = last + 1;

	} while (unlikely(start != mapping->last + 1));

	return 0;
}

/**
 * amdgpu_vm_bo_update - update all BO mappings in the vm page table
 *
 * @adev: amdgpu_device pointer
 * @bo_va: requested BO and VM object
 * @clear: if true clear the entries
 *
 * Fill in the page table entries for @bo_va.
 * Returns 0 for success, -EINVAL for failure.
 */
int amdgpu_vm_bo_update(struct amdgpu_device *adev,
			struct amdgpu_bo_va *bo_va,
			bool clear)
{
	struct amdgpu_bo *bo = bo_va->base.bo;
	struct amdgpu_vm *vm = bo_va->base.vm;
	struct amdgpu_bo_va_mapping *mapping;
	dma_addr_t *pages_addr = NULL;
	struct ttm_mem_reg *mem;
	struct drm_mm_node *nodes;
	struct dma_fence *exclusive, **last_update;
	uint64_t flags;
	int r;

	if (clear || !bo_va->base.bo) {
		mem = NULL;
		nodes = NULL;
		exclusive = NULL;
	} else {
		struct ttm_dma_tt *ttm;

		mem = &bo_va->base.bo->tbo.mem;
		nodes = mem->mm_node;
		if (mem->mem_type == TTM_PL_TT) {
			ttm = container_of(bo_va->base.bo->tbo.ttm,
					   struct ttm_dma_tt, ttm);
			pages_addr = ttm->dma_address;
		}
		exclusive = reservation_object_get_excl(bo->tbo.resv);
	}

	if (bo)
		flags = amdgpu_ttm_tt_pte_flags(adev, bo->tbo.ttm, mem);
	else
		flags = 0x0;

	if (clear || (bo && bo->tbo.resv == vm->root.base.bo->tbo.resv))
		last_update = &vm->last_update;
	else
		last_update = &bo_va->last_pt_update;

	if (!clear && bo_va->base.moved) {
		bo_va->base.moved = false;
		list_splice_init(&bo_va->valids, &bo_va->invalids);

	} else if (bo_va->cleared != clear) {
		list_splice_init(&bo_va->valids, &bo_va->invalids);
	}

	list_for_each_entry(mapping, &bo_va->invalids, list) {
		r = amdgpu_vm_bo_split_mapping(adev, exclusive, pages_addr, vm,
					       mapping, flags, nodes,
					       last_update);
		if (r)
			return r;
	}

	if (vm->use_cpu_for_update) {
		/* Flush HDP */
		mb();
		amdgpu_gart_flush_gpu_tlb(adev, 0);
	}

	spin_lock(&vm->status_lock);
	list_del_init(&bo_va->base.vm_status);
	spin_unlock(&vm->status_lock);

	list_splice_init(&bo_va->invalids, &bo_va->valids);
	bo_va->cleared = clear;

	if (trace_amdgpu_vm_bo_mapping_enabled()) {
		list_for_each_entry(mapping, &bo_va->valids, list)
			trace_amdgpu_vm_bo_mapping(mapping);
	}

	return 0;
}

/**
 * amdgpu_vm_update_prt_state - update the global PRT state
 */
static void amdgpu_vm_update_prt_state(struct amdgpu_device *adev)
{
	unsigned long flags;
	bool enable;

	spin_lock_irqsave(&adev->vm_manager.prt_lock, flags);
	enable = !!atomic_read(&adev->vm_manager.num_prt_users);
	adev->gart.gart_funcs->set_prt(adev, enable);
	spin_unlock_irqrestore(&adev->vm_manager.prt_lock, flags);
}

/**
 * amdgpu_vm_prt_get - add a PRT user
 */
static void amdgpu_vm_prt_get(struct amdgpu_device *adev)
{
	if (!adev->gart.gart_funcs->set_prt)
		return;

	if (atomic_inc_return(&adev->vm_manager.num_prt_users) == 1)
		amdgpu_vm_update_prt_state(adev);
}

/**
 * amdgpu_vm_prt_put - drop a PRT user
 */
static void amdgpu_vm_prt_put(struct amdgpu_device *adev)
{
	if (atomic_dec_return(&adev->vm_manager.num_prt_users) == 0)
		amdgpu_vm_update_prt_state(adev);
}

/**
 * amdgpu_vm_prt_cb - callback for updating the PRT status
 */
static void amdgpu_vm_prt_cb(struct dma_fence *fence, struct dma_fence_cb *_cb)
{
	struct amdgpu_prt_cb *cb = container_of(_cb, struct amdgpu_prt_cb, cb);

	amdgpu_vm_prt_put(cb->adev);
	kfree(cb);
}

/**
 * amdgpu_vm_add_prt_cb - add callback for updating the PRT status
 */
static void amdgpu_vm_add_prt_cb(struct amdgpu_device *adev,
				 struct dma_fence *fence)
{
	struct amdgpu_prt_cb *cb;

	if (!adev->gart.gart_funcs->set_prt)
		return;

	cb = kmalloc(sizeof(struct amdgpu_prt_cb), GFP_KERNEL);
	if (!cb) {
		/* Last resort when we are OOM */
		if (fence)
			dma_fence_wait(fence, false);

		amdgpu_vm_prt_put(adev);
	} else {
		cb->adev = adev;
		if (!fence || dma_fence_add_callback(fence, &cb->cb,
						     amdgpu_vm_prt_cb))
			amdgpu_vm_prt_cb(fence, &cb->cb);
	}
}

/**
 * amdgpu_vm_free_mapping - free a mapping
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @mapping: mapping to be freed
 * @fence: fence of the unmap operation
 *
 * Free a mapping and make sure we decrease the PRT usage count if applicable.
 */
static void amdgpu_vm_free_mapping(struct amdgpu_device *adev,
				   struct amdgpu_vm *vm,
				   struct amdgpu_bo_va_mapping *mapping,
				   struct dma_fence *fence)
{
	if (mapping->flags & AMDGPU_PTE_PRT)
		amdgpu_vm_add_prt_cb(adev, fence);
	kfree(mapping);
}

/**
 * amdgpu_vm_prt_fini - finish all prt mappings
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 *
 * Register a cleanup callback to disable PRT support after VM dies.
 */
static void amdgpu_vm_prt_fini(struct amdgpu_device *adev, struct amdgpu_vm *vm)
{
	struct reservation_object *resv = vm->root.base.bo->tbo.resv;
	struct dma_fence *excl, **shared;
	unsigned i, shared_count;
	int r;

	r = reservation_object_get_fences_rcu(resv, &excl,
					      &shared_count, &shared);
	if (r) {
		/* Not enough memory to grab the fence list, as last resort
		 * block for all the fences to complete.
		 */
		reservation_object_wait_timeout_rcu(resv, true, false,
						    MAX_SCHEDULE_TIMEOUT);
		return;
	}

	/* Add a callback for each fence in the reservation object */
	amdgpu_vm_prt_get(adev);
	amdgpu_vm_add_prt_cb(adev, excl);

	for (i = 0; i < shared_count; ++i) {
		amdgpu_vm_prt_get(adev);
		amdgpu_vm_add_prt_cb(adev, shared[i]);
	}

	kfree(shared);
}

/**
 * amdgpu_vm_clear_freed - clear freed BOs in the PT
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @fence: optional resulting fence (unchanged if no work needed to be done
 * or if an error occurred)
 *
 * Make sure all freed BOs are cleared in the PT.
 * Returns 0 for success.
 *
 * PTs have to be reserved and mutex must be locked!
 */
int amdgpu_vm_clear_freed(struct amdgpu_device *adev,
			  struct amdgpu_vm *vm,
			  struct dma_fence **fence)
{
	struct amdgpu_bo_va_mapping *mapping;
	struct dma_fence *f = NULL;
	int r;
	uint64_t init_pte_value = 0;

	while (!list_empty(&vm->freed)) {
		mapping = list_first_entry(&vm->freed,
			struct amdgpu_bo_va_mapping, list);
		list_del(&mapping->list);

		if (vm->pte_support_ats)
			init_pte_value = AMDGPU_PTE_SYSTEM;

		r = amdgpu_vm_bo_update_mapping(adev, NULL, NULL, vm,
						mapping->start, mapping->last,
						init_pte_value, 0, &f);
		amdgpu_vm_free_mapping(adev, vm, mapping, f);
		if (r) {
			dma_fence_put(f);
			return r;
		}
	}

	if (fence && f) {
		dma_fence_put(*fence);
		*fence = f;
	} else {
		dma_fence_put(f);
	}

	return 0;

}

/**
 * amdgpu_vm_handle_moved - handle moved BOs in the PT
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @sync: sync object to add fences to
 *
 * Make sure all BOs which are moved are updated in the PTs.
 * Returns 0 for success.
 *
 * PTs have to be reserved!
 */
int amdgpu_vm_handle_moved(struct amdgpu_device *adev,
			   struct amdgpu_vm *vm)
{
	bool clear;
	int r = 0;

	spin_lock(&vm->status_lock);
	while (!list_empty(&vm->moved)) {
		struct amdgpu_bo_va *bo_va;

		bo_va = list_first_entry(&vm->moved,
			struct amdgpu_bo_va, base.vm_status);
		spin_unlock(&vm->status_lock);

		/* Per VM BOs never need to bo cleared in the page tables */
		clear = bo_va->base.bo->tbo.resv != vm->root.base.bo->tbo.resv;

		r = amdgpu_vm_bo_update(adev, bo_va, clear);
		if (r)
			return r;

		spin_lock(&vm->status_lock);
	}
	spin_unlock(&vm->status_lock);

	return r;
}

/**
 * amdgpu_vm_bo_add - add a bo to a specific vm
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @bo: amdgpu buffer object
 *
 * Add @bo into the requested vm.
 * Add @bo to the list of bos associated with the vm
 * Returns newly added bo_va or NULL for failure
 *
 * Object has to be reserved!
 */
struct amdgpu_bo_va *amdgpu_vm_bo_add(struct amdgpu_device *adev,
				      struct amdgpu_vm *vm,
				      struct amdgpu_bo *bo)
{
	struct amdgpu_bo_va *bo_va;

	bo_va = kzalloc(sizeof(struct amdgpu_bo_va), GFP_KERNEL);
	if (bo_va == NULL) {
		return NULL;
	}
	bo_va->base.vm = vm;
	bo_va->base.bo = bo;
	INIT_LIST_HEAD(&bo_va->base.bo_list);
	INIT_LIST_HEAD(&bo_va->base.vm_status);

	bo_va->ref_count = 1;
	INIT_LIST_HEAD(&bo_va->valids);
	INIT_LIST_HEAD(&bo_va->invalids);

	if (bo)
		list_add_tail(&bo_va->base.bo_list, &bo->va);

	return bo_va;
}


/**
 * amdgpu_vm_bo_insert_mapping - insert a new mapping
 *
 * @adev: amdgpu_device pointer
 * @bo_va: bo_va to store the address
 * @mapping: the mapping to insert
 *
 * Insert a new mapping into all structures.
 */
static void amdgpu_vm_bo_insert_map(struct amdgpu_device *adev,
				    struct amdgpu_bo_va *bo_va,
				    struct amdgpu_bo_va_mapping *mapping)
{
	struct amdgpu_vm *vm = bo_va->base.vm;
	struct amdgpu_bo *bo = bo_va->base.bo;

	mapping->bo_va = bo_va;
	list_add(&mapping->list, &bo_va->invalids);
	amdgpu_vm_it_insert(mapping, &vm->va);

	if (mapping->flags & AMDGPU_PTE_PRT)
		amdgpu_vm_prt_get(adev);

	if (bo && bo->tbo.resv == vm->root.base.bo->tbo.resv) {
		spin_lock(&vm->status_lock);
		if (list_empty(&bo_va->base.vm_status))
			list_add(&bo_va->base.vm_status, &vm->moved);
		spin_unlock(&vm->status_lock);
	}
	trace_amdgpu_vm_bo_map(bo_va, mapping);
}

/**
 * amdgpu_vm_bo_map - map bo inside a vm
 *
 * @adev: amdgpu_device pointer
 * @bo_va: bo_va to store the address
 * @saddr: where to map the BO
 * @offset: requested offset in the BO
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Add a mapping of the BO at the specefied addr into the VM.
 * Returns 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int amdgpu_vm_bo_map(struct amdgpu_device *adev,
		     struct amdgpu_bo_va *bo_va,
		     uint64_t saddr, uint64_t offset,
		     uint64_t size, uint64_t flags)
{
	struct amdgpu_bo_va_mapping *mapping, *tmp;
	struct amdgpu_bo *bo = bo_va->base.bo;
	struct amdgpu_vm *vm = bo_va->base.vm;
	uint64_t eaddr;

	/* validate the parameters */
	if (saddr & AMDGPU_GPU_PAGE_MASK || offset & AMDGPU_GPU_PAGE_MASK ||
	    size == 0 || size & AMDGPU_GPU_PAGE_MASK)
		return -EINVAL;

	/* make sure object fit at this offset */
	eaddr = saddr + size - 1;
	if (saddr >= eaddr ||
	    (bo && offset + size > amdgpu_bo_size(bo)))
		return -EINVAL;

	saddr /= AMDGPU_GPU_PAGE_SIZE;
	eaddr /= AMDGPU_GPU_PAGE_SIZE;

	tmp = amdgpu_vm_it_iter_first(&vm->va, saddr, eaddr);
	if (tmp) {
		/* bo and tmp overlap, invalid addr */
		dev_err(adev->dev, "bo %p va 0x%010Lx-0x%010Lx conflict with "
			"0x%010Lx-0x%010Lx\n", bo, saddr, eaddr,
			tmp->start, tmp->last + 1);
		return -EINVAL;
	}

	mapping = kmalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	mapping->start = saddr;
	mapping->last = eaddr;
	mapping->offset = offset;
	mapping->flags = flags;

	amdgpu_vm_bo_insert_map(adev, bo_va, mapping);

	return 0;
}

/**
 * amdgpu_vm_bo_replace_map - map bo inside a vm, replacing existing mappings
 *
 * @adev: amdgpu_device pointer
 * @bo_va: bo_va to store the address
 * @saddr: where to map the BO
 * @offset: requested offset in the BO
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Add a mapping of the BO at the specefied addr into the VM. Replace existing
 * mappings as we do so.
 * Returns 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int amdgpu_vm_bo_replace_map(struct amdgpu_device *adev,
			     struct amdgpu_bo_va *bo_va,
			     uint64_t saddr, uint64_t offset,
			     uint64_t size, uint64_t flags)
{
	struct amdgpu_bo_va_mapping *mapping;
	struct amdgpu_bo *bo = bo_va->base.bo;
	uint64_t eaddr;
	int r;

	/* validate the parameters */
	if (saddr & AMDGPU_GPU_PAGE_MASK || offset & AMDGPU_GPU_PAGE_MASK ||
	    size == 0 || size & AMDGPU_GPU_PAGE_MASK)
		return -EINVAL;

	/* make sure object fit at this offset */
	eaddr = saddr + size - 1;
	if (saddr >= eaddr ||
	    (bo && offset + size > amdgpu_bo_size(bo)))
		return -EINVAL;

	/* Allocate all the needed memory */
	mapping = kmalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	r = amdgpu_vm_bo_clear_mappings(adev, bo_va->base.vm, saddr, size);
	if (r) {
		kfree(mapping);
		return r;
	}

	saddr /= AMDGPU_GPU_PAGE_SIZE;
	eaddr /= AMDGPU_GPU_PAGE_SIZE;

	mapping->start = saddr;
	mapping->last = eaddr;
	mapping->offset = offset;
	mapping->flags = flags;

	amdgpu_vm_bo_insert_map(adev, bo_va, mapping);

	return 0;
}

/**
 * amdgpu_vm_bo_unmap - remove bo mapping from vm
 *
 * @adev: amdgpu_device pointer
 * @bo_va: bo_va to remove the address from
 * @saddr: where to the BO is mapped
 *
 * Remove a mapping of the BO at the specefied addr from the VM.
 * Returns 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int amdgpu_vm_bo_unmap(struct amdgpu_device *adev,
		       struct amdgpu_bo_va *bo_va,
		       uint64_t saddr)
{
	struct amdgpu_bo_va_mapping *mapping;
	struct amdgpu_vm *vm = bo_va->base.vm;
	bool valid = true;

	saddr /= AMDGPU_GPU_PAGE_SIZE;

	list_for_each_entry(mapping, &bo_va->valids, list) {
		if (mapping->start == saddr)
			break;
	}

	if (&mapping->list == &bo_va->valids) {
		valid = false;

		list_for_each_entry(mapping, &bo_va->invalids, list) {
			if (mapping->start == saddr)
				break;
		}

		if (&mapping->list == &bo_va->invalids)
			return -ENOENT;
	}

	list_del(&mapping->list);
	amdgpu_vm_it_remove(mapping, &vm->va);
	mapping->bo_va = NULL;
	trace_amdgpu_vm_bo_unmap(bo_va, mapping);

	if (valid)
		list_add(&mapping->list, &vm->freed);
	else
		amdgpu_vm_free_mapping(adev, vm, mapping,
				       bo_va->last_pt_update);

	return 0;
}

/**
 * amdgpu_vm_bo_clear_mappings - remove all mappings in a specific range
 *
 * @adev: amdgpu_device pointer
 * @vm: VM structure to use
 * @saddr: start of the range
 * @size: size of the range
 *
 * Remove all mappings in a range, split them as appropriate.
 * Returns 0 for success, error for failure.
 */
int amdgpu_vm_bo_clear_mappings(struct amdgpu_device *adev,
				struct amdgpu_vm *vm,
				uint64_t saddr, uint64_t size)
{
	struct amdgpu_bo_va_mapping *before, *after, *tmp, *next;
	LIST_HEAD(removed);
	uint64_t eaddr;

	eaddr = saddr + size - 1;
	saddr /= AMDGPU_GPU_PAGE_SIZE;
	eaddr /= AMDGPU_GPU_PAGE_SIZE;

	/* Allocate all the needed memory */
	before = kzalloc(sizeof(*before), GFP_KERNEL);
	if (!before)
		return -ENOMEM;
	INIT_LIST_HEAD(&before->list);

	after = kzalloc(sizeof(*after), GFP_KERNEL);
	if (!after) {
		kfree(before);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&after->list);

	/* Now gather all removed mappings */
	tmp = amdgpu_vm_it_iter_first(&vm->va, saddr, eaddr);
	while (tmp) {
		/* Remember mapping split at the start */
		if (tmp->start < saddr) {
			before->start = tmp->start;
			before->last = saddr - 1;
			before->offset = tmp->offset;
			before->flags = tmp->flags;
			list_add(&before->list, &tmp->list);
		}

		/* Remember mapping split at the end */
		if (tmp->last > eaddr) {
			after->start = eaddr + 1;
			after->last = tmp->last;
			after->offset = tmp->offset;
			after->offset += after->start - tmp->start;
			after->flags = tmp->flags;
			list_add(&after->list, &tmp->list);
		}

		list_del(&tmp->list);
		list_add(&tmp->list, &removed);

		tmp = amdgpu_vm_it_iter_next(tmp, saddr, eaddr);
	}

	/* And free them up */
	list_for_each_entry_safe(tmp, next, &removed, list) {
		amdgpu_vm_it_remove(tmp, &vm->va);
		list_del(&tmp->list);

		if (tmp->start < saddr)
		    tmp->start = saddr;
		if (tmp->last > eaddr)
		    tmp->last = eaddr;

		tmp->bo_va = NULL;
		list_add(&tmp->list, &vm->freed);
		trace_amdgpu_vm_bo_unmap(NULL, tmp);
	}

	/* Insert partial mapping before the range */
	if (!list_empty(&before->list)) {
		amdgpu_vm_it_insert(before, &vm->va);
		if (before->flags & AMDGPU_PTE_PRT)
			amdgpu_vm_prt_get(adev);
	} else {
		kfree(before);
	}

	/* Insert partial mapping after the range */
	if (!list_empty(&after->list)) {
		amdgpu_vm_it_insert(after, &vm->va);
		if (after->flags & AMDGPU_PTE_PRT)
			amdgpu_vm_prt_get(adev);
	} else {
		kfree(after);
	}

	return 0;
}

/**
 * amdgpu_vm_bo_lookup_mapping - find mapping by address
 *
 * @vm: the requested VM
 *
 * Find a mapping by it's address.
 */
struct amdgpu_bo_va_mapping *amdgpu_vm_bo_lookup_mapping(struct amdgpu_vm *vm,
							 uint64_t addr)
{
	return amdgpu_vm_it_iter_first(&vm->va, addr, addr);
}

/**
 * amdgpu_vm_bo_rmv - remove a bo to a specific vm
 *
 * @adev: amdgpu_device pointer
 * @bo_va: requested bo_va
 *
 * Remove @bo_va->bo from the requested vm.
 *
 * Object have to be reserved!
 */
void amdgpu_vm_bo_rmv(struct amdgpu_device *adev,
		      struct amdgpu_bo_va *bo_va)
{
	struct amdgpu_bo_va_mapping *mapping, *next;
	struct amdgpu_vm *vm = bo_va->base.vm;

	list_del(&bo_va->base.bo_list);

	spin_lock(&vm->status_lock);
	list_del(&bo_va->base.vm_status);
	spin_unlock(&vm->status_lock);

	list_for_each_entry_safe(mapping, next, &bo_va->valids, list) {
		list_del(&mapping->list);
		amdgpu_vm_it_remove(mapping, &vm->va);
		mapping->bo_va = NULL;
		trace_amdgpu_vm_bo_unmap(bo_va, mapping);
		list_add(&mapping->list, &vm->freed);
	}
	list_for_each_entry_safe(mapping, next, &bo_va->invalids, list) {
		list_del(&mapping->list);
		amdgpu_vm_it_remove(mapping, &vm->va);
		amdgpu_vm_free_mapping(adev, vm, mapping,
				       bo_va->last_pt_update);
	}

	dma_fence_put(bo_va->last_pt_update);
	kfree(bo_va);
}

/**
 * amdgpu_vm_bo_invalidate - mark the bo as invalid
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @bo: amdgpu buffer object
 *
 * Mark @bo as invalid.
 */
void amdgpu_vm_bo_invalidate(struct amdgpu_device *adev,
			     struct amdgpu_bo *bo, bool evicted)
{
	struct amdgpu_vm_bo_base *bo_base;

	list_for_each_entry(bo_base, &bo->va, bo_list) {
		struct amdgpu_vm *vm = bo_base->vm;

		bo_base->moved = true;
		if (evicted && bo->tbo.resv == vm->root.base.bo->tbo.resv) {
			spin_lock(&bo_base->vm->status_lock);
			if (bo->tbo.type == ttm_bo_type_kernel)
				list_move(&bo_base->vm_status, &vm->evicted);
			else
				list_move_tail(&bo_base->vm_status,
					       &vm->evicted);
			spin_unlock(&bo_base->vm->status_lock);
			continue;
		}

		if (bo->tbo.type == ttm_bo_type_kernel) {
			spin_lock(&bo_base->vm->status_lock);
			if (list_empty(&bo_base->vm_status))
				list_add(&bo_base->vm_status, &vm->relocated);
			spin_unlock(&bo_base->vm->status_lock);
			continue;
		}

		spin_lock(&bo_base->vm->status_lock);
		if (list_empty(&bo_base->vm_status))
			list_add(&bo_base->vm_status, &vm->moved);
		spin_unlock(&bo_base->vm->status_lock);
	}
}

static uint32_t amdgpu_vm_get_block_size(uint64_t vm_size)
{
	/* Total bits covered by PD + PTs */
	unsigned bits = ilog2(vm_size) + 18;

	/* Make sure the PD is 4K in size up to 8GB address space.
	   Above that split equal between PD and PTs */
	if (vm_size <= 8)
		return (bits - 9);
	else
		return ((bits + 3) / 2);
}

/**
 * amdgpu_vm_set_fragment_size - adjust fragment size in PTE
 *
 * @adev: amdgpu_device pointer
 * @fragment_size_default: the default fragment size if it's set auto
 */
void amdgpu_vm_set_fragment_size(struct amdgpu_device *adev, uint32_t fragment_size_default)
{
	if (amdgpu_vm_fragment_size == -1)
		adev->vm_manager.fragment_size = fragment_size_default;
	else
		adev->vm_manager.fragment_size = amdgpu_vm_fragment_size;
}

/**
 * amdgpu_vm_adjust_size - adjust vm size, block size and fragment size
 *
 * @adev: amdgpu_device pointer
 * @vm_size: the default vm size if it's set auto
 */
void amdgpu_vm_adjust_size(struct amdgpu_device *adev, uint64_t vm_size, uint32_t fragment_size_default)
{
	/* adjust vm size firstly */
	if (amdgpu_vm_size == -1)
		adev->vm_manager.vm_size = vm_size;
	else
		adev->vm_manager.vm_size = amdgpu_vm_size;

	/* block size depends on vm size */
	if (amdgpu_vm_block_size == -1)
		adev->vm_manager.block_size =
			amdgpu_vm_get_block_size(adev->vm_manager.vm_size);
	else
		adev->vm_manager.block_size = amdgpu_vm_block_size;

	amdgpu_vm_set_fragment_size(adev, fragment_size_default);

	DRM_INFO("vm size is %llu GB, block size is %u-bit, fragment size is %u-bit\n",
		adev->vm_manager.vm_size, adev->vm_manager.block_size,
		adev->vm_manager.fragment_size);
}

/**
 * amdgpu_vm_init - initialize a vm instance
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 * @vm_context: Indicates if it GFX or Compute context
 *
 * Init @vm fields.
 */
int amdgpu_vm_init(struct amdgpu_device *adev, struct amdgpu_vm *vm,
		   int vm_context)
{
	const unsigned align = min(AMDGPU_VM_PTB_ALIGN_SIZE,
		AMDGPU_VM_PTE_COUNT(adev) * 8);
	unsigned ring_instance;
	struct amdgpu_ring *ring;
	struct amd_sched_rq *rq;
	int r, i;
	u64 flags;
	uint64_t init_pde_value = 0;

	vm->va = RB_ROOT;
	vm->client_id = atomic64_inc_return(&adev->vm_manager.client_counter);
	for (i = 0; i < AMDGPU_MAX_VMHUBS; i++)
		vm->reserved_vmid[i] = NULL;
	spin_lock_init(&vm->status_lock);
	INIT_LIST_HEAD(&vm->evicted);
	INIT_LIST_HEAD(&vm->relocated);
	INIT_LIST_HEAD(&vm->moved);
	INIT_LIST_HEAD(&vm->freed);

	/* create scheduler entity for page table updates */

	ring_instance = atomic_inc_return(&adev->vm_manager.vm_pte_next_ring);
	ring_instance %= adev->vm_manager.vm_pte_num_rings;
	ring = adev->vm_manager.vm_pte_rings[ring_instance];
	rq = &ring->sched.sched_rq[AMD_SCHED_PRIORITY_KERNEL];
	r = amd_sched_entity_init(&ring->sched, &vm->entity,
				  rq, amdgpu_sched_jobs);
	if (r)
		return r;

	vm->pte_support_ats = false;

	if (vm_context == AMDGPU_VM_CONTEXT_COMPUTE) {
		vm->use_cpu_for_update = !!(adev->vm_manager.vm_update_mode &
						AMDGPU_VM_USE_CPU_FOR_COMPUTE);

		if (adev->asic_type == CHIP_RAVEN) {
			vm->pte_support_ats = true;
			init_pde_value = AMDGPU_PTE_SYSTEM | AMDGPU_PDE_PTE;
		}
	} else
		vm->use_cpu_for_update = !!(adev->vm_manager.vm_update_mode &
						AMDGPU_VM_USE_CPU_FOR_GFX);
	DRM_DEBUG_DRIVER("VM update mode is %s\n",
			 vm->use_cpu_for_update ? "CPU" : "SDMA");
	WARN_ONCE((vm->use_cpu_for_update & !amdgpu_vm_is_large_bar(adev)),
		  "CPU update of VM recommended only for large BAR system\n");
	vm->last_update = NULL;

	flags = AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS |
			AMDGPU_GEM_CREATE_VRAM_CLEARED;
	if (vm->use_cpu_for_update)
		flags |= AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	else
		flags |= (AMDGPU_GEM_CREATE_NO_CPU_ACCESS |
				AMDGPU_GEM_CREATE_SHADOW);

	r = amdgpu_bo_create(adev, amdgpu_vm_bo_size(adev, 0), align, true,
			     AMDGPU_GEM_DOMAIN_VRAM,
			     flags,
			     NULL, NULL, init_pde_value, &vm->root.base.bo);
	if (r)
		goto error_free_sched_entity;

	vm->root.base.vm = vm;
	list_add_tail(&vm->root.base.bo_list, &vm->root.base.bo->va);
	INIT_LIST_HEAD(&vm->root.base.vm_status);

	if (vm->use_cpu_for_update) {
		r = amdgpu_bo_reserve(vm->root.base.bo, false);
		if (r)
			goto error_free_root;

		r = amdgpu_bo_kmap(vm->root.base.bo, NULL);
		if (r)
			goto error_free_root;
		amdgpu_bo_unreserve(vm->root.base.bo);
	}

	return 0;

error_free_root:
	amdgpu_bo_unref(&vm->root.base.bo->shadow);
	amdgpu_bo_unref(&vm->root.base.bo);
	vm->root.base.bo = NULL;

error_free_sched_entity:
	amd_sched_entity_fini(&ring->sched, &vm->entity);

	return r;
}

/**
 * amdgpu_vm_free_levels - free PD/PT levels
 *
 * @level: PD/PT starting level to free
 *
 * Free the page directory or page table level and all sub levels.
 */
static void amdgpu_vm_free_levels(struct amdgpu_vm_pt *level)
{
	unsigned i;

	if (level->base.bo) {
		list_del(&level->base.bo_list);
		list_del(&level->base.vm_status);
		amdgpu_bo_unref(&level->base.bo->shadow);
		amdgpu_bo_unref(&level->base.bo);
	}

	if (level->entries)
		for (i = 0; i <= level->last_entry_used; i++)
			amdgpu_vm_free_levels(&level->entries[i]);

	kvfree(level->entries);
}

/**
 * amdgpu_vm_fini - tear down a vm instance
 *
 * @adev: amdgpu_device pointer
 * @vm: requested vm
 *
 * Tear down @vm.
 * Unbind the VM and remove all bos from the vm bo list
 */
void amdgpu_vm_fini(struct amdgpu_device *adev, struct amdgpu_vm *vm)
{
	struct amdgpu_bo_va_mapping *mapping, *tmp;
	bool prt_fini_needed = !!adev->gart.gart_funcs->set_prt;
	int i;

	amd_sched_entity_fini(vm->entity.sched, &vm->entity);

	if (!RB_EMPTY_ROOT(&vm->va)) {
		dev_err(adev->dev, "still active bo inside vm\n");
	}
	rbtree_postorder_for_each_entry_safe(mapping, tmp, &vm->va, rb) {
		list_del(&mapping->list);
		amdgpu_vm_it_remove(mapping, &vm->va);
		kfree(mapping);
	}
	list_for_each_entry_safe(mapping, tmp, &vm->freed, list) {
		if (mapping->flags & AMDGPU_PTE_PRT && prt_fini_needed) {
			amdgpu_vm_prt_fini(adev, vm);
			prt_fini_needed = false;
		}

		list_del(&mapping->list);
		amdgpu_vm_free_mapping(adev, vm, mapping, NULL);
	}

	amdgpu_vm_free_levels(&vm->root);
	dma_fence_put(vm->last_update);
	for (i = 0; i < AMDGPU_MAX_VMHUBS; i++)
		amdgpu_vm_free_reserved_vmid(adev, vm, i);
}

/**
 * amdgpu_vm_manager_init - init the VM manager
 *
 * @adev: amdgpu_device pointer
 *
 * Initialize the VM manager structures
 */
void amdgpu_vm_manager_init(struct amdgpu_device *adev)
{
	unsigned i, j;

	for (i = 0; i < AMDGPU_MAX_VMHUBS; ++i) {
		struct amdgpu_vm_id_manager *id_mgr =
			&adev->vm_manager.id_mgr[i];

		mutex_init(&id_mgr->lock);
		INIT_LIST_HEAD(&id_mgr->ids_lru);
		atomic_set(&id_mgr->reserved_vmid_num, 0);

		/* skip over VMID 0, since it is the system VM */
		for (j = 1; j < id_mgr->num_ids; ++j) {
			amdgpu_vm_reset_id(adev, i, j);
			amdgpu_sync_create(&id_mgr->ids[i].active);
			list_add_tail(&id_mgr->ids[j].list, &id_mgr->ids_lru);
		}
	}

	adev->vm_manager.fence_context =
		dma_fence_context_alloc(AMDGPU_MAX_RINGS);
	for (i = 0; i < AMDGPU_MAX_RINGS; ++i)
		adev->vm_manager.seqno[i] = 0;

	atomic_set(&adev->vm_manager.vm_pte_next_ring, 0);
	atomic64_set(&adev->vm_manager.client_counter, 0);
	spin_lock_init(&adev->vm_manager.prt_lock);
	atomic_set(&adev->vm_manager.num_prt_users, 0);

	/* If not overridden by the user, by default, only in large BAR systems
	 * Compute VM tables will be updated by CPU
	 */
#ifdef CONFIG_X86_64
	if (amdgpu_vm_update_mode == -1) {
		if (amdgpu_vm_is_large_bar(adev))
			adev->vm_manager.vm_update_mode =
				AMDGPU_VM_USE_CPU_FOR_COMPUTE;
		else
			adev->vm_manager.vm_update_mode = 0;
	} else
		adev->vm_manager.vm_update_mode = amdgpu_vm_update_mode;
#else
	adev->vm_manager.vm_update_mode = 0;
#endif

}

/**
 * amdgpu_vm_manager_fini - cleanup VM manager
 *
 * @adev: amdgpu_device pointer
 *
 * Cleanup the VM manager and free resources.
 */
void amdgpu_vm_manager_fini(struct amdgpu_device *adev)
{
	unsigned i, j;

	for (i = 0; i < AMDGPU_MAX_VMHUBS; ++i) {
		struct amdgpu_vm_id_manager *id_mgr =
			&adev->vm_manager.id_mgr[i];

		mutex_destroy(&id_mgr->lock);
		for (j = 0; j < AMDGPU_NUM_VM; ++j) {
			struct amdgpu_vm_id *id = &id_mgr->ids[j];

			amdgpu_sync_free(&id->active);
			dma_fence_put(id->flushed_updates);
			dma_fence_put(id->last_flush);
		}
	}
}

int amdgpu_vm_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	union drm_amdgpu_vm *args = data;
	struct amdgpu_device *adev = dev->dev_private;
	struct amdgpu_fpriv *fpriv = filp->driver_priv;
	int r;

	switch (args->in.op) {
	case AMDGPU_VM_OP_RESERVE_VMID:
		/* current, we only have requirement to reserve vmid from gfxhub */
		r = amdgpu_vm_alloc_reserved_vmid(adev, &fpriv->vm,
						  AMDGPU_GFXHUB);
		if (r)
			return r;
		break;
	case AMDGPU_VM_OP_UNRESERVE_VMID:
		amdgpu_vm_free_reserved_vmid(adev, &fpriv->vm, AMDGPU_GFXHUB);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
