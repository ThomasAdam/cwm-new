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

/* Command queue entry. */
struct cmd_q_item {
	struct cmd_list		*cmdlist;
	TAILQ_ENTRY(cmd_q_item)	 qentry;
};
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

	enum cmd_retval  (*exec)(struct cmd *, struct cmd_q *);
};

#endif