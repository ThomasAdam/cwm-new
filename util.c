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

#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

#define MAXARGLEN 20

static void	 u_append_char(char **, const char *, ...);

void
u_spawn(char *argstr)
{
	switch (fork()) {
	case 0:
		u_exec(argstr);
		err(1, "%s", argstr);
		break;
	case -1:
		warn("fork");
	default:
		break;
	}
}

void
u_exec(char *argstr)
{
	char	*args[MAXARGLEN], **ap = args;
	char	**end = &args[MAXARGLEN - 1], *tmp;

	while (ap < end && (*ap = strsep(&argstr, " \t")) != NULL) {
		if (**ap == '\0')
			continue;
		ap++;
		if (argstr != NULL) {
			/* deal with quoted strings */
			switch(argstr[0]) {
			case '"':
			case '\'':
				if ((tmp = strchr(argstr + 1, argstr[0]))
				    != NULL) {
					*(tmp++) = '\0';
					*(ap++) = ++argstr;
					argstr = tmp;
				}
				break;
			default:
				break;
			}
		}
	}

	*ap = NULL;
	(void)setsid();
	(void)execvp(args[0], args);
}

void
u_init_pipes(void)
{
	struct screen_ctx	*sc;
	char			 pipe_name[PATH_MAX];
	int			 sc_num, status_fd = -1;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		snprintf(pipe_name, sizeof(pipe_name),
			"/tmp/cwm-%s.fifo", sc->name);

		unlink(pipe_name);

		if ((mkfifo(pipe_name, 0666) == -1)) {
			log_debug("mkfifo: %s", strerror(errno));
			continue;
		}

		if ((status_fd = open(pipe_name, O_RDWR|O_NONBLOCK)) == -1)
			log_debug("Couldn't open pipe '%s': %s", pipe_name,
				strerror(errno));

		sc->status_fp = fdopen(status_fd, "w");
	}
}

void
u_put_status(struct screen_ctx *sc)
{
	/* FIXME: implement this! */

	return;
}

static void
u_append(char **append, const char *fmt, ...)
{
	char	*temp, *result;
	va_list	 ap;

	va_start(ap, fmt);
	vasprintf(&temp, fmt, ap);
	va_end(ap);

	/* Big enough on the first iteration to hold the value of temp. */
	if (*append == NULL)
		*append = xcalloc(1, strlen(temp) + 1);

	/* We only append the string if it's not already there. */
	if (strstr(*append, temp) == NULL)
		xasprintf(&result, "%s%s", *append, temp);
	free(temp);
	free(*append);

	*append = xstrdup(result);
}
