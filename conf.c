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
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

static const char	*conf_bind_getmask(const char *, unsigned int *);
static void	 	 conf_cmd_remove(const char *);
static void	 	 conf_unbind_kbd(struct binding *);
static void	 	 conf_unbind_mouse(struct binding *);

int
conf_cmd_add(const char *name, const char *path)
{
	struct cmd	*cmd;

	cmd = xcalloc(1, sizeof *cmd);

	cmd->name = xstrdup(name);
	if (strlcpy(cmd->path, path, sizeof(cmd->path)) >= sizeof(cmd->path)) {
		free(cmd->name);
		free(cmd);
		return(0);
	}

	conf_cmd_remove(name);

	TAILQ_INSERT_TAIL(&cmdq, cmd, entry);
	return(1);
}

static void
conf_cmd_remove(const char *name)
{
	struct cmd	*cmd = NULL, *cmdnxt;

	TAILQ_FOREACH_SAFE(cmd, &cmdq, entry, cmdnxt) {
		if (strcmp(cmd->name, name) == 0) {
			TAILQ_REMOVE(&cmdq, cmd, entry);
			free(cmd->name);
			free(cmd);
		}
	}
}

void
conf_autogroup(int num, const char *name, const char *class)
{
	struct autogroupwin	*aw;
	char			*p;

	aw = xmalloc(sizeof(*aw));

	if ((p = strchr(class, ',')) == NULL) {
		if (name == NULL)
			aw->name = NULL;
		else
			aw->name = xstrdup(name);

		aw->class = xstrdup(class);
	} else {
		*(p++) = '\0';

		if (name == NULL)
			aw->name = xstrdup(class);
		else
			aw->name = xstrdup(name);

		aw->class = xstrdup(p);
	}
	aw->num = num;

	TAILQ_INSERT_TAIL(&autogroupq, aw, entry);
}

void
conf_ignore(const char *name)
{
	struct winname	*wn;

	wn = xmalloc(sizeof(*wn));
	wn->name = xstrdup(name);
	TAILQ_INSERT_TAIL(&ignoreq, wn, entry);
}

void
conf_screen(struct screen_ctx *sc, struct group_ctx *gc)
{
	unsigned int		 i;
	XftColor		 xc;
	Colormap		 colormap = DefaultColormap(X_Dpy, sc->which);
	Visual			*visual = DefaultVisual(X_Dpy, sc->which);
	struct config_group	*cgrp = gc->config_group;
	struct config_screen	*cscr = sc->config_screen;

	cgrp->xftfont = XftFontOpenXlfd(X_Dpy, sc->which, cscr->font);
	if (cgrp->xftfont == NULL) {
		cgrp->xftfont = XftFontOpenName(X_Dpy, sc->which, cscr->font);
		if (cgrp->xftfont == NULL)
			log_fatal("XftFontOpenName() failed");
	}

	for (i = 0; i < nitems(cgrp->color); i++) {
		if (i == CWM_COLOR_MENU_FONT_SEL) {
			xu_xorcolor(cgrp->xftcolor[CWM_COLOR_MENU_BG],
			    cgrp->xftcolor[CWM_COLOR_MENU_FG], &xc);
			xu_xorcolor(cgrp->xftcolor[CWM_COLOR_MENU_FONT], xc, &xc);
			if (!XftColorAllocValue(X_Dpy, visual, colormap,
			    &xc.color, &cgrp->xftcolor[CWM_COLOR_MENU_FONT_SEL]))
				log_debug("%s: %s", __func__, cgrp->color[i]);
			break;
		}
		if (XftColorAllocName(X_Dpy, visual, colormap,
		    cgrp->color[i], &xc)) {
			cgrp->xftcolor[i] = xc;
			XftColorFree(X_Dpy, visual, colormap, &xc);
		} else {
			XftColorAllocName(X_Dpy, visual, colormap,
			    cgrp->color[i], &cgrp->xftcolor[i]);
		}
	}

	if (sc->menuwin <= 0) {
		sc->menuwin = XCreateSimpleWindow(X_Dpy, sc->rootwin, 0, 0, 1, 1,
				cgrp->bwidth,
				cgrp->xftcolor[CWM_COLOR_MENU_FG].pixel,
				cgrp->xftcolor[CWM_COLOR_MENU_BG].pixel);
	}

	if (sc->xftdraw == NULL) {
		sc->xftdraw = XftDrawCreate(X_Dpy, sc->menuwin, visual, colormap);
		if (sc->xftdraw == NULL)
			log_fatal("XftDrawCreate() failed");
	}

	conf_cursor(sc);
}

