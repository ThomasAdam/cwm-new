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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

#define OVERLAP(a,b,c,d) (((a)==(c) && (b)==(d)) || \
		MIN((a)+(b), (c)+(d)) - MAX((a), (c)) > 0)

static struct client_ctx	*client_next(struct client_ctx *);
static struct client_ctx	*client_prev(struct client_ctx *);
static void			 client_mtf(struct client_ctx *);
static void			 client_none(struct screen_ctx *);
static void			 client_placecalc(struct client_ctx *);
static void			 client_wm_protocols(struct client_ctx *);
static void			 client_mwm_hints(struct client_ctx *);
static int			 client_inbound(struct client_ctx *, int, int);
static void			 client_expand_horiz(struct client_ctx *,
					struct geom *);
static void			 client_expand_vert(struct client_ctx *,
					struct geom *);

struct client_ctx	*curcc = NULL;

void
client_data_extend(struct client_ctx *cc)
{
	struct screen_ctx	*sc;
	XWindowAttributes	 wattr;
	int			 x, y, w, h;

	if (cc == NULL || cc->extended_data)
		return;

	sc = cc->sc;

	XGetWindowAttributes(X_Dpy, cc->win, &wattr);

	x = wattr.x - sc->work.x - cc->bwidth;
	y = wattr.y - sc->work.y - cc->bwidth;
	w = wattr.width + cc->bwidth * 2;
	h = wattr.height + cc->bwidth * 2;

	cc->geom.x = sc->work.x + x;
	cc->geom.y = sc->work.y + y;
	cc->geom.w = w;
	cc->geom.h = h;
	cc->extended_data = 1;
}

void
client_scan_for_windows(void)
{
	struct screen_ctx	*sc;
	struct client_ctx	*cc;
	Window			*wins, w0, w1, root;
	unsigned int		 i, nwins;
	int			 ptr_x, ptr_y;

	/* Assume the entire root window to scan for clients.  client_init()
	 * can sort out those clients it wants for its screen.
	 */
	root = RootWindow(X_Dpy, DefaultScreen(X_Dpy));

	/* Deal with existing clients. */
	if (XQueryTree(X_Dpy, root, &w0, &w1, &wins, &nwins)) {
		for (i = 0; i < nwins; i++) {
			/* If the pointer is over a window, focus it. */
			if ((cc = client_init(wins[i], 0)) != NULL) {
				xu_ptr_getpos(root, &ptr_x, &ptr_y);

				if (client_inbound(cc, ptr_x, ptr_y))
					client_setactive(cc);
			}
		}

		XFree(wins);
	}

	/* Having got a list of managed clients, update their stacking order.
	 * We have to do this here, rather than client_init() as it's the only
	 * way to know when all clients have been managed.
	 */
	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (screen_should_ignore_global(sc))
			continue;
		screen_updatestackingorder(sc);
	}
}

struct client_ctx *
client_find(Window win)
{
	struct screen_ctx	*sc;
	struct client_ctx	*cc;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (screen_should_ignore_global(sc))
			continue;
		TAILQ_FOREACH(cc, &sc->clientq, entry) {
			if (cc->win == win)
				return(cc);
		}
	}
	return(NULL);
}

struct client_ctx *
client_init(Window win, int skip_map_check)
{
	struct screen_ctx	*sc;
	struct client_ctx	*cc;
	XWindowAttributes	 wattr;
	int			 mapped;

	if (win == None) {
		log_debug("%s: win is NULL", __func__);
		return(NULL);
	}

	if (!XGetWindowAttributes(X_Dpy, win, &wattr)) {
		log_debug("%s: XGetWindowAttributes() failed", __func__);
		return(NULL);
	}

	if (skip_map_check)
		mapped = 1;
	else {
		if (wattr.override_redirect || wattr.map_state != IsViewable) {
			return(NULL);
		}
	}
	mapped = wattr.map_state != IsUnmapped;

	cc = xcalloc(1, sizeof(*cc));

	XGrabServer(X_Dpy);

	cc->win = win;
	cc->extended_data = 0;

	TAILQ_INIT(&cc->nameq);
	client_setname(cc);

	XGetClassHint(X_Dpy, cc->win, &cc->ch);
	client_wm_hints(cc);
	client_wm_protocols(cc);
	client_getsizehints(cc);
	client_mwm_hints(cc);

	cc->flags |= CLIENT_BORDER;

	/* Saved pointer position */
	cc->ptr.x = -1;
	cc->ptr.y = -1;

	cc->geom.x = wattr.x;
	cc->geom.y = wattr.y;
	cc->geom.w = wattr.width;
	cc->geom.h = wattr.height;
	cc->colormap = wattr.colormap;
	cc->bwidth = wattr.border_width;

	if (wattr.map_state != IsViewable) {
		client_placecalc(cc);
		if ((cc->wmh) && (cc->wmh->flags & StateHint))
			client_set_wm_state(cc, cc->wmh->initial_state);
	}

	{
		/*
		 * The client is mapped; in applying borders to the window,
		 * the window would be n pixels out.  Reposition the window
		 * accordingly.
		 */
		XMoveWindow(X_Dpy, cc->win, cc->geom.x + cc->bwidth,
				cc->geom.y + cc->bwidth);
	}
	conf_client(cc);

	sc = screen_find_screen(cc->geom.x, cc->geom.y);
	log_debug("client: (0x%x) to screen '%s'", (int)cc->win, sc->name);
	cc->sc = sc;

	XSelectInput(X_Dpy, cc->win, ColormapChangeMask | EnterWindowMask |
	    PropertyChangeMask | KeyReleaseMask);

	XAddToSaveSet(X_Dpy, cc->win);

	client_transient(cc);

	TAILQ_INSERT_TAIL(&sc->clientq, cc, entry);

	xu_ewmh_net_client_list(sc);
	xu_ewmh_net_client_list_stacking(sc);
	xu_ewmh_restore_net_wm_state(cc);

	if (client_get_wm_state(cc) == IconicState)
		client_hide(cc);
	else
		client_unhide(cc);

	if (!mapped)
		log_debug("client not mapped!");

	group_autogroup(cc);

	XSync(X_Dpy, False);
	XUngrabServer(X_Dpy);

	u_put_status();

	return(cc);
}

