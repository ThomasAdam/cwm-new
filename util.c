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

#include <sys/param.h>
#include "queue.h"

#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

#define MAXARGLEN 20

static void	 u_append_str(char **, const char *, ...);

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
	int			 screens = 0, status_fd = -1;

	TAILQ_FOREACH(sc, &Screenq, entry)
	{
		for (screens = 0; screens < sc->xinerama_no; screens++) {
			(void)snprintf(pipe_name, sizeof(pipe_name),
					"/tmp/cwm-%d.fifo", screens);
			unlink(pipe_name);
			if ((mkfifo(pipe_name, 0666) == -1)) {
				fprintf(stderr, "mkfifo() error: %s\n",
					strerror(errno));
				continue;
			}

			status_fd = open(pipe_name, O_RDWR|O_NONBLOCK);
			if (status_fd == -1)
				fprintf(stderr, "Couldn't open pipe: %s: %s\n",
					pipe_name, strerror(errno));

			sc->status_fp[screens] = fdopen(status_fd, "w");
		}
	}
}

void
u_put_status(struct screen_ctx *sc)
{
	FILE			*status;
	struct client_ctx	*cc = client_current(), *ci;
	struct group_ctx	*gc;
	int			 screen = 0, client_count = 0;
	char			*active_groups, *all_groups, *urgencies;
	XineramaScreenInfo	*xsi = NULL;

	urgencies = all_groups = active_groups = NULL;

	if ((xsi = screen_find_ptr_xinerama()) != NULL)
		screen = xsi->screen_number;

	if ((status = sc->status_fp[screen]) == NULL)
		return;

	fprintf(status, "screen:%d|", screen);

	/* If there's no currently active client, we might be looking at an
	 * empty group---skip any client processing and instead just mark this
	 * group as active.
	 */
	if (cc == NULL)
		goto out;

	/*
	 * Go through all clients regardless of the group.  Looking at all
	 * clients will allow for catching urgent clients on other groups which
	 * might not be active.
	 */
	TAILQ_FOREACH(ci, &Clientq, entry) {
		if (ci->xinerama != cc->xinerama)
			continue;
		if (ci == cc)
			fprintf(status, "client:%s|", cc->name);
		if (ci->flags & CLIENT_URGENCY) {
			u_append_str(&urgencies, "%s,",
				sc->group_names[ci->group != NULL ?
				ci->group->shortcut : 0]);
		}
		if (ci->flags & CLIENT_HIDDEN)
			continue;
	}
out:
	/* Now go through all groups and find how many clients are on each.
	 * This is useful so that status indicators can make groups as having
	 * clients on them, if they're not the active group(s); making a
	 * distinction between empty and occupied groups, for example.
	 */
	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		client_count = 0;
		TAILQ_FOREACH(ci, &gc->clients, group_entry)
			client_count++;
		if (!gc->hidden) {
			u_append_str(&active_groups, "%s,",
				sc->group_names[gc->shortcut]);
		}
		u_append_str(&all_groups, "%s (%d) (%d),",
				sc->group_names[gc->shortcut],
				gc->shortcut, client_count);
	}
	/* Remove any trailing commas. */
	if (urgencies != NULL)
		urgencies[strlen(urgencies) - 1] = '\0';
	if (all_groups != NULL)
		all_groups[strlen(all_groups) - 1] = '\0';
	if (active_groups != NULL)
		active_groups[strlen(active_groups) - 1] = '\0';

	fprintf(status, "urgency:%s|", urgencies != NULL ? urgencies : "");
	fprintf(status, "desktops:%s|", all_groups != NULL ? all_groups : "");
	fprintf(status, "active_desktops:%s", active_groups != NULL ?
			active_groups : "");
	fprintf(status, "\n");
	fflush(status);

	free(urgencies);
	free(all_groups);
	free(active_groups);
}

static void
u_append_str(char **append, const char *fmt, ...)
{
	char		*temp, *result;
	va_list		 ap;

	va_start(ap, fmt);
	vasprintf(&temp, fmt, ap);
	va_end(ap);

	/* Big enough on the first iteration to hold the value of temp */
	if (*append == NULL)
		*append = xcalloc(1, strlen(temp) + 1);

	/* We only append the string if it's not already there. */
	if (strstr(*append, temp) == NULL)
		xasprintf(&result, "%s%s", *append, temp);
	free(temp);
	free(*append);
	*append = xstrdup(result);
}
