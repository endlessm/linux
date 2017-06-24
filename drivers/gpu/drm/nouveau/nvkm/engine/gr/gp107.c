/*
 * Copyright 2017 Red Hat Inc.
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
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */
#include "gf100.h"
#include "ctxgf100.h"

#include <nvif/class.h>

static const struct gf100_gr_func
gp107_gr = {
	.init = gp100_gr_init,
	.init_gpc_mmu = gm200_gr_init_gpc_mmu,
	.init_rop_active_fbps = gp100_gr_init_rop_active_fbps,
	.init_ppc_exceptions = gk104_gr_init_ppc_exceptions,
	.init_swdx_pes_mask = gp102_gr_init_swdx_pes_mask,
	.init_num_active_ltcs = gp100_gr_init_num_active_ltcs,
	.rops = gm200_gr_rops,
	.ppc_nr = 1,
	.grctx = &gp107_grctx,
	.sclass = {
		{ -1, -1, FERMI_TWOD_A },
		{ -1, -1, KEPLER_INLINE_TO_MEMORY_B },
		{ -1, -1, PASCAL_B, &gf100_fermi },
		{ -1, -1, PASCAL_COMPUTE_B },
		{}
	}
};

int
gp107_gr_new(struct nvkm_device *device, int index, struct nvkm_gr **pgr)
{
	return gm200_gr_new_(&gp107_gr, device, index, pgr);
}
