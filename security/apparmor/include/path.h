/*
 * AppArmor security module
 *
 * This file contains AppArmor basic path manipulation function definitions.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_PATH_H
#define __AA_PATH_H


enum path_flags {
	PATH_IS_DIR = 0x1,		/* path is a directory */
	PATH_CONNECT_PATH = 0x4,	/* connect disconnected paths to / */
	PATH_CHROOT_REL = 0x8,		/* do path lookup relative to chroot */
	PATH_CHROOT_NSCONNECT = 0x10,	/* connect paths that are at ns root */

	PATH_DELEGATE_DELETED = 0x08000, /* delegate deleted files */
	PATH_MEDIATE_DELETED = 0x10000,	 /* mediate deleted paths */
};

int aa_path_name(struct path *path, int flags, char *buffer,
		 const char **name, const char **info);


/* Per cpu buffers used during mediation */
/* preallocated buffers to use during path lookups */
struct aa_buffers {
	char *buf[2];
};

#include <linux/percpu.h>
#include <linux/preempt.h>

DECLARE_PER_CPU(struct aa_buffers, aa_buffers);

#define COUNT_ARGS(X...) COUNT_ARGS_HELPER ( , ##X ,9,8,7,6,5,4,3,2,1,0)
#define COUNT_ARGS_HELPER(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,n,X...) n
#define CONCAT(X, Y) X ## Y
#define CONCAT_AFTER(X, Y) CONCAT(X, Y)

//#define __macroarg_counter(Y...) __macroarg_count1 ( , ##Y)
//#define __macroarg_count1(Y...) __macroarg_count2 (Y,5,4,3,2,1,0)
//#define __macroarg_count2(_,x0,x1,x2,x3,x4,n,Y...) n

#define ASSIGN(FN, X, N) do { (X) = FN(N); } while (0)
#define EVAL1(FN, X) ASSIGN(FN, X, 0) /*X = FN(0)*/
#define EVAL2(FN, X, Y...) ASSIGN(FN, X, 1); /*X = FN(1);*/ EVAL1(FN, Y)
//#define EVAL(FN, X...) CONCAT_AFTER(EVAL, __macroarg_counter(X))(FN, X)
#define EVAL(FN, X...) CONCAT_AFTER(EVAL, COUNT_ARGS(X))(FN, X)

#define for_each_cpu_buffer(I) for ((I) = 0; (I) < 2; (I)++)

#ifdef CONFIG_DEBUG_PREEMPT
#define AA_BUG_PREEMPT_ENABLED(X) AA_BUG(preempt_count() <= 0, X)
#else
#define AA_BUG_PREEMPT_ENABLED(X) /* nop */
#endif

#define __get_buffer(N) ({					\
	struct aa_buffers *__cpu_var; \
	AA_BUG_PREEMPT_ENABLED("__get_buffer without preempt disabled");  \
	__cpu_var = &__get_cpu_var(aa_buffers);			\
        __cpu_var->buf[(N)]; })

#define __get_buffers(X...)		\
do {					\
	EVAL(__get_buffer, X);		\
} while (0)

#define get_buffers(X...)	\
do {				\
	preempt_disable();	\
	__get_buffers(X);	\
} while (0)

#define put_buffers(X, Y...)	\
do {				\
	(void)&(X);		\
	preempt_enable();	\
} while (0)

#endif /* __AA_PATH_H */