void
conf_init(void)
{
	return;
}

void
conf_clear(void)
{
	struct autogroupwin	*aw, *aw_tmp;
	struct binding		*kb, *mb, *bind_tmp;
	struct winname		*wn, *wn_tmp;
	struct cmd		*cmd, *cmd_tmp;

	TAILQ_FOREACH_SAFE(cmd, &cmdq, entry, cmd_tmp) {
		free(cmd->name);
		TAILQ_REMOVE(&cmdq, cmd, entry);
		free(cmd);
	}

	TAILQ_FOREACH_SAFE(kb, &keybindingq, entry, bind_tmp) {
		TAILQ_REMOVE(&keybindingq, kb, entry);
		free(kb);
	}

	TAILQ_FOREACH_SAFE(aw, &autogroupq, entry, aw_tmp) {
		free(aw->class);
		free(aw->name);
		TAILQ_REMOVE(&autogroupq, aw, entry);
		free(aw);
	}

	TAILQ_FOREACH_SAFE(wn, &ignoreq, entry, wn_tmp) {
		free(wn->name);
		TAILQ_REMOVE(&ignoreq, wn, entry);
		free(wn);
	}

	TAILQ_FOREACH_SAFE(mb, &mousebindingq, entry, bind_tmp) {
		TAILQ_REMOVE(&mousebindingq, mb, entry);
		free(mb);
	}

	/* FIXME: free() colors here. */
}

void
conf_client(struct client_ctx *cc)
{
	struct winname		*wn;
	bool			 ignore = false;

	if (cc->group == NULL) {
		log_debug("Client '%p' is iffy", cc);
		return;
	}

	cc->bwidth = cc->group->config_group->bwidth;

	TAILQ_FOREACH(wn, &ignoreq, entry) {
		if (strcmp(wn->name, cc->name) == 0) {
			ignore = true;
			break;
		}
	}

	if (ignore) {
		cc->bwidth = 0;
		cc->flags |= ignore ? CLIENT_IGNORE : 0;
	}
	client_config(cc);
	client_draw_border(cc);
}

