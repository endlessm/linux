/*
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/linkage.h>

/*
 * Copy to user space from a kernel buffer (alignment handled by the hardware)
 *
 * Parameters:
 *	x0 - to
 *	x1 - from
 *	x2 - n
 * Returns:
 *	x0 - bytes not copied
 */
ENTRY(__copy_to_user)
#include "copy_template.S"
ENDPROC(__copy_to_user)

	.section .fixup,"ax"
	.align    2
8:
10:
	/*
	 * count is accurate
	 */
	mov	x0, count
	b	.Lfinalize
9:
	/*
	 * 16 bytes remain
	 */
	mov	x0, #16
	b	.Lfinalize
11:
	/*
	 *(count + tmp2) bytes remain
	 * dst points to the start of the remaining bytes
	 */
	add	x0, count, tmp2
	b	.Lfinalize
12:
13:
	/*
	 * (count + 128) bytes remain
	 */
	add	x0, count, #128
	b	.Lfinalize
.Lfinalize:
	ret
	.previous
