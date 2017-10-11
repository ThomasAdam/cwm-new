/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Marius Aamodt Eriksen <marius@monkey.org>
 * Copyright (c) 2017 Thomas Adam <thomas@xteddy.org>
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "calmwm.h"

int
load_cfg(const char *path)
{
	FILE			*f;
	const char		 delim[3] = { '\\', '\\', '\0' };
	u_int			 found = 0;
	size_t			 line = 0;
	char			*buf, *cause1, *p, *q;
	struct cmd_list		*cmdlist;
	struct cmdq_item	*new_item;

	log_debug("loading %s", path);
	if ((f = fopen(path, "rb")) == NULL) {
		log_debug("%s: %s: %s", __func__, path, strerror(errno));
		return (-1);
	}

	while ((buf = fparseln(f, NULL, &line, delim, 0)) != NULL) {
		log_debug("%s: %s: %s", __func__, path, buf);

		p = buf;
		while (isspace((u_char)*p))
			p++;
		if (*p == '\0') {
			free(buf);
			continue;
		}
		q = p + strlen(p) - 1;
		while (q != p && isspace((u_char)*q))
			*q-- = '\0';

		cmdlist = cmd_string_parse(p, path, line, &cause1);
		if (cmdlist == NULL) {
			free(buf);
			if (cause1 == NULL)
				continue;
			log_debug("%s: %s:%zu: %s", __func__, path, line, cause1);
			free(cause1);
			continue;
		}
		free(buf);

		new_item = cmdq_get_command(cmdlist, 0);
		cmdq_append(new_item);
		cmd_list_free(cmdlist);

		found++;
	}
	fclose(f);

	return (found);
}

void
config2(void)
{
	log_debug("You got me...");
}
