/*
 * Copyright 2017 Advanced Micro Devices, Inc.
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
 * Authors: AMD
 *
 */
#ifndef __DISPLAY_MODE_LIB_H__
#define __DISPLAY_MODE_LIB_H__

#include "dml_common_defs.h"
#include "soc_bounding_box.h"
#include "display_watermark.h"
#include "display_pipe_clocks.h"
#include "display_rq_dlg_calc.h"
#include "display_mode_support.h"

enum dml_project {
	DML_PROJECT_UNDEFINED,
	DML_PROJECT_RAVEN1
};

struct display_mode_lib {
	struct _vcs_dpi_ip_params_st ip;
	struct _vcs_dpi_soc_bounding_box_st soc;
	struct _vcs_dpi_mode_evaluation_st me;
	enum dml_project project;
	struct dml_ms_internal_vars vars;
	struct _vcs_dpi_wm_calc_pipe_params_st wm_param[DC__NUM_PIPES__MAX];
	struct dal_logger *logger;
};

void dml_init_instance(struct display_mode_lib *lib, enum dml_project project);

#endif
