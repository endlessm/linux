/*
 * Copyright 2012-15 Advanced Micro Devices, Inc.
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
#include "dm_services.h"
#include "include/logger_interface.h"
#include "logger.h"


#define NUM_ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

static const struct dc_log_type_info log_type_info_tbl[] = {
		{LOG_ERROR,                 "Error"},
		{LOG_WARNING,               "Warning"},
		{LOG_DEBUG,		    "Debug"},
		{LOG_DC,                    "DC_Interface"},
		{LOG_SURFACE,               "Surface"},
		{LOG_HW_HOTPLUG,            "HW_Hotplug"},
		{LOG_HW_LINK_TRAINING,      "HW_LKTN"},
		{LOG_HW_SET_MODE,           "HW_Mode"},
		{LOG_HW_RESUME_S3,          "HW_Resume"},
		{LOG_HW_AUDIO,              "HW_Audio"},
		{LOG_HW_HPD_IRQ,            "HW_HPDIRQ"},
		{LOG_MST,                   "MST"},
		{LOG_SCALER,                "Scaler"},
		{LOG_BIOS,                  "BIOS"},
		{LOG_BANDWIDTH_CALCS,       "BWCalcs"},
		{LOG_BANDWIDTH_VALIDATION,  "BWValidation"},
		{LOG_I2C_AUX,               "I2C_AUX"},
		{LOG_SYNC,                  "Sync"},
		{LOG_BACKLIGHT,             "Backlight"},
		{LOG_FEATURE_OVERRIDE,      "Override"},
		{LOG_DETECTION_EDID_PARSER, "Edid"},
		{LOG_DETECTION_DP_CAPS,     "DP_Caps"},
		{LOG_RESOURCE,              "Resource"},
		{LOG_DML,                   "DML"},
		{LOG_EVENT_MODE_SET,        "Mode"},
		{LOG_EVENT_DETECTION,       "Detect"},
		{LOG_EVENT_LINK_TRAINING,   "LKTN"},
		{LOG_EVENT_LINK_LOSS,       "LinkLoss"},
		{LOG_EVENT_UNDERFLOW,       "Underflow"},
		{LOG_IF_TRACE,              "InterfaceTrace"},
		{LOG_DTN,                   "DTN"}
};


#define DC_DEFAULT_LOG_MASK ((1 << LOG_ERROR) | \
		(1 << LOG_WARNING) | \
		(1 << LOG_EVENT_MODE_SET) | \
		(1 << LOG_EVENT_DETECTION) | \
		(1 << LOG_EVENT_LINK_TRAINING) | \
		(1 << LOG_EVENT_LINK_LOSS) | \
		(1 << LOG_EVENT_UNDERFLOW) | \
		(1 << LOG_RESOURCE) | \
		(1 << LOG_FEATURE_OVERRIDE) | \
		(1 << LOG_DETECTION_EDID_PARSER) | \
		(1 << LOG_DC) | \
		(1 << LOG_HW_HOTPLUG) | \
		(1 << LOG_HW_SET_MODE) | \
		(1 << LOG_HW_RESUME_S3) | \
		(1 << LOG_HW_HPD_IRQ) | \
		(1 << LOG_SYNC) | \
		(1 << LOG_BANDWIDTH_VALIDATION) | \
		(1 << LOG_MST) | \
		(1 << LOG_DETECTION_DP_CAPS) | \
		(1 << LOG_BACKLIGHT)) | \
		(1 << LOG_I2C_AUX) | \
		(1 << LOG_IF_TRACE) | \
		(1 << LOG_DTN) /* | \
		(1 << LOG_DEBUG) | \
		(1 << LOG_BIOS) | \
		(1 << LOG_SURFACE) | \
		(1 << LOG_SCALER) | \
		(1 << LOG_DML) | \
		(1 << LOG_HW_LINK_TRAINING) | \
		(1 << LOG_HW_AUDIO)| \
		(1 << LOG_BANDWIDTH_CALCS)*/

/* ----------- Object init and destruction ----------- */
static bool construct(struct dc_context *ctx, struct dal_logger *logger)
{
	/* malloc buffer and init offsets */
	logger->log_buffer_size = DAL_LOGGER_BUFFER_MAX_SIZE;
	logger->log_buffer = (char *)dm_alloc(logger->log_buffer_size *
		sizeof(char));

	if (!logger->log_buffer)
		return false;

	/* Initialize both offsets to start of buffer (empty) */
	logger->buffer_read_offset = 0;
	logger->buffer_write_offset = 0;

	logger->write_wrap_count = 0;
	logger->read_wrap_count = 0;
	logger->open_count = 0;

	logger->flags.bits.ENABLE_CONSOLE = 1;
	logger->flags.bits.ENABLE_BUFFER = 0;

	logger->ctx = ctx;

	logger->mask = DC_DEFAULT_LOG_MASK;

	return true;
}

