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
		for (screens = 0; sc->xinerama != NULL ? screens < sc->xinerama_no : screens < 1; screens++) {
			(void)sprintf(pipe_name, "/tmp/cwm-%d.sock",
					screens);
			unlink(pipe_name);
			fprintf(stderr, "Creating pipe: %s\n", pipe_name);
			if ((mkfifo(pipe_name, 0666) == -1)) {
				fprintf(stderr, "mkfifo(): %s\n",
					strerror(errno));
				continue;
			}

			status_fd = open(pipe_name, O_RDWR|O_NONBLOCK);
			if (status_fd == -1)
				fprintf(stderr, "Eh?  %s\n", strerror(errno));

			sc->status_fp[screens] = fdopen(status_fd, "w");
		}
	}
}

void
u_put_status(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	FILE			*status;
	struct client_ctx	*ci;
	int			 screen = 0;
	char			**g_name;
	char			 group_name[1024], urgency_desk[1024];
	char			 groups[8192], urgencies[8192];

	if (cc->xinerama != NULL) {
		(void)screen_find_xinerama(sc,
			cc->geom.x + cc->geom.w / 2,
			cc->geom.y + cc->geom.h / 2, CWM_NOGAP, &screen);
	}
	if ((status = sc->status_fp[screen]) == NULL)
		return;

	memset(groups, '\0', sizeof(groups));

	fprintf(status, "screen:%d|", screen);

	for (g_name = sc->group_names; *g_name != NULL; g_name++) {
		(void)snprintf(group_name, sizeof(group_name),
				"%s,", *g_name);
		strlcat(groups, group_name, sizeof(group_name));
	}
	/* Truncate the final comma. */
	groups[strlen(groups) - 1] = '\0';
	fprintf(status, "desktops:%s|", groups);

	memset(groups, '\0', sizeof(groups));
	memset(group_name, '\0', sizeof(groups));
	memset(urgency_desk, '\0', sizeof(urgency_desk));
	memset(urgencies, '\0', sizeof(urgencies));

	/*
	 * Go through all clients regardless of the group, because we want to
	 * look at all clients due to the fact that it's possible for more than
	 * one group to be present on the screen at once.  Looking at all
	 * clients therefore will determine the group it is in, and hence which
	 * ones are currently being displayed.
	 */
	TAILQ_FOREACH(ci, &Clientq, entry) {
		if (ci->xinerama != cc->xinerama)
			continue;
		if (ci == cc)
			fprintf(status, "client:%s|", cc->name);
		if (ci->flags & CLIENT_URGENCY) {
			(void)snprintf(urgency_desk, sizeof(urgency_desk),
				"%s,",
				sc->group_names[ci->group != NULL ?
				ci->group->shortcut : 0]);
			/* XXX: Got to be a better way than this! */
			if (strstr(urgencies, urgency_desk) == NULL)
				strlcat(urgencies, urgency_desk,
					sizeof(urgency_desk));
		}
		if (ci->flags & CLIENT_HIDDEN)
			continue;
		(void)snprintf(group_name, sizeof(group_name), "%s,",
			sc->group_names[ci->group != NULL ?
			ci->group->shortcut : 0]);
		/* XXX: Got to be a better way than this! */
		if (strstr(groups, group_name) == NULL)
			strlcat(groups, group_name, sizeof(group_name));
	}
	/* Truncate the final comma. */
	groups[strlen(groups) - 1] = '\0';
	urgencies[strlen(urgencies) - 1] = '\0';
	fprintf(status, "urgency:%s|", urgencies);
	fprintf(status, "active_desktops:%s", groups);
	fprintf(status, "\n");
	fflush(status);
}
