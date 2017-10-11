/*
 * calmwm - the calm window manager
 *
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
 */

#ifndef _CMD_CALMWM_H_
#define _CMD_CALMWM_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "array.h"
#include "calmwm.h"

/* Parsed arguments structures. */
struct args_entry {
	u_char			 flag;
	char			*value;
	RB_ENTRY(args_entry)	 entry;
};
RB_HEAD(args_tree, args_entry);

struct args {
	struct args_tree	  tree;
	int		 	  argc;
	char	       		**argv;
};

/* Information used to register commands lswm understands. */
struct cmd {
	const struct cmd_entry	*entry;
	struct args		*args;

	char			*file;
	u_int			 line;

	TAILQ_ENTRY(cmd)	 qentry;
};

struct cmd_list {
	int		 	 references;
	TAILQ_HEAD(, cmd) 	 list;
};

/* Command return values. */
enum cmd_retval {
	CMD_RETURN_ERROR = -1,
	CMD_RETURN_NORMAL = 0,
};

/* Command queue item type. */
enum cmdq_type {
	CMDQ_COMMAND,
	CMDQ_CALLBACK,
};

/* Command queue item. */
struct cmdq_item;

typedef enum cmd_retval (*cmdq_cb) (struct cmdq_item *, void *);
struct cmdq_item {
	const char		*name;
	struct cmdq_list	*queue;
	struct cmdq_item	*next;

	enum cmdq_type		 type;
	u_int			 group;

	u_int			 number;
	time_t			 time;

	struct cmd_list		*cmdlist;
	struct cmd		*cmd;

	cmdq_cb			 cb;
	void			*data;

	TAILQ_ENTRY(cmdq_item)	 entry;
};
TAILQ_HEAD(cmdq_list, cmdq_item);
TAILQ_HEAD(cmd_q_items, cmd_q_item);

/* Command queue. */
struct cmd_q {
	int			 references;

	struct cmd_q_items	 queue;
	struct cmd_q_item	*item;
	struct cmd		*cmd;
};

struct cmd_entry {
	const char		*name;

	struct {
		const char	*template;
		int		 lower;
		int		 upper;
	} args;
	const char		*usage;

	enum cmd_retval  (*exec)(struct cmd *, struct cmdq_item *);
};

#endif
