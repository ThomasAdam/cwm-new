/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Andy Adamson <dros@monkey.org>
 * Copyright (c) 2004,2005 Marius Aamodt Eriksen <marius@monkey.org>
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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

static void		 group_restack(struct group_ctx *);

const char *num_to_name[] = {
	"zero", "one", "two", "three", "four", "five", "six",
	"seven", "eight", "nine"
};

void
group_assign(struct group_ctx *gc, struct client_ctx *cc)
{

	if (gc == NULL)
		gc = TAILQ_FIRST(&cc->sc->groupq);

	if (cc->group != NULL)
		TAILQ_REMOVE(&cc->group->clientq, cc, group_entry);

	cc->group = gc;

	if (cc->group != NULL)
		TAILQ_INSERT_TAIL(&gc->clientq, cc, group_entry);

	xu_ewmh_net_wm_desktop(cc);

	log_debug("client: (0x%x) assigned to group '%d' screen '%s'",
		(int)cc->win, cc->group->num, cc->sc->name);

	client_draw_border(cc);
}

void
group_hide(struct group_ctx *gc)
{
	struct client_ctx	*cc;

	screen_updatestackingorder(gc->sc);

	TAILQ_FOREACH(cc, &gc->clientq, group_entry)
		client_hide(cc);

	gc->flags &= ~GROUP_ACTIVE;

	u_put_status();
}

void
group_show(struct group_ctx *gc)
{
	struct client_ctx	*cc;

	TAILQ_FOREACH(cc, &gc->clientq, group_entry)
		client_unhide(cc);

	group_restack(gc);
	gc->flags |= GROUP_ACTIVE;

	u_put_status();
}

static void
group_restack(struct group_ctx *gc)
{
	struct client_ctx	*cc;
	Window			*winlist;
	int			 i, lastempty = -1;
	int			 nwins = 0, highstack = 0;

	TAILQ_FOREACH(cc, &gc->clientq, group_entry) {
		if (cc->stackingorder > highstack)
			highstack = cc->stackingorder;
	}
	winlist = xreallocarray(NULL, (highstack + 1), sizeof(*winlist));

	/* Invert the stacking order for XRestackWindows(). */
	TAILQ_FOREACH(cc, &gc->clientq, group_entry) {
		winlist[highstack - cc->stackingorder] = cc->win;
		nwins++;
	}

	/* Un-sparseify */
	for (i = 0; i <= highstack; i++) {
		if (!winlist[i] && lastempty == -1)
			lastempty = i;
		else if (winlist[i] && lastempty != -1) {
			winlist[lastempty] = winlist[i];
			if (++lastempty == i)
				lastempty = -1;
		}
	}

	XRestackWindows(X_Dpy, winlist, nwins);
	free(winlist);
}

void
group_init(struct screen_ctx *sc, int num)
{
	struct group_ctx	*gc;

	gc = xmalloc(sizeof(*gc));
	gc->config_group = xcalloc(1, sizeof(*gc->config_group));
	gc->sc = sc;
	gc->name = xstrdup(num_to_name[num]);
	gc->num = num;
	TAILQ_INIT(&gc->clientq);

	log_debug("%s: added group '%d' (%s) to screen '%s'",
		__func__, gc->num, gc->name, sc->name);

	TAILQ_INSERT_TAIL(&sc->groupq, gc, entry);

	if (num == 0)
		group_setactive(gc);
}

void
group_setactive(struct group_ctx *gc)
{
	struct screen_ctx	*sc = gc->sc;

	sc->group_current = gc;

	xu_ewmh_net_current_desktop(sc);
	u_put_status();

	log_debug("%s: set active group '%d' on screen '%s'",
		__func__, gc->num, sc->name);
}

void
group_movetogroup(struct client_ctx *cc, int idx)
{
	struct screen_ctx	*sc;
	struct group_ctx	*gc;

	if (cc == NULL)
		return;
	sc = cc->sc;

	if (idx < 0 || idx >= CALMWM_NGROUPS)
		log_fatal("%s: index out of range (%d)", __func__, idx);

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (gc->num == idx)
			break;
	}

	if (cc->group == gc)
		return;
	if (group_holds_only_hidden(gc))
		client_hide(cc);
	group_assign(gc, cc);
}

void
group_toggle_membership_enter(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct group_ctx	*gc = sc->group_current;

	if (gc == cc->group) {
		group_assign(NULL, cc);
		cc->flags |= CLIENT_UNGROUP;
	} else {
		group_assign(gc, cc);
		cc->flags |= CLIENT_GROUP;
	}

	client_draw_border(cc);
}

void
group_toggle_membership_leave(struct client_ctx *cc)
{
	cc->flags &= ~CLIENT_HIGHLIGHT;
	client_draw_border(cc);
}