void
client_delete(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct winname		*wn;

	TAILQ_REMOVE(&sc->clientq, cc, entry);

	xu_ewmh_net_client_list(sc);
	xu_ewmh_net_client_list_stacking(sc);

	if (cc->group != NULL)
		TAILQ_REMOVE(&cc->group->clientq, cc, group_entry);

	if (cc == client_current())
		client_none(sc);

	while ((wn = TAILQ_FIRST(&cc->nameq)) != NULL) {
		TAILQ_REMOVE(&cc->nameq, wn, entry);
		free(wn->name);
		free(wn);
	}

	if (cc->ch.res_class)
		XFree(cc->ch.res_class);
	if (cc->ch.res_name)
		XFree(cc->ch.res_name);
	if (cc->wmh)
		XFree(cc->wmh);

	free(cc);
}

void
client_setactive(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct client_ctx	*oldcc;

	if (cc->flags & CLIENT_HIDDEN)
		return;

	XInstallColormap(X_Dpy, cc->colormap);

	if ((cc->flags & CLIENT_INPUT) ||
	    (!(cc->flags & CLIENT_WM_TAKE_FOCUS))) {
		XSetInputFocus(X_Dpy, cc->win,
		    RevertToPointerRoot, CurrentTime);
	}
	if (cc->flags & CLIENT_WM_TAKE_FOCUS)
		client_msg(cc, cwmh[WM_TAKE_FOCUS], Last_Event_Time);

	if ((oldcc = client_current())) {
		oldcc->flags &= ~CLIENT_ACTIVE;
		client_draw_border(oldcc);
	}

	/* If we're in the middle of cycing, don't change the order. */
	if (!sc->cycling)
		client_mtf(cc);

	curcc = cc;
	cc->flags |= CLIENT_ACTIVE;
	cc->flags &= ~CLIENT_URGENCY;
	client_draw_border(cc);
	conf_grab_mouse(cc->win);
	xu_ewmh_net_active_window(sc, cc->win);

	u_put_status();
}

/*
 * set when there is no active client
 */
static void
client_none(struct screen_ctx *sc)
{
	Window none = None;

	xu_ewmh_net_active_window(sc, none);

	curcc = NULL;
}

void
client_snap(struct client_ctx *cc, int dir)
{
	struct client_ctx	*ci;
	struct screen_ctx	*sc = cc->sc;
	int			 n, x, y, w, h;

	client_data_extend(cc);

	n = 0;
	x = cc->geom.x, y = cc->geom.y, w = cc->geom.w, h = cc->geom.h;

	if (dir == CWM_SNAP_UP) {
		y--;
		n = sc->work.y;
		TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
			if (cc == ci)
				continue;

			if (!OVERLAP(cc->geom.x - 1, cc->geom.w + 2, ci->geom.x,
				     ci->geom.w)) {
				continue;
			}
			if (ci->geom.y + ci->geom.h <= y)
				n = MAX(n, ci->geom.y + ci->geom.h);
			if (ci->geom.y <= y)
				n = MAX(n, ci->geom.y);
			if (ci->geom.y + ci->geom.h <= y + h)
				n = MAX(n, ci->geom.y + ci->geom.h - h);
			if (ci->geom.y <= y + h)
				n = MAX(n, ci->geom.y - h);
		}
		y = n;
	}

	if (dir == CWM_SNAP_DOWN) {
		y++;
		n = sc->work.y + sc->work.h;
		TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
			if (cc == ci)
				continue;

			client_data_extend(ci);
			if (!OVERLAP(cc->geom.x - 1, cc->geom.w + 2, ci->geom.x,
			    ci->geom.w)) {
				continue;
			}

			if (ci->geom.y + ci->geom.h >= y + h)
				n = MIN(n, ci->geom.y + ci->geom.h);
			if (ci->geom.y >= y + h)
				n = MIN(n, ci->geom.y);
			if (ci->geom.y + ci->geom.h >= y)
				n = MIN(n, ci->geom.y + ci->geom.h + h);
			if (ci->geom.y >= y)
				n = MIN(n, ci->geom.y + h);
		}
		y = n - h;
	}

	if (dir == CWM_SNAP_LEFT) {
		x--;
		n = sc->work.x;
		TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
			if (cc == ci)
				continue;

			client_data_extend(ci);
			if (!OVERLAP(cc->geom.y - 1, cc->geom.h + 2, ci->geom.y,
			    ci->geom.h)) {
				continue;
			}
			if (ci->geom.x + ci->geom.w <= x)
				n = MAX(n, ci->geom.x + ci->geom.w);
			if (ci->geom.x <= x)
				n = MAX(n, ci->geom.x);
			if (ci->geom.x + ci->geom.w <= x + w)
				n = MAX(n, ci->geom.x + ci->geom.w - w);
			if (ci->geom.x <= x + w)
				n = MAX(n, ci->geom.x - w);
		}
		x = n;
	}

	if (dir == CWM_SNAP_RIGHT) {
		x++;
		n = sc->work.x + sc->work.w;
		TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
			if (cc == ci)
				continue;

			client_data_extend(ci);
			if (!OVERLAP(cc->geom.y - 1, cc->geom.h + 2, ci->geom.y,
			    ci->geom.h)) {
				continue;
			}
			if (ci->geom.x + ci->geom.w >= x + w)
				n = MIN(n, ci->geom.x + ci->geom.w);
			if (ci->geom.x >= x + w)
				n = MIN(n, ci->geom.x);
			if (ci->geom.x + ci->geom.w >= x)
				n = MIN(n, ci->geom.x + ci->geom.w + w);
			if (ci->geom.x >= x) n = MIN(n, ci->geom.x + w);
		}
		x = n - w;
	}

	cc->geom.x = x;
	cc->geom.y = y;
	client_move(cc);
	client_ptrwarp(cc);
}

