/*
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

#include <signal.h>
#include <unistd.h>

#include "calmwm.h"

/*
 * Handle key-bindings.
 */

static enum cmd_retval	cmd_bind_exec(struct cmd *, struct cmd_q *);

const struct cmd_entry cmd_bind_entry = {
	.name = "bind",

	.args = { "m", 2, -1 },
	.usage = "[-m] key command(s)",
	.flags = CMD_NONE,
	.exec = cmd_bind_exec,
};

static enum cmd_retval
cmd_bind_exec(struct cmd *self, struct cmd_q *cmdq)
{
	return (CMD_RETURN_NORMAL);
}