static void destruct(struct dal_logger *logger)
{
	if (logger->log_buffer) {
		dm_free(logger->log_buffer);
		logger->log_buffer = NULL;
	}
}

struct dal_logger *dal_logger_create(struct dc_context *ctx)
{
	/* malloc struct */
	struct dal_logger *logger = dm_alloc(sizeof(struct dal_logger));

	if (!logger)
		return NULL;
	if (!construct(ctx, logger)) {
		dm_free(logger);
		return NULL;
	}

	return logger;
}

uint32_t dal_logger_destroy(struct dal_logger **logger)
{
	if (logger == NULL || *logger == NULL)
		return 1;
	destruct(*logger);
	dm_free(*logger);
	*logger = NULL;

	return 0;
}

/* ------------------------------------------------------------------------ */


static bool dal_logger_should_log(
	struct dal_logger *logger,
	enum dc_log_type log_type)
{
	if (logger->mask & (1 << log_type))
		return true;

	return false;
}

static void log_to_debug_console(struct log_entry *entry)
{
	struct dal_logger *logger = entry->logger;

	if (logger->flags.bits.ENABLE_CONSOLE == 0)
		return;

	if (entry->buf_offset) {
		switch (entry->type) {
		case LOG_ERROR:
			dm_error("%s", entry->buf);
			break;
		default:
			dm_output_to_console("%s", entry->buf);
			break;
		}
	}
}

/* Print everything unread existing in log_buffer to debug console*/
static void flush_to_debug_console(struct dal_logger *logger)
{
	int i = logger->buffer_read_offset;
	char *string_start = &logger->log_buffer[i];

	dm_output_to_console(
		"---------------- FLUSHING LOG BUFFER ----------------\n");
	while (i < logger->buffer_write_offset)	{

		if (logger->log_buffer[i] == '\0') {
			dm_output_to_console("%s", string_start);
			string_start = (char *)logger->log_buffer + i + 1;
		}
		i++;
	}
	dm_output_to_console(
		"-------------- END FLUSHING LOG BUFFER --------------\n\n");
}

static void log_to_internal_buffer(struct log_entry *entry)
{

	uint32_t size = entry->buf_offset;
	struct dal_logger *logger = entry->logger;

	if (logger->flags.bits.ENABLE_BUFFER == 0)
		return;

	if (logger->log_buffer == NULL)
		return;

	if (size > 0 && size < logger->log_buffer_size) {

		int total_free_space = 0;
		int space_before_wrap = 0;

		if (logger->buffer_write_offset > logger->buffer_read_offset) {
			total_free_space = logger->log_buffer_size -
					logger->buffer_write_offset +
					logger->buffer_read_offset;
			space_before_wrap = logger->log_buffer_size -
					logger->buffer_write_offset;
		} else if (logger->buffer_write_offset <
				logger->buffer_read_offset) {
			total_free_space = logger->log_buffer_size -
					logger->buffer_read_offset +
					logger->buffer_write_offset;
			space_before_wrap = total_free_space;
		} else if (logger->write_wrap_count !=
				logger->read_wrap_count) {
			/* Buffer is completely full already */
			total_free_space = 0;
			space_before_wrap = 0;
		} else {
			/* Buffer is empty, start writing at beginning */
			total_free_space = logger->log_buffer_size;
			space_before_wrap = logger->log_buffer_size;
			logger->buffer_write_offset = 0;
			logger->buffer_read_offset = 0;
		}

		if (space_before_wrap > size) {
			/* No wrap around, copy 'size' bytes
			 * from 'entry->buf' to 'log_buffer'
			 */
			memmove(logger->log_buffer +
					logger->buffer_write_offset,
					entry->buf, size);
			logger->buffer_write_offset += size;

		} else if (total_free_space > size) {
			/* We have enough room without flushing,
			 * but need to wrap around */

			int space_after_wrap = total_free_space -
					space_before_wrap;

			memmove(logger->log_buffer +
					logger->buffer_write_offset,
					entry->buf, space_before_wrap);
			memmove(logger->log_buffer, entry->buf +
					space_before_wrap, space_after_wrap);

			logger->buffer_write_offset = space_after_wrap;
			logger->write_wrap_count++;

		} else {
			/* Not enough room remaining, we should flush
			 * existing logs */

			/* Flush existing unread logs to console */
			flush_to_debug_console(logger);

			/* Start writing to beginning of buffer */
			memmove(logger->log_buffer, entry->buf, size);
			logger->buffer_write_offset = size;
			logger->buffer_read_offset = 0;
		}

	}
}