struct client_ctx *
client_current(void)
{
	return(curcc);
}

void
client_toggle_freeze(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_FREEZE)
		cc->flags &= ~CLIENT_FREEZE;
	else
		cc->flags |= CLIENT_FREEZE;
}

void
client_toggle_hidden(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_HIDDEN)
		cc->flags &= ~CLIENT_HIDDEN;
	else
		cc->flags |= CLIENT_HIDDEN;

	xu_ewmh_set_net_wm_state(cc);
}

void
client_toggle_sticky(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_STICKY) {
		cc->flags &= ~CLIENT_STICKY;
		/*
		 * When taking a window out of being sticky, reassign the
		 * client back to the current group.  If the group has changed
		 * from the client's original group at the time the client
		 * became sticky, it should stick to the currently active
		 * group.  Some people may even use this to implement a crude
		 * form of moving a window from one group to the next.
		 */
		group_assign(cc->sc->group_current, cc);
	} else
		cc->flags |= CLIENT_STICKY;

	xu_ewmh_set_net_wm_state(cc);
}

void
client_toggle_border(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_BORDER) {
		cc->flags &= ~CLIENT_BORDER;
		cc->bwidth = 0;
	} else {
		cc->flags |= CLIENT_BORDER;
		cc->bwidth = Conf.bwidth;
	}

	client_config(cc);
	client_draw_border(cc);
}

void
client_toggle_fullscreen(struct client_ctx *cc)
{
	struct geom		 xine;

	if ((cc->flags & CLIENT_FREEZE) &&
	    !(cc->flags & CLIENT_FULLSCREEN))
		return;

	if (cc->flags & CLIENT_FULLSCREEN) {
		if (cc->flags & CLIENT_BORDER)
			cc->bwidth = Conf.bwidth;
		cc->geom = cc->fullgeom;
		cc->flags &= ~(CLIENT_FULLSCREEN | CLIENT_FREEZE);
		goto resize;
	}

	cc->fullgeom = cc->geom;

	xine = screen_find_xinerama(
	    cc->geom.x + cc->geom.w / 2,
	    cc->geom.y + cc->geom.h / 2, CWM_NOGAP);

	cc->bwidth = 0;
	cc->geom = xine;
	cc->flags |= (CLIENT_FULLSCREEN | CLIENT_FREEZE);

resize:
	client_resize(cc, 0);
	xu_ewmh_set_net_wm_state(cc);
}

static void
client_expand_horiz(struct client_ctx *cc, struct geom *new_geom)
{
	struct geom		 win_geom;
	struct screen_ctx	*sc;
	struct client_ctx	*ci;
	int			 cc_x, cc_y, cc_end_x, cc_end_y;
	int			 ci_x, ci_y, ci_end_x, ci_end_y;
	int			 new_x1, new_x2;

	sc = cc->sc;

	new_x1 = sc->work.x;
	new_x2 = new_x1 + sc->work.w;

	cc_x = new_geom->x;
	cc_y = new_geom->y;
	cc_end_x = cc_x + new_geom->w;
	cc_end_y = cc_y + new_geom->h;

	TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
		if (ci == cc)
			continue;

		win_geom = ci->geom;

		ci_x = win_geom.x;
		ci_y = win_geom.y;
		ci_end_x = (ci_x + win_geom.w) + cc->bwidth * 2;
		ci_end_y = ci_y + win_geom.h;

		/* Check to see if the client is the same Y axis. */
		if (!((ci_end_y <= cc_y) || (ci_y >= cc_end_y)))
		{
			if ((ci_end_x <= cc_x) && (ci_end_x >= new_x1))
			{
				/* Expand up to the window, including the
				 * border's edge.
				 */
				new_x1 = ci_end_x;
			}
			else if ((cc_end_x <= ci_x) && (new_x2 >= ci_x))
			{
				/* Shrink back to the top of the border,
				 * outside of its width so we don't overlap.
				 */
				new_x2 = ci_x;
			}
		}
	}
	new_geom->w = (new_x2 - new_x1) - cc->bwidth * 2;
	new_geom->x = new_x1;
}

