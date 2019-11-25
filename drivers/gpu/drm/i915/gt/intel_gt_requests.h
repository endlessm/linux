/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#ifndef INTEL_GT_REQUESTS_H
#define INTEL_GT_REQUESTS_H

struct intel_engine_cs;
struct intel_timeline;

void intel_engine_init_retire(struct intel_engine_cs *engine);
void intel_engine_add_retire(struct intel_engine_cs *engine,
			     struct intel_timeline *tl);
void intel_engine_fini_retire(struct intel_engine_cs *engine);

#endif /* INTEL_GT_REQUESTS_H */
