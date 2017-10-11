/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Marius Aamodt Eriksen <marius@monkey.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $OpenBSD$
 */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

void *
xmalloc(size_t siz)
{
	void	*p;

	if (siz == 0)
		errx(1, "xmalloc: zero size");
	if ((p = malloc(siz)) == NULL)
		err(1, "malloc");

	return(p);
}

void *
xcalloc(size_t no, size_t siz)
{
	void	*p;

	if (siz == 0 || no == 0)
		errx(1, "xcalloc: zero size");
	if (SIZE_MAX / no < siz)
		errx(1, "xcalloc: no * siz > SIZE_MAX");
	if ((p = calloc(no, siz)) == NULL)
		err(1, "calloc");

	return(p);
}

void *
xrealloc(void *ptr, size_t size)
{
	return (xreallocarray(ptr, 1, size));
}

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
	void *new_ptr;

	if (nmemb == 0 || size == 0)
		errx(1, "xreallocarray: zero size");
	new_ptr = reallocarray(ptr, nmemb, size);
	if (new_ptr == NULL)
		errx(1, "xreallocarray: allocating %zu * %zu bytes: %s",
		    nmemb, size, strerror(errno));
	return (new_ptr);
}

char *
xstrdup(const char *str)
{
	char	*p;

	if ((p = strdup(str)) == NULL)
		err(1, "strdup");

	return(p);
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list	 ap;
	int	 i;

	va_start(ap, fmt);
	i = vasprintf(ret, fmt, ap);
	va_end(ap);

	if (i < 0 || *ret == NULL)
		err(1, "asprintf");

	return(i);
}

int
xvasprintf(char **ret, const char *fmt, va_list ap)
{
	int i;

	i = vasprintf(ret, fmt, ap);

	if (i < 0 || *ret == NULL)
		log_fatal("xasprintf: %s", strerror(errno));

	return i;
}



int
xsnprintf(char *str, size_t len, const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = xvsnprintf(str, len, fmt, ap);
	va_end(ap);

	return i;
}


int
xvsnprintf(char *str, size_t len, const char *fmt, va_list ap)
{
	int i;

	if (len > INT_MAX)
		log_fatal("xsnprintf: len > INT_MAX");

	i = vsnprintf(str, len, fmt, ap);

	if (i < 0 || i >= (int)len)
		log_fatal("xsnprintf: overflow");

	return i;
}