static void
client_expand_vert(struct client_ctx *cc, struct geom *new_geom)
{
	struct geom		 win_geom;
	struct screen_ctx	*sc;
	struct client_ctx	*ci;
	int			 cc_x, cc_y, cc_end_x, cc_end_y;
	int			 ci_x, ci_y, ci_end_x, ci_end_y;
	int			 new_y1, new_y2;

	sc = cc->sc;

	cc_x = new_geom->x;
	cc_y = new_geom->y;
	cc_end_x = cc_x + new_geom->w + cc->bwidth * 2;
	cc_end_y = cc_y + new_geom->h;

	new_y1 = sc->work.y;
	new_y2 = new_y1 + sc->work.h;

	/* Go through all clients and move up and down. */
	TAILQ_FOREACH(ci, &cc->group->clientq, group_entry) {
		if (ci == cc)
			continue;

		win_geom = ci->geom;

		ci_x = win_geom.x;
		ci_y = win_geom.y;
		ci_end_x = ci_x + win_geom.w;
		ci_end_y = ci_y + win_geom.h;

		if (!((ci_end_x <= cc_x) || (ci_x >= cc_end_x)))
		{
			if ((ci_end_y <= cc_y) && (ci_end_y >= new_y1))
			{
				new_y1 = ci_end_y;
			}
			else if ((cc_end_y <= ci_y) && (new_y2 >= ci_y))
			{
				new_y2 = ci_y;
			}
		}
	}
	new_geom->h = (new_y2 - new_y1) - cc->bwidth * 2;
	new_geom->y = new_y1;
}

void
client_expand(struct client_ctx *cc)
{
	struct geom	 new_geom;

	log_debug("%s: Expanding client on screen '%s'", __func__, cc->sc->name);

	if (cc->flags & CLIENT_FREEZE)
		return;

	if (cc->flags & CLIENT_EXPANDED) {
		cc->flags &= ~CLIENT_EXPANDED;
		cc->geom = cc->savegeom;
		cc->bwidth = Conf.bwidth;

		/* Don't allow to overrun boundaries. */
		{
			struct geom	 scedge = cc->sc->work;
			scedge.w += scedge.x - Conf.bwidth * 2;
			scedge.h += scedge.y - Conf.bwidth * 2;

			if (cc->geom.x + cc->geom.y >= scedge.w)
				cc->geom.x = scedge.w - cc->geom.w;
			if (cc->geom.x < scedge.x) {
				cc->geom.x = scedge.x;
				cc->geom.w = MIN(cc->geom.w,
						(scedge.w - scedge.x));
			}
			if (cc->geom.y + cc->geom.h >= scedge.h)
				cc->geom.y = scedge.h - cc->geom.h;
			if (cc->geom.y < scedge.y) {
				cc->geom.y = scedge.y;
				cc->geom.h = MIN(cc->geom.h,
						(scedge.h - scedge.y));
			}
		}
		goto resize;
	} else {
		memcpy(&cc->savegeom, &cc->geom, sizeof(cc->geom));
		memcpy(&new_geom, &cc->geom, sizeof(cc->geom));
	}

	client_expand_horiz(cc, &new_geom);
	client_expand_vert(cc, &new_geom);

	memcpy(&cc->geom, &new_geom, sizeof(new_geom));
	cc->extended_data = 0;
        cc->flags |= CLIENT_EXPANDED;

resize:
        client_resize(cc, 0);
}

void
client_toggle_maximize(struct client_ctx *cc)
{
        struct geom              xine;

        if (cc->flags & CLIENT_FREEZE)
                return;

        if ((cc->flags & CLIENT_MAXFLAGS) == CLIENT_MAXIMIZED) {
                cc->geom = cc->savegeom;
                cc->flags &= ~CLIENT_MAXIMIZED;
                goto resize;
        }

        if (!(cc->flags & CLIENT_VMAXIMIZED)) {
                cc->savegeom.h = cc->geom.h;
                cc->savegeom.y = cc->geom.y;
        }

        if (!(cc->flags & CLIENT_HMAXIMIZED)) {
                cc->savegeom.w = cc->geom.w;
                cc->savegeom.x = cc->geom.x;
        }

        /*
         * pick screen that the middle of the window is on.
         * that's probably more fair than if just the origin of
         * a window is poking over a boundary
         */
        xine = screen_find_xinerama(
            cc->geom.x + cc->geom.w / 2,
            cc->geom.y + cc->geom.h / 2, CWM_GAP);

        cc->geom.x = xine.x;
        cc->geom.y = xine.y;
        cc->geom.w = xine.w - (cc->bwidth * 2);
        cc->geom.h = xine.h - (cc->bwidth * 2);
        cc->flags |= CLIENT_MAXIMIZED;

resize:
        client_resize(cc, 0);
        xu_ewmh_set_net_wm_state(cc);
}

void
client_toggle_vmaximize(struct client_ctx *cc)
{
	struct geom		 xine;

	if (cc->flags & CLIENT_FREEZE)
		return;

	if (cc->flags & CLIENT_VMAXIMIZED) {
		cc->geom.y = cc->savegeom.y;
		cc->geom.h = cc->savegeom.h;
		cc->flags &= ~CLIENT_VMAXIMIZED;
		goto resize;
	}

	cc->savegeom.y = cc->geom.y;
	cc->savegeom.h = cc->geom.h;

	xine = screen_find_xinerama(
	    cc->geom.x + cc->geom.w / 2,
	    cc->geom.y + cc->geom.h / 2, CWM_GAP);

	cc->geom.y = xine.y;
	cc->geom.h = xine.h - (cc->bwidth * 2);
	cc->flags |= CLIENT_VMAXIMIZED;

resize:
	client_resize(cc, 0);
	xu_ewmh_set_net_wm_state(cc);
}

void
client_toggle_hmaximize(struct client_ctx *cc)
{
	struct geom		 xine;

	if (cc->flags & CLIENT_FREEZE)
		return;

	if (cc->flags & CLIENT_HMAXIMIZED) {
		cc->geom.x = cc->savegeom.x;
		cc->geom.w = cc->savegeom.w;
		cc->flags &= ~CLIENT_HMAXIMIZED;
		goto resize;
	}

	cc->savegeom.x = cc->geom.x;
	cc->savegeom.w = cc->geom.w;

	xine = screen_find_xinerama(
	    cc->geom.x + cc->geom.w / 2,
	    cc->geom.y + cc->geom.h / 2, CWM_GAP);

	cc->geom.x = xine.x;
	cc->geom.w = xine.w - (cc->bwidth * 2);
	cc->flags |= CLIENT_HMAXIMIZED;

resize:
	client_resize(cc, 0);
	xu_ewmh_set_net_wm_state(cc);
}

