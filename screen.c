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

#include <sys/param.h>
#include "queue.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

void
screen_init_screens(void)
{
	int			 i, j, screen_count = 0, xinerama_screens = 0;
	XineramaScreenInfo	*info = NULL;

	if (XineramaIsActive(X_Dpy)) {
		info = XineramaQueryScreens(X_Dpy, &xinerama_screens);

		fprintf(stderr, "Xinerama:  %d\n", xinerama_screens);
	}

	screen_count = ScreenCount(X_Dpy);

	fprintf(stderr, "ScreenCount: %d\n", screen_count);

	for (i = 0; i < screen_count; i++) {
		for (j = 0; j < xinerama_screens; j++) {
			screen_init(i, &info[j]);
		}
	}

	if (info)
		XFree(info);
}

void
screen_init(int which, XineramaScreenInfo *xin_info)
{
	struct screen_ctx	*sc;
	struct client_ctx	**clients = NULL;
	Window			*wins, w0, w1;
	XSetWindowAttributes	 rootattr;
	unsigned int		 nwins, i;

	sc = xcalloc(1, sizeof(*sc));

	TAILQ_INIT(&sc->mruq);

	sc->which = which;
	sc->xinerama_no = -1;
	sc->has_xinerama = 0;
	sc->xinerama_info = NULL;

	if (xin_info != NULL) {
		sc->has_xinerama = 1;
		sc->xinerama_no = xin_info->screen_number;
		sc->xinerama_info = xin_info;
	}

	sc->rootwin = RootWindow(X_Dpy, sc->which);
	conf_screen(sc);

	xu_ewmh_net_supported(sc);
	xu_ewmh_net_supported_wm_check(sc);

	screen_update_geometry(sc);
	group_init(sc);

	rootattr.cursor = Conf.cursor[CF_NORMAL];
	rootattr.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|
	    PropertyChangeMask|EnterWindowMask|LeaveWindowMask|
	    ColormapChangeMask|BUTTONMASK;

	XChangeWindowAttributes(X_Dpy, sc->rootwin,
	    CWEventMask|CWCursor, &rootattr);

	/* Deal with existing clients. */
	if (XQueryTree(X_Dpy, sc->rootwin, &w0, &w1, &wins, &nwins)) {
		clients = xcalloc(nwins, sizeof(*clients));
		for (i = 0; i < nwins; i++)
			clients[i] = client_init(wins[i], sc);

		XFree(wins);
	}
	screen_updatestackingorder(sc);
	group_set_state(sc);

	if (HasRandr)
		XRRSelectInput(X_Dpy, sc->rootwin, RRScreenChangeNotifyMask);

	TAILQ_INSERT_TAIL(&Screenq, sc, entry);

	for (i = 0; i < nwins; i++) {
		if (clients[i] == NULL)
			continue;

		client_update_xinerama(clients[i]);
	}
	free(clients);

	XSync(X_Dpy, False);
}

struct screen_ctx *
screen_fromroot(Window rootwin)
{
	struct screen_ctx	*sc;
	int			 posx, posy;

	xu_ptr_getpos(rootwin, &posx, &posy);

	TAILQ_FOREACH(sc, &Screenq, entry)
		if (sc->rootwin == rootwin &&
		    (posx >= sc->view.x && posx < sc->view.x + sc->view.w) &&
		    (posy >= sc->view.y && posy < sc->view.y + sc->view.h))
			return (sc);

	/* XXX FAIL HERE */
	return (TAILQ_FIRST(&Screenq));
}

void
screen_updatestackingorder(struct screen_ctx *sc)
{
	Window			*wins, w0, w1;
	struct client_ctx	*cc;
	unsigned int		 nwins, i, s;

	if (XQueryTree(X_Dpy, sc->rootwin, &w0, &w1, &wins, &nwins)) {
		for (s = 0, i = 0; i < nwins; i++) {
			/* Skip hidden windows */
			if ((cc = client_find(wins[i])) == NULL ||
			    cc->flags & CLIENT_HIDDEN)
				continue;
	
			cc->stackingorder = s++;
		}
		XFree(wins);
	}
}

/*
 * Find which xinerama screen the coordinates (x,y) is on.
 */
struct geom
screen_find_xinerama(struct screen_ctx *sc, int x, int y, int flags)
{
	struct screen_ctx	*sc1;
	struct geom		 geom;

	TAILQ_FOREACH(sc1, &Screenq, entry) {
		if (x >= sc1->view.x && x < sc1->view.x + sc1->view.w &&
		    y >= sc1->view.y && y < sc1->view.y + sc1->view.h) {
			geom = sc1->view;
			break;
		}
	}

	if (flags & CWM_GAP) {
		geom.x += sc->gap.left;
		geom.y += sc->gap.top;
		geom.w -= (sc->gap.left + sc->gap.right);
		geom.h -= (sc->gap.top + sc->gap.bottom);
	}
	return (geom);
}

void
screen_update_geometry(struct screen_ctx *sc)
{
	sc->view.x = sc->has_xinerama ? sc->xinerama_info->x_org : 0;
	sc->view.y = sc->has_xinerama ? sc->xinerama_info->y_org : 0;
	sc->view.w = sc->has_xinerama ?
		sc->xinerama_info->width :
		DisplayWidth(X_Dpy, sc->which);
	sc->view.h = sc->has_xinerama ?
		sc->xinerama_info->height :
		DisplayHeight(X_Dpy, sc->which);

	sc->work.x = sc->view.x + sc->gap.left;
	sc->work.y = sc->view.y + sc->gap.top;
	sc->work.w = sc->view.w - (sc->gap.left + sc->gap.right);
	sc->work.h = sc->view.h - (sc->gap.top + sc->gap.bottom);

	xu_ewmh_net_desktop_geometry(sc);
	xu_ewmh_net_workarea(sc);
}
