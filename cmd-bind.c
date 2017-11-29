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
static void		bindings_init(void);

const struct cmd_entry cmd_bind_entry = {
	.name = "bind",

	.args = { "m", 2, -1 },
	.usage = "[-m] key command(s)",
	.flags = CMD_NONE,
	.exec = cmd_bind_exec,
};

static void
bindings_init(void)
{
	char	*defaults[] = {
		"bind 4S-Down	 snapdown",
		"bind 4S-Left	 snapleft",
		"bind 4S-Right	 snapright",
		"bind 4S-Up	 snapup",
		"bind C-Return   expand",
		"bind C-Down	 ptrmovedown",
		"bind C-Left	 ptrmoveleft",
		"bind C-Right	 ptrmoveright",
		"bind C-Up	 ptrmoveup",
		"bind C-slash	 menusearch",
		"bind CM-0	 group0",
		"bind CM-1	 group1",
		"bind CM-2	 group2",
		"bind CM-3	 group3",
		"bind CM-4	 group4",
		"bind CM-5	 group5",
		"bind CM-6	 group6",
		"bind CM-7	 group7",
		"bind CM-8	 group8",
		"bind CM-9	 group9",
		"bind CM-B	 toggle_border",
		"bind CM-Delete	 lock",
		"bind CM-H	 bigresizeleft",
		"bind CM-J	 bigresizedown",
		"bind CM-K	 bigresizeup",
		"bind CM-L	 bigresizeright",
		"bind CM-Return	 terminal",
		"bind CM-a	 nogroup",
		"bind CM-equal	 vmaximize",
		"bind CM-f	 fullscreen",
		"bind CM-g	 grouptoggle",
		"bind CM-h	 resizeleft",
		"bind CM-j	 resizedown",
		"bind CM-k	 resizeup",
		"bind CM-l	 resizeright",
		"bind CM-m	 maximize",
		"bind CM-n	 label",
		"bind CM-s	 sticky",
		"bind CM-x	 delete",
		"bind CMS-equal	 hmaximize",
		"bind CMS-f	 freeze",
		"bind CMS-q	 quit",
		"bind CMS-r	 restart",
		"bind CS-Down	 bigptrmovedown",
		"bind CS-Left	 bigptrmoveleft",
		"bind CS-Right	 bigptrmoveright",
		"bind CS-Up	 bigptrmoveup",
		"bind M-Down	 lower",
		"bind M-H	 bigmoveleft",
		"bind M-J	 bigmovedown",
		"bind M-K	 bigmoveup",
		"bind M-L	 bigmoveright",
		"bind M-Left	 rcyclegroup",
		"bind M-Right	 cyclegroup",
		"bind M-Tab	 cycle",
		"bind M-Up	 raise",
		"bind M-h	 moveleft",
		"bind M-j	 movedown",
		"bind M-k	 moveup",
		"bind M-l	 moveright",
		"bind M-period	 ssh",
		"bind M-question exec",
		"bind M-slash	 search",
		"bind MS-Tab	 rcycle",
		"bind -m 1	 menu_unhide",
		"bind -m 2	 menu_group",
		"bind -m 3	 menu_cmd",
		"bind -m M-1	 window_move",
		"bind -m CM-1	 window_grouptoggle",
		"bind -m M-2	 window_resize",
		"bind -m M-3	 window_lower",
		"bind -m CMS-3	 window_hide",
	};
}

static enum cmd_retval
cmd_bind_exec(struct cmd *self, struct cmd_q *cmdq)
{
	return (CMD_RETURN_NORMAL);
}