void
client_resize(struct client_ctx *cc, int reset)
{
	if (reset) {
		cc->flags &= ~CLIENT_MAXIMIZED;
		xu_ewmh_set_net_wm_state(cc);
	}

	XMoveResizeWindow(X_Dpy, cc->win, cc->geom.x,
	    cc->geom.y, cc->geom.w, cc->geom.h);

	client_config(cc);
	client_draw_border(cc);
}

void
client_move(struct client_ctx *cc)
{
	struct screen_ctx	*sc_new;

	/*
	 * Update which monitor the client is on, and therefore which group
	 * the client is in.
	 *
	 * This assigns the client to the active group the output is on, if
	 * the client being moved acrosses the boundaries between outputs.
	 */
	sc_new = screen_find_screen(cc->geom.x, cc->geom.y);
	if (cc->sc != NULL && cc->sc != sc_new)
		group_assign(sc_new->group_current, cc);

	cc->sc = sc_new;
	client_data_extend(cc);

	XMoveWindow(X_Dpy, cc->win, cc->geom.x, cc->geom.y);
	client_config(cc);

}

void
client_lower(struct client_ctx *cc)
{
	XLowerWindow(X_Dpy, cc->win);
}

void
client_raise(struct client_ctx *cc)
{
	XRaiseWindow(X_Dpy, cc->win);
}

void
client_config(struct client_ctx *cc)
{
	XConfigureEvent	 cn;

	(void)memset(&cn, 0, sizeof(cn));

	cn.type = ConfigureNotify;
	cn.event = cc->win;
	cn.window = cc->win;
	cn.x = cc->geom.x;
	cn.y = cc->geom.y;
	cn.width = cc->geom.w;
	cn.height = cc->geom.h;
	cn.border_width = cc->bwidth;
	cn.above = None;
	cn.override_redirect = 0;

	XSendEvent(X_Dpy, cc->win, False, StructureNotifyMask, (XEvent *)&cn);
}

void
client_ptrwarp(struct client_ctx *cc)
{
	int	 x = cc->ptr.x, y = cc->ptr.y;

	if (x == -1 || y == -1) {
		x = cc->geom.w / 2;
		y = cc->geom.h / 2;
	}

	if (cc->flags & CLIENT_HIDDEN)
		client_unhide(cc);
	else
		client_raise(cc);
	xu_ptr_setpos(cc->win, x, y);
}

void
client_ptrsave(struct client_ctx *cc)
{
	int	 x, y;

	xu_ptr_getpos(cc->win, &x, &y);
	if (client_inbound(cc, x, y)) {
		cc->ptr.x = x;
		cc->ptr.y = y;
	} else {
		cc->ptr.x = -1;
		cc->ptr.y = -1;
	}
}

void
client_hide(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_STICKY)
		return;

	XUnmapWindow(X_Dpy, cc->win);

	cc->flags &= ~CLIENT_ACTIVE;
	cc->flags |= CLIENT_HIDDEN;
	client_set_wm_state(cc, IconicState);

	if (cc == client_current())
		client_none(cc->sc);

	u_put_status();
}

void
client_unhide(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_STICKY)
		return;

	XMapRaised(X_Dpy, cc->win);

	cc->flags &= ~CLIENT_HIDDEN;
	client_set_wm_state(cc, NormalState);
	client_draw_border(cc);

	u_put_status();
}

void
client_urgency(struct client_ctx *cc)
{
	if (!(cc->flags & CLIENT_ACTIVE))
		cc->flags |= CLIENT_URGENCY;

	if (cc->sc != NULL)
		u_put_status();
}

void
client_draw_border(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	unsigned long		 pixel;

	if (cc->flags & CLIENT_ACTIVE)
		switch (cc->flags & CLIENT_HIGHLIGHT) {
		case CLIENT_GROUP:
			pixel = sc->xftcolor[CWM_COLOR_BORDER_GROUP].pixel;
			break;
		case CLIENT_UNGROUP:
			pixel = sc->xftcolor[CWM_COLOR_BORDER_UNGROUP].pixel;
			break;
		default:
			pixel = sc->xftcolor[CWM_COLOR_BORDER_ACTIVE].pixel;
			break;
		}
	else
		pixel = sc->xftcolor[CWM_COLOR_BORDER_INACTIVE].pixel;

	if (cc->flags & CLIENT_URGENCY)
		pixel = sc->xftcolor[CWM_COLOR_BORDER_URGENCY].pixel;

	XSetWindowBorderWidth(X_Dpy, cc->win, cc->bwidth);
	XSetWindowBorder(X_Dpy, cc->win, pixel);
}

static void
client_wm_protocols(struct client_ctx *cc)
{
	Atom	*p;
	int	 i, j;

	if (XGetWMProtocols(X_Dpy, cc->win, &p, &j)) {
		for (i = 0; i < j; i++) {
			if (p[i] == cwmh[WM_DELETE_WINDOW])
				cc->flags |= CLIENT_WM_DELETE_WINDOW;
			else if (p[i] == cwmh[WM_TAKE_FOCUS])
				cc->flags |= CLIENT_WM_TAKE_FOCUS;
		}
		XFree(p);
	}
}

