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

static enum cmd_retval	 cmd_bind_exec(struct cmd *, struct cmd_q *);
static struct binding	*bindings_find(const char *);
static bool		 bindings_bind(const char *, const char *,
			    enum binding_type);
static void		 bindings_unbind(struct binding *, enum binding_type);
static const char	*binding_getmask(const char *, unsigned int *);

const struct cmd_entry cmd_bind_entry = {
	.name = "bind",

	.args = { "mu", 2, -1 },
	.usage = "[-m] key command(s)",
	.flags = CMD_NONE,
	.exec = cmd_bind_exec,
};

static const struct {
	const char	ch;
	int		mask;
} bind_mods[] = {
	{ 'C',	ControlMask },
	{ 'M',	Mod1Mask },
	{ '4',	Mod4Mask },
	{ 'S',	ShiftMask },
};

void
bindings_init(void)
{
	unsigned int 	 i;
	struct cmd_list	*cmdlist;
	struct cmd_q	*cmdq = cmdq_new();
	char		*cause;
	char		*defaults[] = {
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

	TAILQ_INIT(&bindingq);

	for (i = 0; i < nitems(defaults); i++) {
		cmdlist = cmd_string_parse(defaults[i], "<default>", i, &cause);
		if (cause != NULL)
			log_fatal("Bad default key: %s", defaults[i]);
		cmdq_append(cmdq, cmdlist);
	}

	cmdq_continue(cmdq);
}

static struct binding *
bindings_find(const char *bind)
{
	struct binding	*key = NULL, *keynxt;
	unsigned int	 modmask;
	const char	*modifier;

	modifier = binding_getmask(bind, &modmask);

	TAILQ_FOREACH_SAFE(key, &bindingq, entry, keynxt) {
		if (key->modmask == modmask)
			return (key);
	}

	return (NULL);
}

static const char *
binding_getmask(const char *name, unsigned int *mask)
{
	char		*dash;
	const char	*ch;
	unsigned int	 i;

	*mask = 0;
	if ((dash = strchr(name, '-')) == NULL)
		return(name);
	for (i = 0; i < nitems(bind_mods); i++) {
		if ((ch = strchr(name, bind_mods[i].ch)) != NULL && ch < dash)
			*mask |= bind_mods[i].mask;
	}

	/* Skip past modifiers. */
	return (dash + 1);
}
static bool
bindings_bind(const char *bind, const char *cmd, enum binding_type type)
{
	struct binding		*b;
	const struct name_func	*nf;
	const char		*key, *errstr;
	bool			 is_mouse = (type == BINDING_MOUSE);

	b = xcalloc(1, sizeof(*b));
	b->type = type;
	key = binding_getmask(bind, &b->modmask);

	log_debug("%s: type: %s, key %s, bind: %s, cmd: %s",
	    __func__, is_mouse ? "mouse" : "key", key, bind, cmd);

	if (!is_mouse) {
		b->press.keysym = XStringToKeysym(key);
		if (b->press.keysym == NoSymbol) {
			log_debug("unknown symbol: %s", key);
			free(b);
			return (false);
		}
	} else {
		/* Mouse binding. */
		b->press.button = strtonum(key, Button1, Button5, &errstr);
		if (errstr != NULL) {
			log_debug("button number is %s: %s", errstr, key);
			free(b);
			return (false);
		}
	}

	/* We now have the correct binding, remove duplicates. */
	bindings_unbind(b, type);

	if (strcmp("unmap", cmd) == 0) {
		free(b);
		return (true);
	}

	for (nf = name_to_func; nf->tag != NULL; nf++) {
		if (strcmp(nf->tag, cmd) != 0)
			continue;

		b->callback = nf->handler;
		b->flags = nf->flags;
		b->argument = nf->argument;
		TAILQ_INSERT_TAIL(&bindingq, b, entry);
		return (true);
	}

	if (is_mouse)
		return (true);

	b->callback = kbfunc_cmdexec;
	b->flags = CWM_CMD;
	b->argument.c = xstrdup(cmd);
	TAILQ_INSERT_TAIL(&bindingq, b, entry);
	return (true);
}

static void
bindings_unbind(struct binding *b, enum binding_type type)
{
	struct binding	*ub, *ubnxt;
	bool		 is_mouse = (type == BINDING_MOUSE);

	TAILQ_FOREACH_SAFE(ub, &bindingq, entry, ubnxt) {
		if (b->modmask != ub->modmask)
			continue;

		if ((!is_mouse && (b->press.keysym == ub->press.keysym))
		    ||
		   (is_mouse && (b->press.button == ub->press.button))) {
			TAILQ_REMOVE(&bindingq, b, entry);
			log_debug("\t%s: removed key %d", __func__,
			    is_mouse ? b->press.button : b->press.keysym);
			if (!is_mouse && b->flags & CWM_CMD)
				free(b->argument.c);
			free(b);
		}
	}
}
static enum cmd_retval
cmd_bind_exec(struct cmd *self, struct cmd_q *cmdq)
{
	struct args		*args = self->args;
	struct binding		*b;
	enum binding_type	 type;

	type = args_has(args, 'm') ? BINDING_MOUSE : BINDING_KEY;

	if (args_has(args, 'u')) {
		if ((b = bindings_find(args->argv[0])) == NULL)
			return (CMD_RETURN_ERROR);
		bindings_unbind(b, type);

		return (CMD_RETURN_NORMAL);
	}

	bindings_bind(args->argv[0], args->argv[1], type);

	return (CMD_RETURN_NORMAL);
}
