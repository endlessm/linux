#undef TRACE_SYSTEM
#define TRACE_SYSTEM printk

#if !defined(_TRACE_PRINTK_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PRINTK_H

#include <linux/tracepoint.h>
#include <linux/version.h>

#define MSG_TRACE_MAX_LEN	2048

TRACE_EVENT_CONDITION(console,
	TP_PROTO(const char *log_buf, unsigned start, unsigned end,
		 unsigned log_buf_len),

	TP_ARGS(log_buf, start, end, log_buf_len),

	TP_CONDITION(start != end),

	TP_STRUCT__entry(
		__dynamic_array_text(char, msg,
			min_t(unsigned, end - start, MSG_TRACE_MAX_LEN) + 1)
	),

	TP_fast_assign(
		tp_memcpy_dyn(msg,
			({
				char lmsg[MSG_TRACE_MAX_LEN + 1];

				if ((end - start) > MSG_TRACE_MAX_LEN)
					start = end - MSG_TRACE_MAX_LEN;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0))
				if ((start & (log_buf_len - 1)) >
					(end & (log_buf_len - 1))) {
					memcpy(lmsg,
						log_buf +
						(start & (log_buf_len - 1)),
						log_buf_len -
						(start & (log_buf_len - 1)));
					memcpy(lmsg + log_buf_len -
						(start & (log_buf_len - 1)),
						log_buf,
						end & (log_buf_len - 1));
				} else
					memcpy(lmsg,
						log_buf +
						(start & (log_buf_len - 1)),
						end - start);
#else
				memcpy(lmsg, log_buf + start, end - start);
#endif
				lmsg[end - start] = 0;
				lmsg;
			})
		)
	),

	TP_printk("%s", __get_str(msg))
)
#endif /* _TRACE_PRINTK_H */

/* This part must be outside protection */
#include "../../../probes/define_trace.h"