void
client_wm_hints(struct client_ctx *cc)
{
	if ((cc->wmh = XGetWMHints(X_Dpy, cc->win)) == NULL)
		return;

	if ((cc->wmh->flags & InputHint) && (cc->wmh->input))
		cc->flags |= CLIENT_INPUT;

	if ((cc->wmh->flags & XUrgencyHint))
		client_urgency(cc);
}

void
client_msg(struct client_ctx *cc, Atom proto, Time ts)
{
	XClientMessageEvent	 cm;

	(void)memset(&cm, 0, sizeof(cm));
	cm.type = ClientMessage;
	cm.window = cc->win;
	cm.message_type = cwmh[WM_PROTOCOLS];
	cm.format = 32;
	cm.data.l[0] = proto;
	cm.data.l[1] = ts;

	XSendEvent(X_Dpy, cc->win, False, NoEventMask, (XEvent *)&cm);
}

void
client_send_delete(struct client_ctx *cc)
{
	if (cc->flags & CLIENT_WM_DELETE_WINDOW)
		client_msg(cc, cwmh[WM_DELETE_WINDOW], CurrentTime);
	else
		XKillClient(X_Dpy, cc->win);
}

void
client_setname(struct client_ctx *cc)
{
	struct winname	*wn;
	char		*newname;

	if (!xu_getstrprop(cc->win, ewmh[_NET_WM_NAME], &newname))
		if (!xu_getstrprop(cc->win, XA_WM_NAME, &newname))
			newname = xstrdup("");

	TAILQ_FOREACH(wn, &cc->nameq, entry) {
		if (strcmp(wn->name, newname) == 0) {
			/* Move to the last since we got a hit. */
			TAILQ_REMOVE(&cc->nameq, wn, entry);
			TAILQ_INSERT_TAIL(&cc->nameq, wn, entry);
			goto match;
		}
	}
	wn = xmalloc(sizeof(*wn));
	wn->name = newname;
	TAILQ_INSERT_TAIL(&cc->nameq, wn, entry);
	cc->nameqlen++;

match:
	cc->name = wn->name;

	/* Now, do some garbage collection. */
	if (cc->nameqlen > CLIENT_MAXNAMEQLEN) {
		wn = TAILQ_FIRST(&cc->nameq);
		assert(wn != NULL);
		TAILQ_REMOVE(&cc->nameq, wn, entry);
		free(wn->name);
		free(wn);
		cc->nameqlen--;
	}
	if (cc->sc != NULL)
		u_put_status();
}

void
client_cycle(struct screen_ctx *sc, int flags)
{
	struct client_ctx	*oldcc, *newcc;
	int			 again = 1;

	if (TAILQ_EMPTY(&sc->clientq))
		return;

	oldcc = client_current();
	if (oldcc == NULL)
		oldcc = (flags & CWM_RCYCLE ?
		    TAILQ_LAST(&sc->clientq, client_ctx_q) :
		    TAILQ_FIRST(&sc->clientq));

	newcc = oldcc;
	while (again) {
		again = 0;

		newcc = (flags & CWM_RCYCLE ? client_prev(newcc) :
		    client_next(newcc));

		/* Only cycle visible and non-ignored windows. */
		if ((newcc->flags & (CLIENT_HIDDEN|CLIENT_IGNORE))
		    || ((flags & CWM_INGROUP) &&
			(newcc->group != oldcc->group)))
			again = 1;

		/* Is oldcc the only non-hidden window? */
		if (newcc == oldcc) {
			if (again)
				return;	/* No windows visible. */

			break;
		}
	}

	/* reset when cycling mod is released. XXX I hate this hack */
	sc->cycling = 1;
	client_ptrsave(oldcc);
	client_ptrwarp(newcc);
}

void
client_cycle_leave(struct screen_ctx *sc)
{
	struct client_ctx	*cc;

	sc->cycling = 0;

	if ((cc = client_current())) {
		client_mtf(cc);
		group_toggle_membership_leave(cc);
		XUngrabKeyboard(X_Dpy, CurrentTime);
	}
}

static struct client_ctx *
client_next(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct client_ctx	*ccc;

	return((ccc = TAILQ_NEXT(cc, entry)) != NULL ?
	    ccc : TAILQ_FIRST(&sc->clientq));
}

static struct client_ctx *
client_prev(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;
	struct client_ctx	*ccc;

	return((ccc = TAILQ_PREV(cc, client_ctx_q, entry)) != NULL ?
	    ccc : TAILQ_LAST(&sc->clientq, client_ctx_q));
}

static void
client_placecalc(struct client_ctx *cc)
{
	struct screen_ctx	*sc;
	Window			 root = RootWindow(X_Dpy, DefaultScreen(X_Dpy));
	int			 xslack, yslack;

	if (cc->hint.flags & (USPosition|PPosition)) {
		/*
		 * Ignore XINERAMA screens, just make sure it's somewhere
		 * in the virtual desktop. else it stops people putting xterms
		 * at startup in the screen the mouse doesn't start in *sigh*.
		 * XRandR bits mean that {x,y}max shouldn't be outside what's
		 * currently there.
		 */
		fprintf(stderr, "positioning via hints\n");
		sc = screen_find_by_name(GLOBAL_SCREEN_NAME);
		xslack = sc->view.w - cc->geom.w - cc->bwidth * 2;
		yslack = sc->view.h - cc->geom.h - cc->bwidth * 2;
		cc->geom.x = MIN(cc->geom.x, xslack);
		cc->geom.y = MIN(cc->geom.y, yslack);
	} else {
		struct geom		 xine;
		int			 xmouse, ymouse;

		fprintf(stderr, "using the mouse\n");
		xu_ptr_getpos(root, &xmouse, &ymouse);
		xine = screen_find_xinerama(xmouse, ymouse, CWM_GAP);
		xine.w += xine.x;
		xine.h += xine.y;
		xmouse = MAX(xmouse, xine.x) - cc->geom.w / 2;
		ymouse = MAX(ymouse, xine.y) - cc->geom.h / 2;

		xmouse = MAX(xmouse, xine.x);
		ymouse = MAX(ymouse, xine.y);

		xslack = xine.w - cc->geom.w - cc->bwidth * 2;
		yslack = xine.h - cc->geom.h - cc->bwidth * 2;

		if (xslack >= xine.x) {
			cc->geom.x = MAX(MIN(xmouse, xslack), xine.x);
		} else {
			cc->geom.x = xine.x;
			cc->geom.w = xine.w;
		}
		if (yslack >= xine.y) {
			cc->geom.y = MAX(MIN(ymouse, yslack), xine.y);
		} else {
			cc->geom.y = xine.y;
			cc->geom.h = xine.h;
		}

		fprintf(stderr, "Placing at: x: %d, y: %d, w: %d, h: %d\n",
			cc->geom.x, cc->geom.y, cc->geom.w, cc->geom.h);
	}
}

