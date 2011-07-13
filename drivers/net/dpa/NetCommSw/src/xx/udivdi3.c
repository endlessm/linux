/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <config/modversions.h>
#endif /* MODVERSIONS */

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/div64.h>


#define BITS_PER_UNIT  8
#define SI_TYPE_SIZE (sizeof (SItype) * BITS_PER_UNIT)


typedef unsigned int UQItype    __attribute__ ((mode (QI)));
typedef          int SItype     __attribute__ ((mode (SI)));
typedef unsigned int USItype    __attribute__ ((mode (SI)));
typedef          int DItype     __attribute__ ((mode (DI)));
typedef          int word_type  __attribute__ ((mode (__word__)));
typedef unsigned int UDItype    __attribute__ ((mode (DI)));

struct DIstruct {SItype low, high;};

typedef union
{
  struct DIstruct s;
  DItype ll;
} DIunion;


/* bit divisor, dividend and result. dynamic precision */
static __inline__ uint64_t _div64_64(uint64_t dividend, uint64_t divisor)
{
    uint32_t d = divisor;

    if (divisor > 0xffffffffULL)
    {
        unsigned int shift = fls(divisor >> 32);

        d = divisor >> shift;
        dividend >>= shift;
    }

    /* avoid 64 bit division if possible */
    if (dividend >> 32)
        do_div(dividend, d);
    else
        dividend = (uint32_t) dividend / d;

    return dividend;
}

UDItype __udivdi3 (UDItype n, UDItype d)
{
  return _div64_64(n, d);
}

DItype __divdi3 (DItype n, DItype d)
{
  DItype sign = 1;
  if (n<0)
  {
    sign *= -1;
    n *= -1;
  }
  if (d<0)
  {
    sign *= -1;
    d *= -1;
  }
  return sign*_div64_64((UDItype)n, (UDItype)d);
}

UDItype __umoddi3 (UDItype n, UDItype d)
{
  return n-(_div64_64(n, d)*d);
}

#ifdef MODULE
word_type __ucmpdi2 (DItype a, DItype b)
{
  DIunion au, bu;

  au.ll = a, bu.ll = b;

  if ((USItype) au.s.high < (USItype) bu.s.high)
    return 0;
  else if ((USItype) au.s.high > (USItype) bu.s.high)
    return 2;
  if ((USItype) au.s.low < (USItype) bu.s.low)
    return 0;
  else if ((USItype) au.s.low > (USItype) bu.s.low)
    return 2;
  return 1;
}
#endif /* MODULE */
