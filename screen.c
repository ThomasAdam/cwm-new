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

static void	 screen_create_randr_region(struct screen_ctx *, const char *,
    struct geom *, int);
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
	log_debug("%s: couldn't find monitor '%s'", __func__, name);
	return (NULL);
}

void
screen_maybe_init_randr(void)
{
	XRRScreenResources	*screen_res;
	XRROutputInfo		*oinfo;
	XRRCrtcInfo		*crtc_info;
	RROutput		 rr_output, rr_output_primary;
	struct screen_ctx	*sc;
	struct geom		 size;
	int			 iscres, i, is_primary = 0;

	if (!XRRQueryExtension(X_Dpy, &Randr_ev, &i)) {
		/* No RandR present.  Single screen only. */
		log_debug("No RandR present; using single screen...");
		goto single_screen;
	}

	screen_res = XRRGetScreenResources(X_Dpy, DefaultRootWindow(X_Dpy));

	if (screen_res != NULL && screen_res->noutput == 0) {
		/* Single screen only. */
		log_debug("noutput: %d - trying to create single_screen",
			screen_res->noutput);
		XRRFreeScreenResources(screen_res);
		goto single_screen;
	}

        for (iscres = 0; iscres < screen_res->ncrtc; iscres++) {
	    crtc_info = XRRGetCrtcInfo(X_Dpy, screen_res,
		screen_res->crtcs[iscres]);

	    if (crtc_info->noutput == 0) {
		    log_debug("Screen found, but no crtc info present");
		    XRRFreeCrtcInfo(crtc_info);
		    continue;
	    }

	    rr_output = crtc_info->outputs[0];
	    rr_output_primary = XRRGetOutputPrimary(X_Dpy,
		DefaultRootWindow(X_Dpy));

	    if (rr_output == rr_output_primary)
		    is_primary = 1;
	    else
		    is_primary = 0;

            oinfo = XRRGetOutputInfo (X_Dpy, screen_res, rr_output);

	    if (oinfo == NULL) {
		    log_debug("Tried to find monitor '%d' but no oinfo present",
			iscres);
		    continue;
	    }

	    /* If the display isn't registered then move on to the next. */
	    if (oinfo->connection != RR_Connected) {
		    log_debug("Tried to create monitor '%s' but not connected",
			oinfo->name);
		    XRRFreeOutputInfo(oinfo);
		    XRRFreeCrtcInfo(crtc_info);
		    continue;
	    }

	    sc = xcalloc(1, sizeof(*sc));
	    size.x = crtc_info->x;
	    size.y = crtc_info->y;
	    size.w = crtc_info->width;
	    size.h = crtc_info->height;

	    screen_create_randr_region(sc, oinfo->name, &size, is_primary);
	    no_of_screens++;

	    XRRFreeCrtcInfo(crtc_info);
	    XRRFreeOutputInfo(oinfo);
        }
        XRRFreeScreenResources(screen_res);

single_screen:
	if (no_of_screens >= 1) {
		log_debug("Request single_screen but > 1 screen.  Skipping...");
		goto out;
	}

	sc = xcalloc(1, sizeof(*sc));
	sc->which = DefaultScreen(X_Dpy);
	size.x = 0;
	size.y = 0;
	size.w = DisplayWidth(X_Dpy, sc->which);
	size.h = DisplayHeight(X_Dpy, sc->which);

	screen_create_randr_region(sc, GLOBAL_SCREEN_NAME, &size, is_primary);
out:
	screen_init_contents();
}

/* Each RandR output is its own screen_ctx */
static void
screen_create_randr_region(struct screen_ctx *sc, const char *name,
    struct geom *geom, int is_primary)
{
	log_debug("RandR output:\t%s %s\n", name, is_primary ? "(primary)" : "");
	log_debug("\t\tx: %d\n\t\ty: %d\n\t\tw: %d\n\t\th: %d\n",
		geom->x, geom->y, geom->w, geom->h);

	sc->name = xstrdup(name);
	memcpy(&sc->view, geom, sizeof(*geom));

	if (is_primary) {
		sc->is_primary = 1;
		TAILQ_INSERT_HEAD(&Screenq, sc, entry);
	} else {
		sc->is_primary = 0;
		TAILQ_INSERT_TAIL(&Screenq, sc, entry);
	}

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

	/* Run the panel command for each screen. */
	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (sc->config_screen->panel_cmd != NULL)
			u_spawn(sc->config_screen->panel_cmd);
	}
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
screen_find_screen(int x, int y, struct screen_ctx *s)
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

	if (sc_ret == NULL) {
		/* Most likely, the monitor resolutions have found dead space,
		 * which would put a window on an area where no screen
		 * existed.
		 *
		 * For now, return the original screen.
		 */
		log_debug("%s: couldn't find monitor at %dx%d", __func__, x, y);
		if (s != NULL)
			return (s);

		/* Return the first sccreen if s == NULL. */
		return (TAILQ_FIRST(&Screenq));
	}

	return (sc_ret);
}

/*
 * Find which geometry the screen has at  the coordinates x,y.
 */
struct geom
screen_find_xinerama(int x, int y, int flags)
{
	struct screen_ctx	*sc = screen_find_screen(x, y, NULL);
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