static void
client_mtf(struct client_ctx *cc)
{
	struct screen_ctx	*sc = cc->sc;

	TAILQ_REMOVE(&sc->clientq, cc, entry);
	TAILQ_INSERT_HEAD(&sc->clientq, cc, entry);
}

void
client_getsizehints(struct client_ctx *cc)
{
	long		 tmp;
	XSizeHints	 size;

	if (!XGetWMNormalHints(X_Dpy, cc->win, &size, &tmp))
		size.flags = 0;

	cc->hint.flags = size.flags;

	if (size.flags & PBaseSize) {
		cc->hint.basew = size.base_width;
		cc->hint.baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		cc->hint.basew = size.min_width;
		cc->hint.baseh = size.min_height;
	}
	if (size.flags & PMinSize) {
		cc->hint.minw = size.min_width;
		cc->hint.minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		cc->hint.minw = size.base_width;
		cc->hint.minh = size.base_height;
	}
	if (size.flags & PMaxSize) {
		cc->hint.maxw = size.max_width;
		cc->hint.maxh = size.max_height;
	}
	if (size.flags & PResizeInc) {
		cc->hint.incw = size.width_inc;
		cc->hint.inch = size.height_inc;
	}
	cc->hint.incw = MAX(1, cc->hint.incw);
	cc->hint.inch = MAX(1, cc->hint.inch);

	if (size.flags & PAspect) {
		if (size.min_aspect.x > 0)
			cc->hint.mina = (float)size.min_aspect.y /
			    size.min_aspect.x;
		if (size.max_aspect.y > 0)
			cc->hint.maxa = (float)size.max_aspect.x /
			    size.max_aspect.y;
	}
}

void
client_applysizehints(struct client_ctx *cc)
{
	Bool		 baseismin;

	baseismin = (cc->hint.basew == cc->hint.minw) &&
	    (cc->hint.baseh == cc->hint.minh);

	/* temporarily remove base dimensions, ICCCM 4.1.2.3 */
	if (!baseismin) {
		cc->geom.w -= cc->hint.basew;
		cc->geom.h -= cc->hint.baseh;
	}

	/* adjust for aspect limits */
	if (cc->hint.mina && cc->hint.maxa) {
		if (cc->hint.maxa < (float)cc->geom.w / cc->geom.h)
			cc->geom.w = cc->geom.h * cc->hint.maxa;
		else if (cc->hint.mina < (float)cc->geom.h / cc->geom.w)
			cc->geom.h = cc->geom.w * cc->hint.mina;
	}

	/* remove base dimensions for increment */
	if (baseismin) {
		cc->geom.w -= cc->hint.basew;
		cc->geom.h -= cc->hint.baseh;
	}

	/* adjust for increment value */
	cc->geom.w -= cc->geom.w % cc->hint.incw;
	cc->geom.h -= cc->geom.h % cc->hint.inch;

	/* restore base dimensions */
	cc->geom.w += cc->hint.basew;
	cc->geom.h += cc->hint.baseh;

	/* adjust for min width/height */
	cc->geom.w = MAX(cc->geom.w, cc->hint.minw);
	cc->geom.h = MAX(cc->geom.h, cc->hint.minh);

	/* adjust for max width/height */
	if (cc->hint.maxw)
		cc->geom.w = MIN(cc->geom.w, cc->hint.maxw);
	if (cc->hint.maxh)
		cc->geom.h = MIN(cc->geom.h, cc->hint.maxh);

	cc->geom.w = MAX(cc->geom.w, 1);
	cc->geom.h = MAX(cc->geom.h, 1);

	cc->dim.w = (cc->geom.w - cc->hint.basew) / cc->hint.incw;
	cc->dim.h = (cc->geom.h - cc->hint.baseh) / cc->hint.inch;
}

static void
client_mwm_hints(struct client_ctx *cc)
{
	struct mwm_hints	*mwmh;

	if (xu_getprop(cc->win, cwmh[_MOTIF_WM_HINTS], cwmh[_MOTIF_WM_HINTS],
	    MWM_HINTS_ELEMENTS, (unsigned char **)&mwmh) == MWM_HINTS_ELEMENTS) {
		if (mwmh->flags & MWM_FLAGS_DECORATIONS &&
		    !(mwmh->decorations & MWM_DECOR_ALL) &&
		    !(mwmh->decorations & MWM_DECOR_BORDER))
			cc->bwidth = 0;
		XFree(mwmh);
	}
}