static void log_heading(struct log_entry *entry)
{
	int j;

	for (j = 0; j < NUM_ELEMENTS(log_type_info_tbl); j++) {

		const struct dc_log_type_info *info = &log_type_info_tbl[j];

		if (info->type == entry->type)
			dm_logger_append(entry, "[%s]\t", info->name);
	}
}

static void append_entry(
		struct log_entry *entry,
		char *buffer,
		uint32_t buf_size)
{
	if (!entry->buf ||
		entry->buf_offset + buf_size > entry->max_buf_bytes
	) {
		BREAK_TO_DEBUGGER();
		return;
	}

	/* Todo: check if off by 1 byte due to \0 anywhere */
	memmove(entry->buf + entry->buf_offset, buffer, buf_size);
	entry->buf_offset += buf_size;
}

/* ------------------------------------------------------------------------ */

/* Warning: Be careful that 'msg' is null terminated and the total size is
 * less than DAL_LOGGER_BUFFER_MAX_LOG_LINE_SIZE (256) including '\0'
 */
void dm_logger_write(
	struct dal_logger *logger,
	enum dc_log_type log_type,
	const char *msg,
	...)
{
	if (logger && dal_logger_should_log(logger, log_type)) {
		uint32_t size;
		va_list args;
		char buffer[LOG_MAX_LINE_SIZE];
		struct log_entry entry;

		va_start(args, msg);

		entry.logger = logger;

		entry.buf = buffer;

		entry.buf_offset = 0;
		entry.max_buf_bytes = DAL_LOGGER_BUFFER_MAX_SIZE * sizeof(char);

		entry.type = log_type;

		log_heading(&entry);

		size = dm_log_to_buffer(
			buffer, LOG_MAX_LINE_SIZE, msg, args);

		entry.buf_offset += size;

		/* --Flush log_entry buffer-- */
		/* print to kernel console */
		log_to_debug_console(&entry);
		/* log internally for dsat */
		log_to_internal_buffer(&entry);

		va_end(args);
	}
}

/* Same as dm_logger_write, except without open() and close(), which must
 * be done separately.
 */
void dm_logger_append(
	struct log_entry *entry,
	const char *msg,
	...)
{
	struct dal_logger *logger;

	if (!entry) {
		BREAK_TO_DEBUGGER();
		return;
	}

	logger = entry->logger;

	if (logger && logger->open_count > 0 &&
		dal_logger_should_log(logger, entry->type)) {

		uint32_t size;
		va_list args;
		char buffer[LOG_MAX_LINE_SIZE];

		va_start(args, msg);

		size = dm_log_to_buffer(
			buffer, LOG_MAX_LINE_SIZE, msg, args);

		if (size < LOG_MAX_LINE_SIZE - 1) {
			append_entry(entry, buffer, size);
		} else {
			append_entry(entry, "LOG_ERROR, line too long\n", 27);
		}

		va_end(args);
	}
}

void dm_logger_open(
		struct dal_logger *logger,
		struct log_entry *entry, /* out */
		enum dc_log_type log_type)
{
	if (!entry) {
		BREAK_TO_DEBUGGER();
		return;
	}

	entry->type = log_type;
	entry->logger = logger;

	entry->buf = dm_alloc(DAL_LOGGER_BUFFER_MAX_SIZE * sizeof(char));

	entry->buf_offset = 0;
	entry->max_buf_bytes = DAL_LOGGER_BUFFER_MAX_SIZE * sizeof(char);

	logger->open_count++;

	log_heading(entry);
}

void dm_logger_close(struct log_entry *entry)
{
	struct dal_logger *logger = entry->logger;

	if (logger && logger->open_count > 0) {
		logger->open_count--;
	} else {
		BREAK_TO_DEBUGGER();
		goto cleanup;
	}

	/* --Flush log_entry buffer-- */
	/* print to kernel console */
	log_to_debug_console(entry);
	/* log internally for dsat */
	log_to_internal_buffer(entry);

	/* TODO: Write end heading */

cleanup:
	if (entry->buf) {
		dm_free(entry->buf);
		entry->buf = NULL;
		entry->buf_offset = 0;
		entry->max_buf_bytes = 0;
	}
}
