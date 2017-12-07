/*
 * Copyright 2014 Advanced Micro Devices, Inc.
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
 */

#include "amdgpu_amdkfd.h"
#include "amd_shared.h"
#include <drm/drmP.h>
#include "amdgpu.h"
#include "amdgpu_gfx.h"
#include <linux/module.h>

const struct kgd2kfd_calls *kgd2kfd;
bool (*kgd2kfd_init_p)(unsigned int, const struct kgd2kfd_calls**);

int amdgpu_amdkfd_init(void)
{
	int ret;

#if defined(CONFIG_HSA_AMD_MODULE)
	int (*kgd2kfd_init_p)(unsigned int, const struct kgd2kfd_calls**);

	kgd2kfd_init_p = symbol_request(kgd2kfd_init);

	if (kgd2kfd_init_p == NULL)
		return -ENOENT;

	ret = kgd2kfd_init_p(KFD_INTERFACE_VERSION, &kgd2kfd);
	if (ret) {
		symbol_put(kgd2kfd_init);
		kgd2kfd = NULL;
	}

#elif defined(CONFIG_HSA_AMD)
	ret = kgd2kfd_init(KFD_INTERFACE_VERSION, &kgd2kfd);
	if (ret)
		kgd2kfd = NULL;

#else
	ret = -ENOENT;
#endif

	return ret;
}

void amdgpu_amdkfd_fini(void)
{
	if (kgd2kfd) {
		kgd2kfd->exit();
		symbol_put(kgd2kfd_init);
	}
}

void amdgpu_amdkfd_device_probe(struct amdgpu_device *adev)
{
	const struct kfd2kgd_calls *kfd2kgd;

	if (!kgd2kfd)
		return;

	switch (adev->asic_type) {
#ifdef CONFIG_DRM_AMDGPU_CIK
	case CHIP_KAVERI:
		kfd2kgd = amdgpu_amdkfd_gfx_7_get_functions();
		break;
#endif
	case CHIP_CARRIZO:
		kfd2kgd = amdgpu_amdkfd_gfx_8_0_get_functions();
		break;
	default:
		dev_info(adev->dev, "kfd not supported on this ASIC\n");
		return;
	}

	adev->kfd = kgd2kfd->probe((struct kgd_dev *)adev,
				   adev->pdev, kfd2kgd);
}

void amdgpu_amdkfd_device_init(struct amdgpu_device *adev)
{
	int i;
	int last_valid_bit;
	if (adev->kfd) {
		struct kgd2kfd_shared_resources gpu_resources = {
			.compute_vmid_bitmap = 0xFF00,
			.num_pipe_per_mec = adev->gfx.mec.num_pipe_per_mec,
			.num_queue_per_pipe = adev->gfx.mec.num_queue_per_pipe
		};

		/* this is going to have a few of the MSBs set that we need to
		 * clear */
		bitmap_complement(gpu_resources.queue_bitmap,
				  adev->gfx.mec.queue_bitmap,
				  KGD_MAX_QUEUES);

		/* remove the KIQ bit as well */
		if (adev->gfx.kiq.ring.ready)
			clear_bit(amdgpu_gfx_queue_to_bit(adev,
							  adev->gfx.kiq.ring.me - 1,
							  adev->gfx.kiq.ring.pipe,
							  adev->gfx.kiq.ring.queue),
				  gpu_resources.queue_bitmap);

		/* According to linux/bitmap.h we shouldn't use bitmap_clear if
		 * nbits is not compile time constant */
		last_valid_bit = 1 /* only first MEC can have compute queues */
				* adev->gfx.mec.num_pipe_per_mec
				* adev->gfx.mec.num_queue_per_pipe;
		for (i = last_valid_bit; i < KGD_MAX_QUEUES; ++i)
			clear_bit(i, gpu_resources.queue_bitmap);

		amdgpu_doorbell_get_kfd_info(adev,
				&gpu_resources.doorbell_physical_address,
				&gpu_resources.doorbell_aperture_size,
				&gpu_resources.doorbell_start_offset);

		kgd2kfd->device_init(adev->kfd, &gpu_resources);
	}
}

void amdgpu_amdkfd_device_fini(struct amdgpu_device *adev)
{
	if (adev->kfd) {
		kgd2kfd->device_exit(adev->kfd);
		adev->kfd = NULL;
	}
}

