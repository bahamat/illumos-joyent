/*
 * Copyright 2013 Garrett D'Amore <garrett@damore.org>
 * Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2002-2004 Tim J. Robbins.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "lint.h"
#include <wchar.h>
#include "mblocal.h"
#include "localeimpl.h"
#include "lctype.h"

size_t
mbrtowc_l(wchar_t *_RESTRICT_KYWD pwc, const char *_RESTRICT_KYWD s,
    size_t n, mbstate_t *_RESTRICT_KYWD ps, locale_t loc)
{
	static mbstate_t mbs;

	if (ps == NULL)
		ps = &mbs;
	return (loc->ctype->lc_mbrtowc(pwc, s, n, ps, B_FALSE));
}

size_t
mbrtowc(wchar_t *_RESTRICT_KYWD pwc, const char *_RESTRICT_KYWD s,
    size_t n, mbstate_t *_RESTRICT_KYWD ps)
{
	return (mbrtowc_l(pwc, s, n, ps, uselocale(NULL)));
}

size_t
mbrtowc_nz_l(wchar_t *_RESTRICT_KYWD pwc, const char *_RESTRICT_KYWD s,
    size_t n, mbstate_t *_RESTRICT_KYWD ps, locale_t loc)
{
	static mbstate_t mbs;

	if (ps == NULL)
		ps = &mbs;
	return (loc->ctype->lc_mbrtowc(pwc, s, n, ps, B_TRUE));
}

size_t
mbrtowc_nz(wchar_t *_RESTRICT_KYWD pwc, const char *_RESTRICT_KYWD s,
    size_t n, mbstate_t *_RESTRICT_KYWD ps)
{
	return (mbrtowc_nz_l(pwc, s, n, ps, uselocale(NULL)));
}