void
client_transient(struct client_ctx *cc)
{
	struct client_ctx	*tc;
	Window			 trans;

	if (XGetTransientForHint(X_Dpy, cc->win, &trans)) {
		if ((tc = client_find(trans)) && tc->group) {
			group_movetogroup(cc, tc->group->num);
			if (tc->flags & CLIENT_IGNORE)
				cc->flags |= CLIENT_IGNORE;
		}
	}
}

static int
client_inbound(struct client_ctx *cc, int x, int y)
{
	return(x < cc->geom.w && x >= 0 &&
	    y < cc->geom.h && y >= 0);
}

int
client_snapcalc(int n0, int n1, int e0, int e1, int snapdist)
{
	int	 s0, s1;

	s0 = s1 = 0;

	if (abs(e0 - n0) <= snapdist)
		s0 = e0 - n0;

	if (abs(e1 - n1) <= snapdist)
		s1 = e1 - n1;

	/* possible to snap in both directions */
	if (s0 != 0 && s1 != 0)
		if (abs(s0) < abs(s1))
			return(s0);
		else
			return(s1);
	else if (s0 != 0)
		return(s0);
	else if (s1 != 0)
		return(s1);
	else
		return(0);
}

void
client_htile(struct client_ctx *cc)
{
	struct client_ctx	*ci;
	struct group_ctx 	*gc = cc->group;
	struct geom 		 xine;
	int 			 i, n, mh, x, h, w;

	if (!gc)
		return;
	i = n = 0;

	TAILQ_FOREACH(ci, &gc->clientq, group_entry) {
		if (ci->flags & CLIENT_HIDDEN ||
		    ci->flags & CLIENT_IGNORE || (ci == cc))
			continue;
		n++;
	}
	if (n == 0)
		return;

	xine = screen_find_xinerama(
	    cc->geom.x + cc->geom.w / 2,
	    cc->geom.y + cc->geom.h / 2, CWM_GAP);

	if (cc->flags & CLIENT_VMAXIMIZED ||
	    cc->geom.h + (cc->bwidth * 2) >= xine.h)
		return;

	cc->flags &= ~CLIENT_HMAXIMIZED;
	cc->geom.x = xine.x;
	cc->geom.y = xine.y;
	cc->geom.w = xine.w - (cc->bwidth * 2);
	client_resize(cc, 1);
	client_ptrwarp(cc);

	mh = cc->geom.h + (cc->bwidth * 2);
	x = xine.x;
	w = xine.w / n;
	h = xine.h - mh;
	TAILQ_FOREACH(ci, &gc->clientq, group_entry) {
		if (ci->flags & CLIENT_HIDDEN ||
		    ci->flags & CLIENT_IGNORE || (ci == cc))
			continue;
		ci->bwidth = Conf.bwidth;
		ci->geom.y = xine.y + mh;
		ci->geom.x = x;
		ci->geom.h = h - (ci->bwidth * 2);
		ci->geom.w = w - (ci->bwidth * 2);
		if (i + 1 == n)
			ci->geom.w = xine.x + xine.w -
			    ci->geom.x - (ci->bwidth * 2);
		x += w;
		client_resize(ci, 1);
		i++;
	}
}

void
client_vtile(struct client_ctx *cc)
{
	struct client_ctx	*ci;
	struct group_ctx 	*gc = cc->group;
	struct geom 		 xine;
	int 			 i, n, mw, y, h, w;

	if (!gc)
		return;
	i = n = 0;

	TAILQ_FOREACH(ci, &gc->clientq, group_entry) {
		if (ci->flags & CLIENT_HIDDEN ||
		    ci->flags & CLIENT_IGNORE || (ci == cc))
			continue;
		n++;
	}
	if (n == 0)
		return;

	xine = screen_find_xinerama(
	    cc->geom.x + cc->geom.w / 2,
	    cc->geom.y + cc->geom.h / 2, CWM_GAP);

	if (cc->flags & CLIENT_HMAXIMIZED ||
	    cc->geom.w + (cc->bwidth * 2) >= xine.w)
		return;

	cc->flags &= ~CLIENT_VMAXIMIZED;
	cc->geom.x = xine.x;
	cc->geom.y = xine.y;
	cc->geom.h = xine.h - (cc->bwidth * 2);
	client_resize(cc, 1);
	client_ptrwarp(cc);

	mw = cc->geom.w + (cc->bwidth * 2);
	y = xine.y;
	h = xine.h / n;
	w = xine.w - mw;
	TAILQ_FOREACH(ci, &gc->clientq, group_entry) {
		if (ci->flags & CLIENT_HIDDEN ||
		    ci->flags & CLIENT_IGNORE || (ci == cc))
			continue;
		ci->bwidth = Conf.bwidth;
		ci->geom.y = y;
		ci->geom.x = xine.x + mw;
		ci->geom.h = h - (ci->bwidth * 2);
		ci->geom.w = w - (ci->bwidth * 2);
		if (i + 1 == n)
			ci->geom.h = xine.y + xine.h -
			    ci->geom.y - (ci->bwidth * 2);
		y += h;
		client_resize(ci, 1);
		i++;
	}
}

long
client_get_wm_state(struct client_ctx *cc)
{
	long	*p, state = -1;

	if (xu_getprop(cc->win, cwmh[WM_STATE], cwmh[WM_STATE], 2L,
	    (unsigned char **)&p) > 0) {
		state = *p;
		XFree(p);
	}
	return(state);
}

void
client_set_wm_state(struct client_ctx *cc, long state)
{
	long	 data[] = { state, None };

	XChangeProperty(X_Dpy, cc->win, cwmh[WM_STATE], cwmh[WM_STATE], 32,
	    PropModeReplace, (unsigned char *)data, 2);
}

