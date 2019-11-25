/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#include <linux/workqueue.h>

#include "i915_drv.h" /* for_each_engine() */
#include "i915_request.h"
#include "intel_gt.h"
#include "intel_gt_pm.h"
#include "intel_gt_requests.h"
#include "intel_timeline.h"

static void retire_requests(struct intel_timeline *tl)
{
	struct i915_request *rq, *rn;

	list_for_each_entry_safe(rq, rn, &tl->requests, link)
		if (!i915_request_retire(rq))
			break;
}

static void engine_retire(struct work_struct *work)
{
	struct intel_engine_cs *engine =
		container_of(work, typeof(*engine), retire_work);
	struct intel_timeline *tl = xchg(&engine->retire, NULL);

	mutex_lock(&engine->i915->drm.struct_mutex);
	do {
		struct intel_timeline *next = xchg(&tl->retire, NULL);

		/*
		 * Our goal here is to retire _idle_ timelines as soon as
		 * possible (as they are idle, we do not expect userspace
		 * to be cleaning up anytime soon).
		 *
		 * If the timeline is currently locked, either it is being
		 * retired elsewhere or about to be!
		 */
		if (mutex_trylock(&tl->mutex)) {
			retire_requests(tl);
			mutex_unlock(&tl->mutex);
		}
		intel_timeline_put(tl);

		GEM_BUG_ON(!next);
		tl = ptr_mask_bits(next, 1);
	} while (tl);
	mutex_unlock(&engine->i915->drm.struct_mutex);
}

static bool add_retire(struct intel_engine_cs *engine,
		       struct intel_timeline *tl)
{
	struct intel_timeline *first;

	/*
	 * We open-code a llist here to include the additional tag [BIT(0)]
	 * so that we know when the timeline is already on a
	 * retirement queue: either this engine or another.
	 *
	 * However, we rely on that a timeline can only be active on a single
	 * engine at any one time and that add_retire() is called before the
	 * engine releases the timeline and transferred to another to retire.
	 */

	if (READ_ONCE(tl->retire)) /* already queued */
		return false;

	intel_timeline_get(tl);
	first = READ_ONCE(engine->retire);
	do
		tl->retire = ptr_pack_bits(first, 1, 1);
	while (!try_cmpxchg(&engine->retire, &first, tl));

	return !first;
}

void intel_engine_add_retire(struct intel_engine_cs *engine,
			     struct intel_timeline *tl)
{
	if (add_retire(engine, tl))
		schedule_work(&engine->retire_work);
}

void intel_engine_init_retire(struct intel_engine_cs *engine)
{
	INIT_WORK(&engine->retire_work, engine_retire);
}

void intel_engine_fini_retire(struct intel_engine_cs *engine)
{
	flush_work(&engine->retire_work);
	GEM_BUG_ON(engine->retire);
}
