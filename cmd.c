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

/* Routines for generic command parsing. */

#include <string.h>
#include <stdbool.h>
#include "calmwm.h"

static bool	 starts_with(const char *, const char *);

extern const struct cmd_entry	cmd_example_entry;

const struct cmd_entry	*cmd_table[] = {
	&cmd_example_entry,
	NULL,
};

const struct cmd_entry *
cmd_find_cmd(const char *cmd_name)
{
	const struct cmd_entry	**cmd_ent = NULL;
	const struct cmd_entry	*cmd_p = NULL;

	if (cmd_name == NULL)
		log_fatal("command name was NULL");

	for (cmd_ent = cmd_table; *cmd_ent != NULL; cmd_ent++) {
		if (strcmp((*cmd_ent)->name, cmd_name) == 0) {
			log_debug("%s: found: %s with %s", __func__,
			    (*cmd_ent)->name, cmd_name);
			cmd_p = *cmd_ent;
			break;
		}
	}

	return (cmd_p);
}

char **
cmd_copy_argv(int argc, char *const *argv)
{
	char	**new_argv;
	int	  i;

	if (argc == 0)
		return (NULL);
	new_argv = xcalloc(argc, sizeof *new_argv);
	for (i = 0; i < argc; i++) {
		if (argv[i] != NULL)
			new_argv[i] = xstrdup(argv[i]);
	}
	return (new_argv);
}

void
cmd_free_argv(int argc, char **argv)
{
	int	i;

	if (argc == 0)
		return;
	for (i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
}

struct cmd *
cmd_parse(int argc, char **argv, const char *file, u_int line, char **cause)
{
	const struct cmd_entry	*entry;
	struct cmd		*cmd;
	struct args		*args;

	*cause = NULL;
	if (argc == 0) {
		xasprintf(cause, "no command");
		return (NULL);
	}

	if ((entry = cmd_find_cmd(argv[0])) == NULL) {
		xasprintf(cause, "unknown command: %s", argv[0]);
		return (NULL);
	}

	args = args_parse(entry->args.template, argc, argv);
	if (args == NULL)
		goto usage;
	if (entry->args.lower != -1 && args->argc < entry->args.lower)
		goto usage;
	if (entry->args.upper != -1 && args->argc > entry->args.upper)
		goto usage;

	cmd = xcalloc(1, sizeof *cmd);
	cmd->entry = entry;
	cmd->args = args;

	if (file != NULL)
		cmd->file = xstrdup(file);
	cmd->line = line;

	return (cmd);

usage:
	if (args != NULL)
		args_free(args);
	xasprintf(cause, "usage: %s %s", entry->name, entry->usage);
	return (NULL);
}

static bool
starts_with(const char *full_str, const char *sub_str)
{
	size_t	 size = strlen(sub_str);

	return (strncmp(full_str, sub_str, size) == 0);
}

struct cmd_find *
cmd_find_target(struct cmd_q *cmdq, const char *target)
{
	/* Some commands are going to need to ensure:
	 *
	 * screen:group:client
	 *
	 * If screen is missing, then the command will either expect a group,
	 * and/or a client.  The current screen is assumed.  This is the one
	 * which has either the client focus, or the mouse focus on the root
	 * window.
	 *
	 * If a group is requested and no screen is present, then the current
	 * screen is assumed (see above).  Otherwise, the group number is
	 * assumed to be the correct argument.  Currently, group names are not
	 * accepted.
	 *
	 * If neither screen or group are present, then the client is assumed.
	 * A client format can be one of:
	 *
	 * Window ID (0x1234567)
	 * Label (as set by the 'label' command)
	 * Class/Resource name.
	 *
	 * If a client match is not amiguous, and there's more than one, then
	 * a menu is shown with which to select the available clients.
	 *
	 * If a client is found to be matching a WindowID and that is not
	 * found, then an error occurs since that was a deliberate and
	 * explicit intent by the user.
	 *
	 * -t 0x12345	-- explicit cilent (global; no need for screen)
	 * -t HDMI1:1	-- group 1 on screen 'HDMI1'
	 * -t :2	-- group 2 on current screen
	 * -t 2		-- group 2 on current screen
	 * -t :1:foo	-- client "foo" in group 1 on current screen.
	 */

	struct cmd_find		*cmdf;
	struct screen_ctx	*sc_cur;
	struct client_ctx	*cc_cur;
	int			 direct_client = 0;

	cmdf = xcalloc(1, sizeof(*cmdf));

	log_debug("%s: target is: %s", __func__, target);

	if (cmdq->cmd->entry->flags == CMD_CLIENT) {
		log_debug("%s: cmd is CMD_CLIENT", __func__);
		if (starts_with(target, "0x")) {
			/* Skip the '0x' */
			target += 2;
			log_debug("%s: 0x -- direct client.  target is: %s",
			    __func__, target);
			direct_client = 1;
		} else if (starts_with(target, ":0x")) {
			/* It might be that we have:
			 *
			 * :0x1234
			 *
			 * Which would mean the current screen has a client
			 * with an ID.
			 */

			/* Skip the ':0x' */
			target += 3;

			log_debug("%s: :0x -- target is: %s", __func__, target);

			sc_cur = screen_current_screen(NULL);
			cc_cur = client_find_win_str(sc_cur, target);

			log_debug("%s: sc_cur: %p (%s), cc_cur: %p", __func__,
			    sc_cur, sc_cur->name, cc_cur);
		}
	}
	return (NULL);
}
