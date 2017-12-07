/*
 * Copyright 2015 Advanced Micro Devices, Inc.
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

/**
 * This file defines external dependencies of Display Core.
 */

#ifndef __DM_SERVICES_H__

#define __DM_SERVICES_H__

/* TODO: remove when DC is complete. */
#include "dm_services_types.h"
#include "logger_interface.h"
#include "link_service_types.h"

#undef DEPRECATED

/*
 *
 * general debug capabilities
 *
 */
#if defined(CONFIG_DEBUG_KERNEL) || defined(CONFIG_DEBUG_DRIVER)

#if defined(CONFIG_HAVE_KGDB) || defined(CONFIG_KGDB)
#define ASSERT_CRITICAL(expr) do {	\
	if (WARN_ON(!(expr))) { \
		kgdb_breakpoint(); \
	} \
} while (0)
#else
#define ASSERT_CRITICAL(expr) do {	\
	if (WARN_ON(!(expr))) { \
		; \
	} \
} while (0)
#endif

#if defined(CONFIG_DEBUG_KERNEL_DC)
#define ASSERT(expr) ASSERT_CRITICAL(expr)

#else
#define ASSERT(expr) WARN_ON(!(expr))
#endif

#define BREAK_TO_DEBUGGER() ASSERT(0)

#endif /* CONFIG_DEBUG_KERNEL || CONFIG_DEBUG_DRIVER */

#define DC_ERR(...)  do { \
	dm_error(__VA_ARGS__); \
	BREAK_TO_DEBUGGER(); \
} while (0)

#if defined(CONFIG_DRM_AMD_DC_DCN1_0)
#include <asm/fpu/api.h>
#endif

#define dm_alloc(size) kzalloc(size, GFP_KERNEL)
#define dm_realloc(ptr, size) krealloc(ptr, size, GFP_KERNEL)
#define dm_free(ptr) kfree(ptr)

irq_handler_idx dm_register_interrupt(
	struct dc_context *ctx,
	struct dc_interrupt_params *int_params,
	interrupt_handler ih,
	void *handler_args);


/*
 *
 * GPU registers access
 *
 */

#define dm_read_reg(ctx, address)	\
		dm_read_reg_func(ctx, address, __func__)

static inline uint32_t dm_read_reg_func(
	const struct dc_context *ctx,
	uint32_t address,
	const char *func_name)
{
	uint32_t value;

	if (address == 0) {
		DC_ERR("invalid register read; address = 0\n");
		return 0;
	}

	value = cgs_read_register(ctx->cgs_device, address);

	return value;
}

#define dm_write_reg(ctx, address, value)	\
	dm_write_reg_func(ctx, address, value, __func__)

static inline void dm_write_reg_func(
	const struct dc_context *ctx,
	uint32_t address,
	uint32_t value,
	const char *func_name)
{
	if (address == 0) {
		DC_ERR("invalid register write. address = 0");
		return;
	}
	cgs_write_register(ctx->cgs_device, address, value);
}

static inline uint32_t dm_read_index_reg(
	const struct dc_context *ctx,
	enum cgs_ind_reg addr_space,
	uint32_t index)
{
	return cgs_read_ind_register(ctx->cgs_device, addr_space, index);
}

static inline void dm_write_index_reg(
	const struct dc_context *ctx,
	enum cgs_ind_reg addr_space,
	uint32_t index,
	uint32_t value)
{
	cgs_write_ind_register(ctx->cgs_device, addr_space, index, value);
}

static inline uint32_t get_reg_field_value_ex(
	uint32_t reg_value,
	uint32_t mask,
	uint8_t shift)
{
	return (mask & reg_value) >> shift;
}