static const struct {
	const char	*tag;
	void		 (*handler)(struct client_ctx *, union arg *);
	int		 flags;
	union arg	 argument;
} name_to_func[] = {
	{ "lower", kbfunc_client_lower, CWM_WIN, {0} },
	{ "raise", kbfunc_client_raise, CWM_WIN, {0} },
	{ "search", kbfunc_client_search, 0, {0} },
	{ "menusearch", kbfunc_menu_cmd, 0, {0} },
	{ "groupsearch", kbfunc_menu_group, 0, {0} },
	{ "hide", kbfunc_client_hide, CWM_WIN, {0} },
	{ "expand", kbfunc_client_expand, CWM_WIN, {0} },
	{ "cycle", kbfunc_client_cycle, CWM_WIN, {.i = CWM_CYCLE} },
	{ "rcycle", kbfunc_client_cycle, CWM_WIN, {.i = CWM_RCYCLE} },
	{ "label", kbfunc_client_label, CWM_WIN, {0} },
	{ "delete", kbfunc_client_delete, CWM_WIN, {0} },
	{ "group0", kbfunc_client_group, CWM_WIN, {.i = 0} },
	{ "group1", kbfunc_client_group, CWM_WIN, {.i = 1} },
	{ "group2", kbfunc_client_group, CWM_WIN, {.i = 2} },
	{ "group3", kbfunc_client_group, CWM_WIN, {.i = 3} },
	{ "group4", kbfunc_client_group, CWM_WIN, {.i = 4} },
	{ "group5", kbfunc_client_group, CWM_WIN, {.i = 5} },
	{ "group6", kbfunc_client_group, CWM_WIN, {.i = 6} },
	{ "group7", kbfunc_client_group, CWM_WIN, {.i = 7} },
	{ "group8", kbfunc_client_group, CWM_WIN, {.i = 8} },
	{ "group9", kbfunc_client_group, CWM_WIN, {.i = 9} },
	{ "grouponly0", kbfunc_client_grouponly, CWM_WIN, {.i = 0} },
	{ "grouponly1", kbfunc_client_grouponly, CWM_WIN, {.i = 1} },
	{ "grouponly2", kbfunc_client_grouponly, CWM_WIN, {.i = 2} },
	{ "grouponly3", kbfunc_client_grouponly, CWM_WIN, {.i = 3} },
	{ "grouponly4", kbfunc_client_grouponly, CWM_WIN, {.i = 4} },
	{ "grouponly5", kbfunc_client_grouponly, CWM_WIN, {.i = 5} },
	{ "grouponly6", kbfunc_client_grouponly, CWM_WIN, {.i = 6} },
	{ "grouponly7", kbfunc_client_grouponly, CWM_WIN, {.i = 7} },
	{ "grouponly8", kbfunc_client_grouponly, CWM_WIN, {.i = 8} },
	{ "grouponly9", kbfunc_client_grouponly, CWM_WIN, {.i = 9} },
	{ "movetogroup0", kbfunc_client_movetogroup, CWM_WIN, {.i = 0} },
	{ "movetogroup1", kbfunc_client_movetogroup, CWM_WIN, {.i = 1} },
	{ "movetogroup2", kbfunc_client_movetogroup, CWM_WIN, {.i = 2} },
	{ "movetogroup3", kbfunc_client_movetogroup, CWM_WIN, {.i = 3} },
	{ "movetogroup4", kbfunc_client_movetogroup, CWM_WIN, {.i = 4} },
	{ "movetogroup5", kbfunc_client_movetogroup, CWM_WIN, {.i = 5} },
	{ "movetogroup6", kbfunc_client_movetogroup, CWM_WIN, {.i = 6} },
	{ "movetogroup7", kbfunc_client_movetogroup, CWM_WIN, {.i = 7} },
	{ "movetogroup8", kbfunc_client_movetogroup, CWM_WIN, {.i = 8} },
	{ "movetogroup9", kbfunc_client_movetogroup, CWM_WIN, {.i = 9} },
	{ "nogroup", kbfunc_client_nogroup, CWM_WIN, {0} },
	{ "cyclegroup", kbfunc_client_cyclegroup, CWM_WIN, {.i = CWM_CYCLE} },
	{ "rcyclegroup", kbfunc_client_cyclegroup, CWM_WIN, {.i = CWM_RCYCLE} },
	{ "cycleingroup", kbfunc_client_cycle, CWM_WIN,
	    {.i = CWM_CYCLE|CWM_INGROUP} },
	{ "rcycleingroup", kbfunc_client_cycle, CWM_WIN,
	    {.i = CWM_RCYCLE|CWM_INGROUP} },
	{ "grouptoggle", kbfunc_client_grouptoggle, CWM_WIN, {.i = 0}},
	{ "sticky", kbfunc_client_toggle_sticky, CWM_WIN, {0} },
	{ "fullscreen", kbfunc_client_toggle_fullscreen, CWM_WIN, {0} },
	{ "maximize", kbfunc_client_toggle_maximize, CWM_WIN, {0} },
	{ "vmaximize", kbfunc_client_toggle_vmaximize, CWM_WIN, {0} },
	{ "hmaximize", kbfunc_client_toggle_hmaximize, CWM_WIN, {0} },
	{ "freeze", kbfunc_client_toggle_freeze, CWM_WIN, {0} },
	{ "restart", kbfunc_cwm_status, 0, {.i = CWM_RESTART} },
	{ "quit", kbfunc_cwm_status, 0, {.i = CWM_QUIT} },
	{ "exec", kbfunc_exec, 0, {.i = CWM_EXEC_PROGRAM} },
	{ "exec_wm", kbfunc_exec, 0, {.i = CWM_EXEC_WM} },
	{ "ssh", kbfunc_ssh, 0, {0} },
	{ "terminal", kbfunc_term, 0, {0} },
	{ "lock", kbfunc_lock, 0, {0} },
	{ "moveup", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_UP|CWM_MOVE)} },
	{ "movedown", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_DOWN|CWM_MOVE)} },
	{ "moveright", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_RIGHT|CWM_MOVE)} },
	{ "moveleft", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_LEFT|CWM_MOVE)} },
	{ "bigmoveup", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_UP|CWM_MOVE|CWM_BIGMOVE)} },
	{ "bigmovedown", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_DOWN|CWM_MOVE|CWM_BIGMOVE)} },
	{ "bigmoveright", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_RIGHT|CWM_MOVE|CWM_BIGMOVE)} },
	{ "bigmoveleft", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_LEFT|CWM_MOVE|CWM_BIGMOVE)} },
	{ "resizeup", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_UP|CWM_RESIZE)} },
	{ "resizedown", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_DOWN|CWM_RESIZE)} },
	{ "resizeright", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_RIGHT|CWM_RESIZE)} },
	{ "resizeleft", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_LEFT|CWM_RESIZE)} },
	{ "bigresizeup", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_UP|CWM_RESIZE|CWM_BIGMOVE)} },
	{ "bigresizedown", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_DOWN|CWM_RESIZE|CWM_BIGMOVE)} },
	{ "bigresizeright", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_RIGHT|CWM_RESIZE|CWM_BIGMOVE)} },
	{ "bigresizeleft", kbfunc_client_moveresize, CWM_WIN,
	    {.i = (CWM_LEFT|CWM_RESIZE|CWM_BIGMOVE)} },
	{ "ptrmoveup", kbfunc_client_moveresize, 0,
	    {.i = (CWM_UP|CWM_PTRMOVE)} },
	{ "ptrmovedown", kbfunc_client_moveresize, 0,
	    {.i = (CWM_DOWN|CWM_PTRMOVE)} },
	{ "ptrmoveleft", kbfunc_client_moveresize, 0,
	    {.i = (CWM_LEFT|CWM_PTRMOVE)} },
	{ "ptrmoveright", kbfunc_client_moveresize, 0,
	    {.i = (CWM_RIGHT|CWM_PTRMOVE)} },
	{ "bigptrmoveup", kbfunc_client_moveresize, 0,
	    {.i = (CWM_UP|CWM_PTRMOVE|CWM_BIGMOVE)} },
	{ "bigptrmovedown", kbfunc_client_moveresize, 0,
	    {.i = (CWM_DOWN|CWM_PTRMOVE|CWM_BIGMOVE)} },
	{ "bigptrmoveleft", kbfunc_client_moveresize, 0,
	    {.i = (CWM_LEFT|CWM_PTRMOVE|CWM_BIGMOVE)} },
	{ "bigptrmoveright", kbfunc_client_moveresize, 0,
	    {.i = (CWM_RIGHT|CWM_PTRMOVE|CWM_BIGMOVE)} },
	{ "snapup", kbfunc_client_snap, CWM_WIN, {.i = (CWM_SNAP_UP) } },
	{ "snapdown", kbfunc_client_snap, CWM_WIN, {.i = (CWM_SNAP_DOWN) } },
	{ "snapleft", kbfunc_client_snap, CWM_WIN, {.i = (CWM_SNAP_LEFT) } },
	{ "snapright", kbfunc_client_snap, CWM_WIN, {.i = (CWM_SNAP_RIGHT) } },
	{ "htile", kbfunc_tile, CWM_WIN, {.i = CWM_TILE_HORIZ} },
	{ "vtile", kbfunc_tile, CWM_WIN, {.i = CWM_TILE_VERT} },
	{ "window_lower", kbfunc_client_lower, CWM_WIN, {0} },
	{ "window_raise", kbfunc_client_raise, CWM_WIN, {0} },
	{ "window_hide", kbfunc_client_hide, CWM_WIN, {0} },
	{ "window_move", mousefunc_client_move, CWM_WIN, {0} },
	{ "window_resize", mousefunc_client_resize, CWM_WIN, {0} },
	{ "window_grouptoggle", kbfunc_client_grouptoggle, CWM_WIN, {.i = 1} },
	{ "menu_group", mousefunc_menu_group, 0, {0} },
	{ "menu_unhide", mousefunc_menu_unhide, 0, {0} },
	{ "menu_cmd", mousefunc_menu_cmd, 0, {0} },
	{ "toggle_border", kbfunc_client_toggle_border, CWM_WIN, {0} }
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