void amdgpu_amdkfd_interrupt(struct amdgpu_device *adev,
		const void *ih_ring_entry)
{
	if (adev->kfd)
		kgd2kfd->interrupt(adev->kfd, ih_ring_entry);
}

void amdgpu_amdkfd_suspend(struct amdgpu_device *adev)
{
	if (adev->kfd)
		kgd2kfd->suspend(adev->kfd);
}

int amdgpu_amdkfd_resume(struct amdgpu_device *adev)
{
	int r = 0;

	if (adev->kfd)
		r = kgd2kfd->resume(adev->kfd);

	return r;
}

int alloc_gtt_mem(struct kgd_dev *kgd, size_t size,
			void **mem_obj, uint64_t *gpu_addr,
			void **cpu_ptr)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)kgd;
	struct kgd_mem **mem = (struct kgd_mem **) mem_obj;
	int r;

	BUG_ON(kgd == NULL);
	BUG_ON(gpu_addr == NULL);
	BUG_ON(cpu_ptr == NULL);

	*mem = kmalloc(sizeof(struct kgd_mem), GFP_KERNEL);
	if ((*mem) == NULL)
		return -ENOMEM;

	r = amdgpu_bo_create(adev, size, PAGE_SIZE, true, AMDGPU_GEM_DOMAIN_GTT,
			     AMDGPU_GEM_CREATE_CPU_GTT_USWC, NULL, NULL, 0,
			     &(*mem)->bo);
	if (r) {
		dev_err(adev->dev,
			"failed to allocate BO for amdkfd (%d)\n", r);
		return r;
	}

	/* map the buffer */
	r = amdgpu_bo_reserve((*mem)->bo, true);
	if (r) {
		dev_err(adev->dev, "(%d) failed to reserve bo for amdkfd\n", r);
		goto allocate_mem_reserve_bo_failed;
	}

	r = amdgpu_bo_pin((*mem)->bo, AMDGPU_GEM_DOMAIN_GTT,
				&(*mem)->gpu_addr);
	if (r) {
		dev_err(adev->dev, "(%d) failed to pin bo for amdkfd\n", r);
		goto allocate_mem_pin_bo_failed;
	}
	*gpu_addr = (*mem)->gpu_addr;

	r = amdgpu_bo_kmap((*mem)->bo, &(*mem)->cpu_ptr);
	if (r) {
		dev_err(adev->dev,
			"(%d) failed to map bo to kernel for amdkfd\n", r);
		goto allocate_mem_kmap_bo_failed;
	}
	*cpu_ptr = (*mem)->cpu_ptr;

	amdgpu_bo_unreserve((*mem)->bo);

	return 0;

allocate_mem_kmap_bo_failed:
	amdgpu_bo_unpin((*mem)->bo);
allocate_mem_pin_bo_failed:
	amdgpu_bo_unreserve((*mem)->bo);
allocate_mem_reserve_bo_failed:
	amdgpu_bo_unref(&(*mem)->bo);

	return r;
}

void free_gtt_mem(struct kgd_dev *kgd, void *mem_obj)
{
	struct kgd_mem *mem = (struct kgd_mem *) mem_obj;

	BUG_ON(mem == NULL);

	amdgpu_bo_reserve(mem->bo, true);
	amdgpu_bo_kunmap(mem->bo);
	amdgpu_bo_unpin(mem->bo);
	amdgpu_bo_unreserve(mem->bo);
	amdgpu_bo_unref(&(mem->bo));
	kfree(mem);
}

uint64_t get_vmem_size(struct kgd_dev *kgd)
{
	struct amdgpu_device *adev =
		(struct amdgpu_device *)kgd;

	BUG_ON(kgd == NULL);

	return adev->mc.real_vram_size;
}

uint64_t get_gpu_clock_counter(struct kgd_dev *kgd)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)kgd;

	if (adev->gfx.funcs->get_gpu_clock_counter)
		return adev->gfx.funcs->get_gpu_clock_counter(adev);
	return 0;
}

uint32_t get_max_engine_clock_in_mhz(struct kgd_dev *kgd)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)kgd;

	/* The sclk is in quantas of 10kHz */
	return adev->pm.dpm.dyn_state.max_clock_voltage_on_ac.sclk / 100;
}
