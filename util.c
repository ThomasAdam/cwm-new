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
#include "parson.h"

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
u_init_pipes(struct screen_ctx *sc)
{
	char			 pipe_name[PATH_MAX];
	int			 status_fd = -1;

	snprintf(pipe_name, sizeof(pipe_name),
		"/tmp/cwm-%s.fifo", sc->name);

	unlink(pipe_name);

	if ((mkfifo(pipe_name, 0666) == -1)) {
		log_debug("mkfifo: %s", strerror(errno));
		return;
	}

	if ((status_fd = open(pipe_name, O_RDWR|O_NONBLOCK)) == -1)
		log_debug("Couldn't open pipe '%s': %s", pipe_name,
			strerror(errno));

	sc->status_fp = fdopen(status_fd, "w");
}

void
u_put_status(struct screen_ctx *sc)
{
	struct client_ctx	*cc = client_current(), *ci;
	struct group_ctx	*gc;
	JSON_Value		*json_root = json_value_init_object();
	JSON_Object		*json_obj = json_object(json_root);
	JSON_Array		*clients = NULL;
	char			*json_out_str;
	char			 key[1024];

	json_object_set_number(json_obj, "version", 1);
	json_object_set_string(json_obj, "screen", sc->name);
	json_object_set_string(json_obj, "current_client",
				cc != NULL ? cc->name : NULL);
	json_object_set_value(json_obj, "groups", json_value_init_object());

	/* Go through all groups, and all clients on those groups, and report
	 * back information about the state of both.
	 */
	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		snprintf(key, sizeof key, "groups.%s.%s", gc->name, "number");
		json_object_dotset_number(json_obj, key, gc->num);

		snprintf(key, sizeof key, "groups.%s.%s", gc->name, "clients");
		json_object_dotset_value(json_obj, key, json_value_init_array());
		clients = json_object_dotget_array(json_obj, key);

		TAILQ_FOREACH(ci, &gc->clientq, group_entry) {
			json_array_append_string(clients, ci->name);
			snprintf(key, sizeof key, "groups.%s.%s", gc->name,
					"is_urgent");
			json_object_dotset_boolean(json_obj, key,
					ci->flags & CLIENT_URGENCY);
		}

		snprintf(key, sizeof key, "groups.%s.%s", gc->name,
					"is_active");
		json_object_dotset_boolean(json_obj, key,
					gc->flags & GROUP_ACTIVE);

		snprintf(key, sizeof key, "groups.%s.%s", gc->name,
					"is_current");
		json_object_dotset_boolean(json_obj, key,
					gc == sc->group_current);

		snprintf(key, sizeof key, "groups.%s.%s", gc->name,
					"number_of_clients");
		json_object_dotset_number(json_obj, key,
					json_array_get_count(clients));
	}

	json_out_str = json_serialize_to_string(json_root);
	fprintf(sc->status_fp, "%s\n", json_out_str);
	fflush(sc->status_fp);
}
