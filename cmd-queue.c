/*
 * Copyright (c) 2013-2017 Nicholas Marriott <nicm@users.sourceforge.net>
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
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "calmwm.h"

/* Global command queue. */
static struct cmdq_list global_queue = TAILQ_HEAD_INITIALIZER(global_queue);

/* Get command queue name. */
static const char *
cmdq_name(void)
{
	return ("<global>");
}

/* Get command queue from client. */
static struct cmdq_list *
cmdq_get(void)
{
	return (&global_queue);
}

/* Append an item. */
void
cmdq_append(struct cmdq_item *item)
{
	struct cmdq_list	*queue = cmdq_get();
	struct cmdq_item	*next;

	do {
		next = item->next;
		item->next = NULL;
		item->queue = queue;
		TAILQ_INSERT_TAIL(queue, item, entry);

		item = next;
	} while (item != NULL);
}

/* Insert an item. */
void
cmdq_insert_after(struct cmdq_item *after, struct cmdq_item *item)
{
	struct cmdq_list	*queue = after->queue;
	struct cmdq_item	*next;

	do {
		next = item->next;
		item->next = NULL;

		item->queue = queue;
		if (after->next != NULL)
			TAILQ_INSERT_AFTER(queue, after->next, item, entry);
		else
			TAILQ_INSERT_AFTER(queue, after, item, entry);
		after->next = item;

		item = next;
	} while (item != NULL);
}

/* Remove an item. */
static void
cmdq_remove(struct cmdq_item *item)
{
	if (item->type == CMDQ_COMMAND)
		cmd_list_free(item->cmdlist);

	TAILQ_REMOVE(item->queue, item, entry);

	free((void *)item->name);
	free(item);
}

/* Get a command for the command queue. */
struct cmdq_item *
cmdq_get_command(struct cmd_list *cmdlist, int flags)
{
	struct cmdq_item	*item, *first = NULL, *last = NULL;
	struct cmd		*cmd;
	char			*tmp;

	TAILQ_FOREACH(cmd, &cmdlist->list, qentry) {
		xasprintf(&tmp, "command[%s]", cmd->entry->name);

		item = xcalloc(1, sizeof *item);
		item->name = tmp;
		item->cmdlist = cmdlist;
		item->cmd = cmd;

		if (first == NULL)
			first = item;
		if (last != NULL)
			last->next = item;
		last = item;
	}
	return (first);
}

/* Fire command on command queue. */
static enum cmd_retval
cmdq_fire_command(struct cmdq_item *item)
{
	struct cmd		*cmd = item->cmd;
	const struct cmd_entry	*entry = cmd->entry;

	return (entry->exec(cmd, item));
}

/* Process next item on command queue. */
u_int
cmdq_next(void)
{
	struct cmdq_list	*queue = cmdq_get();
	const char		*name = cmdq_name();
	struct cmdq_item	*item;
	u_int			 items = 0;
	static u_int		 number;

	if (TAILQ_EMPTY(queue)) {
		log_debug("%s %s: empty", __func__, name);
		return (0);
	}

	log_debug("%s %s: enter", __func__, name);
	for (;;) {
		item = TAILQ_FIRST(queue);
		if (item == NULL)
			break;
		log_debug("%s %s: %s (%d)", __func__, name,
				item->name, item->type);

		item->time = time(NULL);
		item->number = ++number;

		cmdq_fire_command(item);

		items++;
	}
	cmdq_remove(item);

	log_debug("%s %s: exit (empty)", __func__, name);
	return (items);
}

/* Show error from command. */
void
cmdq_error(unused struct cmdq_item *item, const char *fmt, ...)
{
	va_list		 ap;
	char		*msg;

	va_start(ap, fmt);
	(void)xvasprintf(&msg, fmt, ap);
	va_end(ap);

	log_debug("%s: %s", __func__, msg);
	free(msg);
}