#define get_reg_field_value(reg_value, reg_name, reg_field)\
	get_reg_field_value_ex(\
		(reg_value),\
		reg_name ## __ ## reg_field ## _MASK,\
		reg_name ## __ ## reg_field ## __SHIFT)

static inline uint32_t set_reg_field_value_ex(
	uint32_t reg_value,
	uint32_t value,
	uint32_t mask,
	uint8_t shift)
{
	ASSERT(mask != 0);
	return (reg_value & ~mask) | (mask & (value << shift));
}

#define set_reg_field_value(reg_value, value, reg_name, reg_field)\
	(reg_value) = set_reg_field_value_ex(\
		(reg_value),\
		(value),\
		reg_name ## __ ## reg_field ## _MASK,\
		reg_name ## __ ## reg_field ## __SHIFT)

uint32_t generic_reg_update_ex(const struct dc_context *ctx,
		uint32_t addr, uint32_t reg_val, int n,
		uint8_t shift1, uint32_t mask1, uint32_t field_value1, ...);

#define FD(reg_field)	reg_field ## __SHIFT, \
						reg_field ## _MASK

/*
 * return number of poll before condition is met
 * return 0 if condition is not meet after specified time out tries
 */
unsigned int generic_reg_wait(const struct dc_context *ctx,
	uint32_t addr, uint32_t mask, uint32_t shift, uint32_t condition_value,
	unsigned int delay_between_poll_us, unsigned int time_out_num_tries,
	const char *func_name, int line);


/* These macros need to be used with soc15 registers in order to retrieve
 * the actual offset.
 */
#define REG_OFFSET(reg) (reg + DCE_BASE.instance[0].segment[reg##_BASE_IDX])
#define REG_BIF_OFFSET(reg) (reg + NBIF_BASE.instance[0].segment[reg##_BASE_IDX])

#define dm_write_reg_soc15(ctx, reg, inst_offset, value)	\
		dm_write_reg_func(ctx, reg + DCE_BASE.instance[0].segment[reg##_BASE_IDX] + inst_offset, value, __func__)

#define dm_read_reg_soc15(ctx, reg, inst_offset)	\
		dm_read_reg_func(ctx, reg + DCE_BASE.instance[0].segment[reg##_BASE_IDX] + inst_offset, __func__)

#define generic_reg_update_soc15(ctx, inst_offset, reg_name, n, ...)\
		generic_reg_update_ex(ctx, DCE_BASE.instance[0].segment[mm##reg_name##_BASE_IDX] +  mm##reg_name + inst_offset, \
		dm_read_reg_func(ctx, mm##reg_name + DCE_BASE.instance[0].segment[mm##reg_name##_BASE_IDX] + inst_offset, __func__), \
		n, __VA_ARGS__)

#define generic_reg_set_soc15(ctx, inst_offset, reg_name, n, ...)\
		generic_reg_update_ex(ctx, DCE_BASE.instance[0].segment[mm##reg_name##_BASE_IDX] + mm##reg_name + inst_offset, 0, \
		n, __VA_ARGS__)

#define get_reg_field_value_soc15(reg_value, block, reg_num, reg_name, reg_field)\
	get_reg_field_value_ex(\
		(reg_value),\
		block ## reg_num ## _ ## reg_name ## __ ## reg_field ## _MASK,\
		block ## reg_num ## _ ## reg_name ## __ ## reg_field ## __SHIFT)

#define set_reg_field_value_soc15(reg_value, value, block, reg_num, reg_name, reg_field)\
	(reg_value) = set_reg_field_value_ex(\
		(reg_value),\
		(value),\
		block ## reg_num ## _ ## reg_name ## __ ## reg_field ## _MASK,\
		block ## reg_num ## _ ## reg_name ## __ ## reg_field ## __SHIFT)

/* TODO get rid of this pos*/
static inline bool wait_reg_func(
	const struct dc_context *ctx,
	uint32_t addr,
	uint32_t mask,
	uint8_t shift,
	uint32_t condition_value,
	unsigned int interval_us,
	unsigned int timeout_us)
{
	uint32_t field_value;
	uint32_t reg_val;
	unsigned int count = 0;

	if (IS_FPGA_MAXIMUS_DC(ctx->dce_environment))
		timeout_us *= 655;  /* 6553 give about 30 second before time out */

	do {
		/* try once without sleeping */
		if (count > 0) {
			if (interval_us >= 1000)
				msleep(interval_us/1000);
			else
				udelay(interval_us);
		}
		reg_val = dm_read_reg(ctx, addr);
		field_value = get_reg_field_value_ex(reg_val, mask, shift);
		count += interval_us;

	} while (field_value != condition_value && count <= timeout_us);

	ASSERT(count <= timeout_us);

	return count <= timeout_us;
}

#define wait_reg(ctx, inst_offset, reg_name, reg_field, condition_value)\
	wait_reg_func(\
		ctx,\
		mm##reg_name + inst_offset + DCE_BASE.instance[0].segment[mm##reg_name##_BASE_IDX],\
		reg_name ## __ ## reg_field ## _MASK,\
		reg_name ## __ ## reg_field ## __SHIFT,\
		condition_value,\
		20000,\
		200000)

/**************************************
 * Power Play (PP) interfaces
 **************************************/

/* DAL calls this function to notify PP about clocks it needs for the Mode Set.
 * This is done *before* it changes DCE clock.
 *
 * If required clock is higher than current, then PP will increase the voltage.
 *
 * If required clock is lower than current, then PP will defer reduction of
 * voltage until the call to dc_service_pp_post_dce_clock_change().
 *
 * \input - Contains clocks needed for Mode Set.
 *
 * \output - Contains clocks adjusted by PP which DAL should use for Mode Set.
 *		Valid only if function returns zero.
 *
 * \returns	true - call is successful
 *		false - call failed
 */
bool dm_pp_pre_dce_clock_change(
	struct dc_context *ctx,
	struct dm_pp_gpu_clock_range *requested_state,
	struct dm_pp_gpu_clock_range *actual_state);

/* The returned clocks range are 'static' system clocks which will be used for
 * mode validation purposes.
 *
 * \returns	true - call is successful
 *		false - call failed
 */
bool dc_service_get_system_clocks_range(
	const struct dc_context *ctx,
	struct dm_pp_gpu_clock_range *sys_clks);

/* Gets valid clocks levels from pplib
 *
 * input: clk_type - display clk / sclk / mem clk
 *
 * output: array of valid clock levels for given type in ascending order,
 * with invalid levels filtered out
 *
 */
bool dm_pp_get_clock_levels_by_type(
	const struct dc_context *ctx,
	enum dm_pp_clock_type clk_type,
	struct dm_pp_clock_levels *clk_level_info);

bool dm_pp_get_clock_levels_by_type_with_latency(
	const struct dc_context *ctx,
	enum dm_pp_clock_type clk_type,
	struct dm_pp_clock_levels_with_latency *clk_level_info);

bool dm_pp_get_clock_levels_by_type_with_voltage(
	const struct dc_context *ctx,
	enum dm_pp_clock_type clk_type,
	struct dm_pp_clock_levels_with_voltage *clk_level_info);

bool dm_pp_notify_wm_clock_changes(
	const struct dc_context *ctx,
	struct dm_pp_wm_sets_with_clock_ranges *wm_with_clock_ranges);

void dm_pp_get_funcs_rv(struct dc_context *ctx,
		struct pp_smu_funcs_rv *funcs);

/* DAL calls this function to notify PP about completion of Mode Set.
 * For PP it means that current DCE clocks are those which were returned
 * by dc_service_pp_pre_dce_clock_change(), in the 'output' parameter.
 *
 * If the clocks are higher than before, then PP does nothing.
 *
 * If the clocks are lower than before, then PP reduces the voltage.
 *
 * \returns	true - call is successful
 *		false - call failed
 */
bool dm_pp_apply_display_requirements(
	const struct dc_context *ctx,
	const struct dm_pp_display_configuration *pp_display_cfg);

bool dm_pp_apply_power_level_change_request(
	const struct dc_context *ctx,
	struct dm_pp_power_level_change_request *level_change_req);

bool dm_pp_apply_clock_for_voltage_request(
	const struct dc_context *ctx,
	struct dm_pp_clock_for_voltage_req *clock_for_voltage_req);

bool dm_pp_get_static_clocks(
	const struct dc_context *ctx,
	struct dm_pp_static_clock_info *static_clk_info);

/****** end of PP interfaces ******/

struct persistent_data_flag {
	bool save_per_link;
	bool save_per_edid;
};

/* Call to write data in registry editor for persistent data storage.
 *
 * \inputs      sink - identify edid/link for registry folder creation
 *              module name - identify folders for registry
 *              key name - identify keys within folders for registry
 *              params - value to write in defined folder/key
 *              size - size of the input params
 *              flag - determine whether to save by link or edid
 *
 * \returns     true - call is successful
 *              false - call failed
 *
 * sink         module         key
 * -----------------------------------------------------------------------------
 * NULL         NULL           NULL     - failure
 * NULL         NULL           -        - create key with param value
 *                                                      under base folder
 * NULL         -              NULL     - create module folder under base folder
 * -            NULL           NULL     - failure
 * NULL         -              -        - create key under module folder
 *                                            with no edid/link identification
 * -            NULL           -        - create key with param value
 *                                                       under base folder
 * -            -              NULL     - create module folder under base folder
 * -            -              -        - create key under module folder
 *                                              with edid/link identification
 */
bool dm_write_persistent_data(struct dc_context *ctx,
		const struct dc_sink *sink,
		const char *module_name,
		const char *key_name,
		void *params,
		unsigned int size,
		struct persistent_data_flag *flag);


/* Call to read data in registry editor for persistent data storage.
 *
 * \inputs      sink - identify edid/link for registry folder creation
 *              module name - identify folders for registry
 *              key name - identify keys within folders for registry
 *              size - size of the output params
 *              flag - determine whether it was save by link or edid
 *
 * \returns     params - value read from defined folder/key
 *              true - call is successful
 *              false - call failed
 *
 * sink         module         key
 * -----------------------------------------------------------------------------
 * NULL         NULL           NULL     - failure
 * NULL         NULL           -        - read key under base folder
 * NULL         -              NULL     - failure
 * -            NULL           NULL     - failure
 * NULL         -              -        - read key under module folder
 *                                             with no edid/link identification
 * -            NULL           -        - read key under base folder
 * -            -              NULL     - failure
 * -            -              -        - read key under module folder
 *                                              with edid/link identification
 */
bool dm_read_persistent_data(struct dc_context *ctx,
		const struct dc_sink *sink,
		const char *module_name,
		const char *key_name,
		void *params,
		unsigned int size,
		struct persistent_data_flag *flag);

void dm_delay_in_microseconds
	(struct dc_context *ctx, unsigned int microSeconds);

bool dm_query_extended_brightness_caps
	(struct dc_context *ctx, enum dm_acpi_display_type display,
			struct dm_acpi_atif_backlight_caps *pCaps);

bool dm_dmcu_set_pipe(struct dc_context *ctx, unsigned int controller_id);

/*
 *
 * print-out services
 *
 */
#define dm_log_to_buffer(buffer, size, fmt, args)\
	vsnprintf(buffer, size, fmt, args)

/*
 * Debug and verification hooks
 */
bool dm_helpers_dc_conn_log(
		struct dc_context *ctx,
		struct log_entry *entry,
		enum dc_log_type event);

void dm_dtn_log_begin(struct dc_context *ctx);
void dm_dtn_log_append_v(struct dc_context *ctx, const char *msg, ...);
void dm_dtn_log_end(struct dc_context *ctx);

#endif /* __DM_SERVICES_H__ */
