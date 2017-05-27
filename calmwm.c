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
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

#define LOGFILE_NAME "cwm-new.log"

Display				*X_Dpy;
Time				 Last_Event_Time = CurrentTime;
Atom				 cwmh[CWMH_NITEMS];
Atom				 ewmh[EWMH_NITEMS];

struct screen_ctx_q		 Screenq = TAILQ_HEAD_INITIALIZER(Screenq);

int				 HasRandr, Randr_ev;
const char			*homedir;
volatile sig_atomic_t		 cwm_status;

static void	sighdlr(int);
static int	x_errorhandler(Display *, XErrorEvent *);
static void	x_init(const char *);
static void	x_restart(char **);
static void	x_teardown(void);
static int	x_wmerrorhandler(Display *, XErrorEvent *);

int
main(int argc, char **argv)
{
	extern char	*__progname;
	char		*conf_file = NULL, *display_name = NULL;
	char		**cwm_argv;
	int		 ch;
	struct passwd	*pw;
	bool		 open_logfile = false;
	char		*pipe_name = NULL;

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		warnx("no locale support");
	mbtowc(NULL, NULL, MB_CUR_MAX);

	cwm_argv = argv;
	while ((ch = getopt(argc, argv, "vc:d:p:")) != -1) {
		switch (ch) {
		case 'c':
			conf_file = optarg;
			break;
		case 'd':
			display_name = optarg;
			break;
		case 'p':
			pipe_name = optarg;
		case 'v':
			open_logfile = true;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (signal(SIGCHLD, sighdlr) == SIG_ERR)
		err(1, "signal");

	if (open_logfile) {
		log_open(LOGFILE_NAME);
	}

	if ((homedir = getenv("HOME")) == NULL || *homedir == '\0') {
		pw = getpwuid(getuid());
		if (pw != NULL && pw->pw_dir != NULL && *pw->pw_dir != '\0')
			homedir = pw->pw_dir;
		else
			homedir = "/";
	}

	if (conf_file == NULL)
		xasprintf(&conf_path, "%s/%s", homedir, CONFFILE);
	else
		conf_path = xstrdup(conf_file);

	if (pipe_name != NULL)
		cwm_pipe = xstrdup(pipe_name);
	else
		cwm_pipe = xstrdup(CWMPIPE);

	if (access(conf_path, R_OK) != 0) {
		free(conf_path);
		conf_path = NULL;
	}

	log_debug("%s starting...", __progname);
	if (conf_path != NULL)
		log_debug("Using config: %s", conf_path);

	x_init(display_name);

	cwm_status = CWM_RUNNING;
	while (cwm_status == CWM_RUNNING)
		xev_process();
	x_teardown();
	if (cwm_status == CWM_RESTART)
		x_restart(cwm_argv);

	return(0);
}

static void
x_init(const char *dpyname)
{
	if ((X_Dpy = XOpenDisplay(dpyname)) == NULL)
		log_fatal("unable to open display \"%s\"",
		    XDisplayName(dpyname));

	XSetErrorHandler(x_wmerrorhandler);
	XSelectInput(X_Dpy, DefaultRootWindow(X_Dpy), SubstructureRedirectMask);
	XSync(X_Dpy, False);
	XSetErrorHandler(x_errorhandler);

	conf_atoms();
	u_init_pipe();
	screen_maybe_init_randr();
}

static void
x_restart(char **args)
{
	(void)setsid();
	(void)execvp(args[0], args);
}

static void
x_teardown(void)
{
	struct screen_ctx	*sc;
	struct group_ctx	*gc;
	struct config_screen	*cscr;
	struct config_group	*cgrp;
	unsigned int		 i;

	conf_clear();

	TAILQ_FOREACH(sc, &Screenq, entry) {
		cscr = sc->config_screen;
		cgrp = sc->group_current->config_group;

		XftDrawDestroy(sc->xftdraw);
		TAILQ_FOREACH(gc, &sc->groupq, entry) {
			cgrp = gc->config_group;

			for (i = 0; i < CWM_COLOR_NITEMS; i++) {
				XftColorFree(X_Dpy, DefaultVisual(X_Dpy,
				    sc->which),
				    DefaultColormap(X_Dpy, sc->which),
				    &cgrp->xftcolor[i]);

				XftFontClose(X_Dpy, cgrp->xftfont);
			}

			for (i = 0; i < CF_NITEMS; i++)
				XFreeCursor(X_Dpy, cscr->cursor[i]);
		}
		XUnmapWindow(X_Dpy, sc->menuwin);
		XDestroyWindow(X_Dpy, sc->menuwin);
	}
	XUngrabKey(X_Dpy, AnyKey, AnyModifier,
		RootWindow(X_Dpy, DefaultScreen(X_Dpy)));
	XUngrabPointer(X_Dpy, CurrentTime);
	XUngrabKeyboard(X_Dpy, CurrentTime);
	XSync(X_Dpy, False);
	XSetInputFocus(X_Dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

static int
x_wmerrorhandler(Display *dpy, XErrorEvent *e)
{
	log_fatal("root window unavailable - perhaps another wm is running?");

	return (0);
}

static int
x_errorhandler(Display *dpy, XErrorEvent *e)
{
#ifdef DEBUG
	char msg[80], number[80], req[80];

	XGetErrorText(X_Dpy, e->error_code, msg, sizeof(msg));
	(void)snprintf(number, sizeof(number), "%d", e->request_code);
	XGetErrorDatabaseText(X_Dpy, "XRequest", number,
	    "<unknown>", req, sizeof(req));

	warnx("%s(0x%x): %s", req, (unsigned int)e->resourceid, msg);
#endif
	return (0);
}

static void
sighdlr(int sig)
{
	pid_t	 pid;
	int	 save_errno = errno, status;

	switch (sig) {
	case SIGCHLD:
		/* Collect dead children. */
		while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0 ||
		    (pid < 0 && errno == EINTR))
			;
		break;
	}

	errno = save_errno;
}

void
usage(void)
{
	extern char	*__progname;

	fprintf(stderr, "usage: %s [-c file] [-d display] [-p pipe] [-v]\n",
	    __progname);
	exit(1);
}