static const char *
conf_bind_getmask(const char *name, unsigned int *mask)
{
	char		*dash;
	const char	*ch;
	unsigned int 	 i;

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

int
conf_bind_kbd(const char *bind, const char *cmd)
{
	struct binding	*kb;
	const char	*key;
	unsigned int	 i;

	kb = xcalloc(1, sizeof(*kb));
	key = conf_bind_getmask(bind, &kb->modmask);

	log_debug("%s: key %s, bind: %s, cmd: %s", __func__, key, bind, cmd);

	kb->press.keysym = XStringToKeysym(key);
	if (kb->press.keysym == NoSymbol) {
		log_debug("unknown symbol: %s", key);
		free(kb);
		return(0);
	}

	/* We now have the correct binding, remove duplicates. */
	conf_unbind_kbd(kb);

	if (strcmp("unmap", cmd) == 0) {
		free(kb);
		return(1);
	}

	for (i = 0; i < nitems(name_to_func); i++) {
		if (strcmp(name_to_func[i].tag, cmd) != 0)
			continue;

		kb->callback = name_to_func[i].handler;
		kb->flags = name_to_func[i].flags;
		kb->argument = name_to_func[i].argument;
		TAILQ_INSERT_TAIL(&keybindingq, kb, entry);
		return(1);
	}

	kb->callback = kbfunc_cmdexec;
	kb->flags = CWM_CMD;
	kb->argument.c = xstrdup(cmd);
	TAILQ_INSERT_TAIL(&keybindingq, kb, entry);
	return(1);
}

static void
conf_unbind_kbd(struct binding *unbind)
{
	struct binding	*key = NULL, *keynxt;

	TAILQ_FOREACH_SAFE(key, &keybindingq, entry, keynxt) {
		if (key->modmask != unbind->modmask)
			continue;

		if (key->press.keysym == unbind->press.keysym) {
			TAILQ_REMOVE(&keybindingq, key, entry);
			log_debug("\t%s: removed key %d", __func__,
			    key->press.keysym);
			if (key->flags & CWM_CMD)
				free(key->argument.c);
			free(key);
		}
	}
}

int
conf_bind_mouse(const char *bind, const char *cmd)
{
	struct binding	*mb;
	const char	*button, *errstr;
	unsigned int	 i;

	mb = xmalloc(sizeof(*mb));
	button = conf_bind_getmask(bind, &mb->modmask);

	mb->press.button = strtonum(button, Button1, Button5, &errstr);
	if (errstr) {
		log_debug("button number is %s: %s", errstr, button);
		free(mb);
		return(0);
	}

	/* We now have the correct binding, remove duplicates. */
	conf_unbind_mouse(mb);

	if (strcmp("unmap", cmd) == 0) {
		free(mb);
		return(1);
	}

	for (i = 0; i < nitems(name_to_func); i++) {
		if (strcmp(name_to_func[i].tag, cmd) != 0)
			continue;

		mb->callback = name_to_func[i].handler;
		mb->flags = name_to_func[i].flags;
		mb->argument = name_to_func[i].argument;
		TAILQ_INSERT_TAIL(&mousebindingq, mb, entry);
		return(1);
	}

	return(0);
}

static void
conf_unbind_mouse(struct binding *unbind)
{
	struct binding		*mb = NULL, *mbnxt;

	TAILQ_FOREACH_SAFE(mb, &mousebindingq, entry, mbnxt) {
		if (mb->modmask != unbind->modmask)
			continue;

		if (mb->press.button == unbind->press.button) {
			TAILQ_REMOVE(&mousebindingq, mb, entry);
			free(mb);
		}
	}
}

static int cursor_binds[] = {
	XC_X_cursor,		/* CF_DEFAULT */
	XC_fleur,		/* CF_MOVE */
	XC_left_ptr,		/* CF_NORMAL */
	XC_question_arrow,	/* CF_QUESTION */
	XC_bottom_right_corner,	/* CF_RESIZE */
};

void
conf_cursor(struct screen_ctx *sc)
{
	struct config_screen	*cscr = sc->config_screen;
	unsigned int		 i;

	for (i = 0; i < nitems(cursor_binds); i++)
		cscr->cursor[i] = XCreateFontCursor(X_Dpy, cursor_binds[i]);
}

void
conf_grab_mouse(Window win)
{
	struct binding	*mb;

	xu_btn_ungrab(win);

	TAILQ_FOREACH(mb, &mousebindingq, entry) {
		if (mb->flags & CWM_WIN)
			xu_btn_grab(win, mb->modmask, mb->press.button);
	}
}

void
conf_grab_kbd(Window win)
{
	struct binding	*kb;

	XGrabServer(X_Dpy);
	xu_key_ungrab(win);

	TAILQ_FOREACH(kb, &keybindingq, entry)
		xu_key_grab(win, kb->modmask, kb->press.keysym);
	XUngrabServer(X_Dpy);
}

static char *cwmhints[] = {
	"WM_STATE",
	"WM_DELETE_WINDOW",
	"WM_TAKE_FOCUS",
	"WM_PROTOCOLS",
	"_MOTIF_WM_HINTS",
	"UTF8_STRING",
	"WM_CHANGE_STATE",
};
static char *ewmhints[] = {
	"_NET_SUPPORTED",
	"_NET_SUPPORTING_WM_CHECK",
	"_NET_ACTIVE_WINDOW",
	"_NET_CLIENT_LIST",
	"_NET_CLIENT_LIST_STACKING",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_CURRENT_DESKTOP",
	"_NET_DESKTOP_VIEWPORT",
	"_NET_DESKTOP_GEOMETRY",
	"_NET_VIRTUAL_ROOTS",
	"_NET_SHOWING_DESKTOP",
	"_NET_DESKTOP_NAMES",
	"_NET_WORKAREA",
	"_NET_WM_NAME",
	"_NET_WM_DESKTOP",
	"_NET_CLOSE_WINDOW",
	"_NET_WM_STATE",
	"_NET_WM_STATE_STICKY",
	"_NET_WM_STATE_MAXIMIZED_VERT",
	"_NET_WM_STATE_MAXIMIZED_HORZ",
	"_NET_WM_STATE_HIDDEN",
	"_NET_WM_STATE_FULLSCREEN",
	"_NET_WM_STATE_DEMANDS_ATTENTION",
};

void
conf_atoms(void)
{
	XInternAtoms(X_Dpy, cwmhints, nitems(cwmhints), False, cwmh);
	XInternAtoms(X_Dpy, ewmhints, nitems(ewmhints), False, ewmh);
}
