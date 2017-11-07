/*
 * Copyright (c) 2017 Thomas Ada <thomas@xteddy.org>
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

#include <signal.h>
#include <unistd.h>

#include "calmwm.h"

/*
 * Kill the server and do nothing else.
 */

static enum cmd_retval	cmd_example_exec(struct cmd *, struct cmd_q *);

const struct cmd_entry cmd_example_entry = {
	.name = "example",

	.args = { "", 0, 0 },
	.usage = "",
	.flags = CMD_GROUP,
	.exec = cmd_example_exec
};

static enum cmd_retval
cmd_example_exec(struct cmd *self, unused struct cmd_q *cmdq)
{
	//struct cmd_find		*cft;

	(void)cmd_find_target(cmdq, ":2");
	(void)cmd_find_target(cmdq, "jkljljlj");
	
	//(void)cmd_find_target(cmdq, ":.client");
	//(void)cmd_find_target(cmdq, ":6.myclient.foo");
	//(void)cmd_find_target(cmdq, "default:3.myclient");

	return (CMD_RETURN_NORMAL);
}
