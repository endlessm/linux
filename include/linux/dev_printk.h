// SPDX-License-Identifier: GPL-2.0
/*
 * dev_printk.h - printk messages helpers for devices
 *
 * Copyright (c) 2001-2003 Patrick Mochel <mochel@osdl.org>
 * Copyright (c) 2004-2009 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2008-2009 Novell Inc.
 *
 */

#ifndef _DEVICE_PRINTK_H_
#define _DEVICE_PRINTK_H_

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/ratelimit.h>

#ifndef dev_fmt
#define dev_fmt(fmt) fmt
#endif

struct device;

#define PRINTK_INFO_SUBSYSTEM_LEN	16
#define PRINTK_INFO_DEVICE_LEN		48

struct dev_printk_info {
	char subsystem[PRINTK_INFO_SUBSYSTEM_LEN];
	char device[PRINTK_INFO_DEVICE_LEN];
};

#ifdef CONFIG_PRINTK

#if defined(__KMSG_CHECKER) && defined(KMSG_COMPONENT)

/* generate magic string for scripts/kmsg-doc to parse */
#define _dev_emerg(dev, format, arg...)		\
	__KMSG_DEV(KERN_EMERG _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_alert(dev, format, arg...)		\
	__KMSG_DEV(KERN_ALERT _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_crit(dev, format, arg...)		\
	__KMSG_DEV(KERN_CRIT _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_err(dev, format, arg...)		\
	__KMSG_DEV(KERN_ERR _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_warn(dev, format, arg...)		\
	__KMSG_DEV(KERN_WARNING _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_notice(dev, format, arg...)		\
	__KMSG_DEV(KERN_NOTICE _FMT_ format _ARGS_ dev, ## arg _END_)
#define _dev_info(dev, format, arg...)		\
	__KMSG_DEV(KERN_INFO _FMT_ format _ARGS_ dev, ## arg _END_)

#elif defined(CONFIG_KMSG_IDS) && defined(KMSG_COMPONENT)

extern int dev_printk_hash(const char *level, const struct device *dev,
			   const char *fmt, ...);
extern __printf(2,3)
int dev_emerg_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int dev_alert_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int dev_crit_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int dev_err_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int dev_warn_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int dev_notice_hash(const struct device *dev, const char *fmt, ...);
extern __printf(2,3)
int _dev_info_hash(const struct device *dev, const char *fmt, ...);

#define dev_printk(level, dev, format, arg...)				\
	dev_printk_hash(level, dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_emerg(dev, format, arg...) \
	dev_emerg_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_alert(dev, format, arg...) \
	dev_alert_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_crit(dev, format, arg...) \
	dev_crit_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_err(dev, format, arg...) \
	dev_err_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_warn(dev, format, arg...) \
	dev_warn_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_notice(dev, format, arg...) \
	dev_notice_hash(dev, "%s: " format, dev_name(dev), ## arg)
#define _dev_info(dev, format, arg...) \
	_dev_info_hash(dev, "%s: " format, dev_name(dev), ## arg)

#else /* !defined(CONFIG_KMSG_IDS) */

__printf(3, 0) __cold
int dev_vprintk_emit(int level, const struct device *dev,
		     const char *fmt, va_list args);
__printf(3, 4) __cold
int dev_printk_emit(int level, const struct device *dev, const char *fmt, ...);

__printf(3, 4) __cold
void dev_printk(const char *level, const struct device *dev,
		const char *fmt, ...);
__printf(2, 3) __cold
void _dev_emerg(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_alert(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_crit(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_err(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_warn(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_notice(const struct device *dev, const char *fmt, ...);
__printf(2, 3) __cold
void _dev_info(const struct device *dev, const char *fmt, ...);

#endif /* !defined(CONFIG_KMSG_IDS) */

#else  /* !defined(CONFIG_PRINTK) */

static inline __printf(3, 0)
int dev_vprintk_emit(int level, const struct device *dev,
		     const char *fmt, va_list args)
{ return 0; }
static inline __printf(3, 4)
int dev_printk_emit(int level, const struct device *dev, const char *fmt, ...)
{ return 0; }

static inline void __dev_printk(const char *level, const struct device *dev,
				struct va_format *vaf)
{}
static inline __printf(3, 4)
void dev_printk(const char *level, const struct device *dev,
		 const char *fmt, ...)
{}

static inline __printf(2, 3)
void _dev_emerg(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_crit(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_alert(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_err(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_warn(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_notice(const struct device *dev, const char *fmt, ...)
{}
static inline __printf(2, 3)
void _dev_info(const struct device *dev, const char *fmt, ...)
{}

#endif /* !defined(CONFIG_PRINTK) */

/*
 * #defines for all the dev_<level> macros to prefix with whatever
 * possible use of #define dev_fmt(fmt) ...
 */

#define dev_emerg(dev, fmt, ...)					\
	_dev_emerg(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_crit(dev, fmt, ...)						\
	_dev_crit(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_alert(dev, fmt, ...)					\
	_dev_alert(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_err(dev, fmt, ...)						\
	_dev_err(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...)						\
	_dev_warn(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_notice(dev, fmt, ...)					\
	_dev_notice(dev, dev_fmt(fmt), ##__VA_ARGS__)
#define dev_info(dev, fmt, ...)						\
	_dev_info(dev, dev_fmt(fmt), ##__VA_ARGS__)

#if defined(CONFIG_DYNAMIC_DEBUG) || \
	(defined(CONFIG_DYNAMIC_DEBUG_CORE) && defined(DYNAMIC_DEBUG_MODULE))
#define dev_dbg(dev, fmt, ...)						\
	dynamic_dev_dbg(dev, dev_fmt(fmt), ##__VA_ARGS__)
#elif defined(DEBUG)
#define dev_dbg(dev, fmt, ...)						\
	dev_printk(KERN_DEBUG, dev, dev_fmt(fmt), ##__VA_ARGS__)
#else
#define dev_dbg(dev, fmt, ...)						\
({									\
	if (0)								\
		dev_printk(KERN_DEBUG, dev, dev_fmt(fmt), ##__VA_ARGS__); \
})
#endif

#ifdef CONFIG_PRINTK
#define dev_level_once(dev_level, dev, fmt, ...)			\
do {									\
	static bool __print_once __read_mostly;				\
									\
	if (!__print_once) {						\
		__print_once = true;					\
		dev_level(dev, fmt, ##__VA_ARGS__);			\
	}								\
} while (0)
#else
#define dev_level_once(dev_level, dev, fmt, ...)			\
do {									\
	if (0)								\
		dev_level(dev, fmt, ##__VA_ARGS__);			\
} while (0)
#endif

#define dev_emerg_once(dev, fmt, ...)					\
	dev_level_once(dev_emerg, dev, fmt, ##__VA_ARGS__)
#define dev_alert_once(dev, fmt, ...)					\
	dev_level_once(dev_alert, dev, fmt, ##__VA_ARGS__)
#define dev_crit_once(dev, fmt, ...)					\
	dev_level_once(dev_crit, dev, fmt, ##__VA_ARGS__)
#define dev_err_once(dev, fmt, ...)					\
	dev_level_once(dev_err, dev, fmt, ##__VA_ARGS__)
#define dev_warn_once(dev, fmt, ...)					\
	dev_level_once(dev_warn, dev, fmt, ##__VA_ARGS__)
#define dev_notice_once(dev, fmt, ...)					\
	dev_level_once(dev_notice, dev, fmt, ##__VA_ARGS__)
#define dev_info_once(dev, fmt, ...)					\
	dev_level_once(dev_info, dev, fmt, ##__VA_ARGS__)
#define dev_dbg_once(dev, fmt, ...)					\
	dev_level_once(dev_dbg, dev, fmt, ##__VA_ARGS__)

#define dev_level_ratelimited(dev_level, dev, fmt, ...)			\
do {									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
	if (__ratelimit(&_rs))						\
		dev_level(dev, fmt, ##__VA_ARGS__);			\
} while (0)

#define dev_emerg_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_emerg, dev, fmt, ##__VA_ARGS__)
#define dev_alert_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_alert, dev, fmt, ##__VA_ARGS__)
#define dev_crit_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_crit, dev, fmt, ##__VA_ARGS__)
#define dev_err_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_err, dev, fmt, ##__VA_ARGS__)
#define dev_warn_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_warn, dev, fmt, ##__VA_ARGS__)
#define dev_notice_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_notice, dev, fmt, ##__VA_ARGS__)
#define dev_info_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_info, dev, fmt, ##__VA_ARGS__)
#if defined(CONFIG_DYNAMIC_DEBUG) || \
	(defined(CONFIG_DYNAMIC_DEBUG_CORE) && defined(DYNAMIC_DEBUG_MODULE))
/* descriptor check is first to prevent flooding with "callbacks suppressed" */
#define dev_dbg_ratelimited(dev, fmt, ...)				\
do {									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
	DEFINE_DYNAMIC_DEBUG_METADATA(descriptor, fmt);			\
	if (DYNAMIC_DEBUG_BRANCH(descriptor) &&				\
	    __ratelimit(&_rs))						\
		__dynamic_dev_dbg(&descriptor, dev, dev_fmt(fmt),	\
				  ##__VA_ARGS__);			\
} while (0)
#elif defined(DEBUG)
#define dev_dbg_ratelimited(dev, fmt, ...)				\
do {									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
	if (__ratelimit(&_rs))						\
		dev_printk(KERN_DEBUG, dev, dev_fmt(fmt), ##__VA_ARGS__); \
} while (0)
#else
#define dev_dbg_ratelimited(dev, fmt, ...)				\
do {									\
	if (0)								\
		dev_printk(KERN_DEBUG, dev, dev_fmt(fmt), ##__VA_ARGS__); \
} while (0)
#endif

#ifdef VERBOSE_DEBUG
#define dev_vdbg	dev_dbg
#else
#define dev_vdbg(dev, fmt, ...)						\
({									\
	if (0)								\
		dev_printk(KERN_DEBUG, dev, dev_fmt(fmt), ##__VA_ARGS__); \
})
#endif

/*
 * dev_WARN*() acts like dev_printk(), but with the key difference of
 * using WARN/WARN_ONCE to include file/line information and a backtrace.
 */
#define dev_WARN(dev, format, arg...) \
	WARN(1, "%s %s: " format, dev_driver_string(dev), dev_name(dev), ## arg);

#define dev_WARN_ONCE(dev, condition, format, arg...) \
	WARN_ONCE(condition, "%s %s: " format, \
			dev_driver_string(dev), dev_name(dev), ## arg)

#endif /* _DEVICE_PRINTK_H_ */
