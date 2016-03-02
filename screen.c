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

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

static int	 no_of_screens;

static void			 screen_create_randr_region(struct screen_ctx *,
    const char *, struct geom *);
static void	 screen_init_contents(void);

struct screen_ctx *
screen_find_by_name(const char *name)
{
	struct screen_ctx	*sc;

	if (name == NULL)
		log_fatal("%s: name was NULL", __func__);

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (strcmp(sc->name, name) == 0) {
			return (sc);
		}
	}
	log_fatal("%s: couldn't find monitor '%s'", __func__, name);
}

void
screen_maybe_init_randr(void)
{
	XRRScreenResources	*screen_res;
        XRROutputInfo		*oinfo;
        XRRCrtcInfo		*crtc_info;
	struct screen_ctx	*sc;
	struct geom		 size;
        int			 iscres, i;

	if (!XRRQueryExtension(X_Dpy, &Randr_ev, &i)) {
		/* No RandR present.  Single screen only. */
		log_debug("No RandR present; using single screen...");
		goto single_screen;
	}

	screen_res = XRRGetScreenResources(X_Dpy, DefaultRootWindow(X_Dpy));

	if (screen_res != NULL && screen_res->noutput == 0) {
		/* Single screen only. */
		XRRFreeScreenResources(screen_res);
		goto single_screen;
	}

        for (iscres = screen_res->noutput; iscres > 0; iscres--) {
            oinfo = XRRGetOutputInfo (X_Dpy, screen_res,
		screen_res->outputs[iscres]);

	    if (oinfo == NULL)
		    continue;

	    /* If the display isn't registered then move on to the next. */
	    if (oinfo->connection != RR_Connected)
		    continue;

	    crtc_info = XRRGetCrtcInfo(X_Dpy, screen_res, oinfo->crtc);

	    if (crtc_info == NULL)
		    continue;

	    sc = xcalloc(1, sizeof(*sc));
	    size.x = crtc_info->x;
	    size.y = crtc_info->y;
	    size.w = crtc_info->width;
	    size.h = crtc_info->height;

	    screen_create_randr_region(sc, oinfo->name, &size);
	    no_of_screens++;

	    XRRFreeCrtcInfo(crtc_info);
	    XRRFreeOutputInfo(oinfo);
        }
        XRRFreeScreenResources(screen_res);

single_screen:
	if (no_of_screens > 1)
		goto out;

	sc = xcalloc(1, sizeof(*sc));
	sc->which = DefaultScreen(X_Dpy);
	size.x = 0;
	size.y = 0;
	size.w = DisplayWidth(X_Dpy, sc->which);
	size.h = DisplayHeight(X_Dpy, sc->which);

	screen_create_randr_region(sc, GLOBAL_SCREEN_NAME, &size);
out:
	screen_init_contents();
}

/* Each RandR output is its own screen_ctx */
static void
screen_create_randr_region(struct screen_ctx *sc, const char *name,
    struct geom *geom)
{
	log_debug("RandR output:\t%s\n", name);
	log_debug("\t\tx: %d\n\t\ty: %d\n\t\tw: %d\n\t\th: %d\n",
		geom->x, geom->y, geom->w, geom->h);

	sc->name = xstrdup(name);
	memcpy(&sc->view, geom, sizeof(*geom));

	TAILQ_INSERT_TAIL(&Screenq, sc, entry);

	XRRSelectInput(X_Dpy, sc->rootwin, RRScreenChangeNotifyMask);
}

void
screen_apply_ewmh(void)
{
	struct screen_ctx	*sc;
	XSetWindowAttributes	 rootattr;

	/*
	 * Setting up hints only requires one of the outputs to do this on,
	 * since the root window is the same for all outputs.  Doing this
	 * multiple times for all outputs is not what should be done---the
	 * XServer will deadlock trying to reacquire the properties on the
	 * root window.
	 */
	{
		sc = TAILQ_FIRST(&Screenq);

		xu_ewmh_net_supported(sc);
		xu_ewmh_net_supported_wm_check(sc);
		xu_ewmh_net_desktop_names(sc);
		xu_ewmh_net_wm_desktop_viewport(sc);
		xu_ewmh_net_wm_number_of_desktops(sc);
		xu_ewmh_net_showing_desktop(sc);
		xu_ewmh_net_virtual_roots(sc);

		rootattr.cursor = sc->config_screen->cursor[CF_NORMAL];
		rootattr.event_mask = SubstructureRedirectMask|
			SubstructureNotifyMask|PropertyChangeMask|
			EnterWindowMask|LeaveWindowMask|
			ColormapChangeMask|BUTTONMASK;

		XChangeWindowAttributes(X_Dpy, sc->rootwin,
		    CWEventMask|CWCursor, &rootattr);

		XSync(X_Dpy, False);
	}
}

void
screen_init_contents(void)
{
	struct screen_ctx	*sc;
	unsigned int		 i;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		TAILQ_INIT(&sc->clientq);
		TAILQ_INIT(&sc->groupq);

		sc->which = DefaultScreen(X_Dpy);
		sc->rootwin = RootWindow(X_Dpy, sc->which);
		sc->cycling = 0;
		sc->hideall = 0;
		sc->menuwin = 0;
		sc->xftdraw = NULL;
		sc->config_screen = xmalloc(sizeof(sc->config_screen));

		log_debug("%s: Adding groups...", __func__);
		for (i = 0; i < CALMWM_NGROUPS; i++)
			group_init(sc, i);
	}
	screen_apply_ewmh();
	config_parse();
	client_scan_for_windows();
}

struct screen_ctx *
screen_find(Window win)
{
	struct screen_ctx	*sc;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (sc->rootwin == win)
			return(sc);
	}
	/* XXX FAIL HERE */
	return(TAILQ_FIRST(&Screenq));
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

/* Find out which screen is at x, y */
struct screen_ctx *
screen_find_screen(int x, int y)
{
	struct screen_ctx	*sc, *sc_ret = NULL;
	int			 x1 = abs(x), y1 = abs(y);

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (x1 >= sc->view.x && x1 < sc->view.x + sc->view.w &&
		    y1 >= sc->view.y && y1 < sc->view.y + sc->view.h) {
			sc_ret = sc;
			break;
		}
	}

	if (sc_ret == NULL)
		log_fatal("%s: couldn't find monitor at %dx%d", __func__, x, y);

	return (sc_ret);
}

/*
 * Find which geometry the screen has at  the coordinates x,y.
 */
struct geom
screen_find_xinerama(int x, int y, int flags)
{
	struct screen_ctx	*sc = screen_find_screen(x, y);
	struct config_screen	*cscr = sc->config_screen;
	struct geom		 g = sc->view;

	if (flags & CWM_GAP) {
		g.x += cscr->gap.left;
		g.y += cscr->gap.top;
		g.w -= (cscr->gap.left + cscr->gap.right);
		g.h -= (cscr->gap.top + cscr->gap.bottom);
	}
	return(g);
}

void
screen_update_geometry(struct screen_ctx *sc)
{
	struct config_screen	*cscr = sc->config_screen;

	sc->work.x = sc->view.x + cscr->gap.left;
	sc->work.y = sc->view.y + cscr->gap.top;
	sc->work.w = sc->view.w - (cscr->gap.left + cscr->gap.right);
	sc->work.h = sc->view.h - (cscr->gap.top + cscr->gap.bottom);

	xu_ewmh_net_desktop_geometry(sc);
	xu_ewmh_net_workarea(sc);
}