int
group_holds_only_sticky(struct group_ctx *gc)
{
	struct client_ctx	*cc;

	TAILQ_FOREACH(cc, &gc->clientq, group_entry) {
		if (!(cc->flags & CLIENT_STICKY))
			return(0);
	}
	return(1);
}

int
group_holds_only_hidden(struct group_ctx *gc)
{
	struct client_ctx	*cc;
	int			 hidden = 0, same = 0;

	TAILQ_FOREACH(cc, &gc->clientq, group_entry) {
		if (cc->flags & CLIENT_STICKY)
			continue;
		if (hidden == ((cc->flags & CLIENT_HIDDEN) ? 1 : 0))
			same++;
	}

	if (same == 0)
		hidden = !hidden;

	return(hidden);
}

void
group_hidetoggle(struct screen_ctx *sc, int idx)
{
	struct group_ctx	*gc;

	log_debug("%s: screen is '%s'", __func__, sc->name);

	if (idx < 0 || idx >= CALMWM_NGROUPS)
		log_fatal("%s: index out of range (%d)", __func__, idx);

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (gc->num == idx)
			break;
	}

	if (gc->flags & GROUP_HIDDEN) {
		group_show(gc);
		gc->flags &= ~GROUP_HIDDEN;
	} else {
		group_hide(gc);
		gc->flags |= GROUP_HIDDEN;
	}
}

void
group_only(struct screen_ctx *sc, int idx)
{
	struct group_ctx	*gc;

	if (idx < 0 || idx >= CALMWM_NGROUPS)
		log_fatal("%s: index out of range (%d)", __func__, idx);

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (gc->num == idx) {
			group_show(gc);
			group_setactive(gc);
		} else
			group_hide(gc);
	}
}

/*
 * Cycle through all groups on the given screen.
 */
void
group_cycle(struct screen_ctx *sc, int flags)
{
	struct group_ctx	*gc, *gc_loop, *gc_current, *showgroup = NULL;

	assert(sc->group_current != NULL);
	gc_current = sc->group_current;
	TAILQ_FOREACH(gc_loop, &sc->groupq, entry) {
		if (gc_loop == gc_current) {
			gc = gc_loop;
			break;
		}
	}
	gc = (flags & CWM_RCYCLE) ? TAILQ_PREV(gc_loop, group_ctx_q,
		entry) : TAILQ_NEXT(gc_loop, entry);

	if (gc == NULL) {
		gc = (flags & CWM_RCYCLE) ? TAILQ_LAST(&sc->groupq,
		    group_ctx_q) : TAILQ_FIRST(&sc->groupq);
	}

	showgroup = gc;
	log_debug("%s: showing group '%d'", __func__, gc->num);

	group_only(sc, showgroup->num);

	u_put_status();
}

void
group_alltoggle(struct screen_ctx *sc)
{
	struct group_ctx	*gc;

	log_debug("%s: using screen '%s'", __func__, sc->name);

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (TAILQ_EMPTY(&gc->clientq))
			continue;
		if (sc->hideall)
			group_show(gc);
		else
			group_hide(gc);
	}
	sc->hideall = !sc->hideall;
}

void
group_autogroup(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct autogroupwin	*aw;
	struct group_ctx	*gc;
	int			 num = -2, both_match = 0;
	long			*grpnum;

	if (cc->ch.res_class == NULL || cc->ch.res_name == NULL)
		return;

	if (xu_getprop(cc->win, ewmh[_NET_WM_DESKTOP],
	    XA_CARDINAL, 1, (unsigned char **)&grpnum) > 0) {
		num = *grpnum;
		if (num > CALMWM_NGROUPS || num < -1)
			num = CALMWM_NGROUPS - 1;
		XFree(grpnum);
	} else {
		TAILQ_FOREACH(aw, &autogroupq, entry) {
			if (strcmp(aw->class, cc->ch.res_class) == 0) {
				if ((aw->name != NULL) &&
				    (strcmp(aw->name, cc->ch.res_name) == 0)) {
					num = aw->num;
					both_match = 1;
				} else if (aw->name == NULL && !both_match)
					num = aw->num;
			}
		}
	}

	if ((num == -1) || (num == 0)) {
		group_assign(NULL, cc);
		return;
	}

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (gc->num == num) {
			group_assign(gc, cc);
			return;
		}
	}

	group_assign(sc->group_current, cc);

	log_debug("%s: client (0x%x): added to group '%d', screen '%s'",
		__func__, (int)cc->win, sc->group_current->num, cc->sc->name);

	u_put_status();
}

struct group_ctx *
group_find_by_num(struct screen_ctx *sc, int num)
{
	struct group_ctx	*gc = NULL;

	TAILQ_FOREACH(gc, &sc->groupq, entry) {
		if (gc->num == num)
			return (gc);
	}

	log_fatal("%s: group '%d' doesn't exist", __func__, num);
}
