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

#ifndef _CALMWM_H_
#define _CALMWM_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/param.h>
#include <confuse.h>
#if defined(__linux__)
#	include "compat/queue.h"
#	include "compat/tree.h"
#else
#include <sys/queue.h>
#include <sys/tree.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>

#include "array.h"
#include "cmd.h"
#include "config.h"

#ifndef __dead
#define __dead __attribute__ ((__noreturn__))
#endif

/* Definition to shut gcc up about unused arguments. */
#define unused __attribute__ ((unused))

/* Attribute to make gcc check printf-like arguments. */
#define printflike1 __attribute__ ((format (printf, 1, 2)))
#define printflike2 __attribute__ ((format (printf, 2, 3)))
#define printflike3 __attribute__ ((format (printf, 3, 4)))
#define printflike4 __attribute__ ((format (printf, 4, 5)))
#define printflike5 __attribute__ ((format (printf, 5, 6)))

#undef MIN
#undef MAX
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#ifndef nitems
#define nitems(_a) (sizeof((_a)) / sizeof((_a)[0]))
#endif

#define	CONFFILE	".cwm-newrc"
#define CWMPIPE		"/tmp/cwm.pipe"
#define	WMNAME	 	"CWM"

#define BUTTONMASK	(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK	(BUTTONMASK|PointerMotionMask)
#define MENUMASK 	(MOUSEMASK|ButtonMotionMask|ExposureMask)
#define MENUGRABMASK	(MOUSEMASK|ButtonMotionMask|StructureNotifyMask)
#define KEYMASK		(KeyPressMask|ExposureMask)
#define IGNOREMODMASK	(LockMask|Mod2Mask)

/* kb movement */
#define CWM_MOVE		0x0001
#define CWM_RESIZE		0x0002
#define CWM_PTRMOVE		0x0004
#define CWM_BIGMOVE		0x0008
#define CWM_UP			0x0010
#define CWM_DOWN		0x0020
#define CWM_LEFT		0x0040
#define CWM_RIGHT		0x0080

/* exec */
#define	CWM_EXEC_PROGRAM	0x0001
#define	CWM_EXEC_WM		0x0002

/* cycle */
#define CWM_CYCLE		0x0001
#define CWM_RCYCLE		0x0002
#define CWM_INGROUP		0x0004

/* menu */
#define CWM_MENU_DUMMY		0x0001
#define CWM_MENU_FILE		0x0002
#define CWM_MENU_LIST		0x0004

#define ARG_CHAR		0x0001
#define ARG_INT			0x0002

#define CWM_TILE_HORIZ 		0x0001
#define CWM_TILE_VERT 		0x0002

#define CWM_GAP			0x0001
#define CWM_NOGAP		0x0002

#define CWM_WIN			0x0001
#define CWM_CMD			0x0002

#define CWM_QUIT		0x0000
#define CWM_RUNNING		0x0001
#define CWM_RESTART		0x0002

#define CWM_SNAP_UP		0x0001
#define CWM_SNAP_DOWN		0x0002
#define CWM_SNAP_LEFT		0x0004
#define CWM_SNAP_RIGHT		0x0008

long long	 strtonum(const char *, long long, long long, const char **);
size_t		 strlcpy(char *, const char *, size_t);
size_t		 strlcat(char *, const char*, size_t);
void		*reallocarray(void *, size_t, size_t);
char		*fgetln(FILE *, size_t *);
char		*fparseln(FILE *, size_t *, size_t *, const char *, int);

union arg {
	char	*c;
	int	 i;
};

union press {
	KeySym		 keysym;
	unsigned int	 button;
};

enum cursor_font {
	CF_DEFAULT,
	CF_MOVE,
	CF_NORMAL,
	CF_QUESTION,
	CF_RESIZE,
	CF_NITEMS
};

enum color {
	CWM_COLOR_BORDER_ACTIVE,
	CWM_COLOR_BORDER_INACTIVE,
	CWM_COLOR_BORDER_URGENCY,
	CWM_COLOR_BORDER_GROUP,
	CWM_COLOR_BORDER_UNGROUP,
	CWM_COLOR_MENU_FG,
	CWM_COLOR_MENU_BG,
	CWM_COLOR_MENU_FONT,
	CWM_COLOR_MENU_FONT_SEL,
	CWM_COLOR_NITEMS
};

struct geom {
	int		 x;
	int		 y;
	int		 w;
	int		 h;
};

struct gap {
	int		 top;
	int		 bottom;
	int		 left;
	int		 right;
};

struct winname {
	TAILQ_ENTRY(winname)	 entry;
	char			*name;
};
TAILQ_HEAD(winname_q, winname);
TAILQ_HEAD(ignore_q, winname);

struct rule_item {
	struct binding		*b;
	const char		*name;
	TAILQ_ENTRY(rule_item)	 entry;
};
TAILQ_HEAD(rule_item_q, rule_item);

struct rule {
	const char		*client_class;
	const char		*rule_name;
	struct client_ctx	*cc;
	size_t			 ri_size;
	struct rule_item_q	 rule_item;

	TAILQ_ENTRY(rule)	 entry;
};
TAILQ_HEAD(rule_q, rule);

struct geom_record {
	struct screen_ctx		*sc;
	struct geom	 		 geom;
	int		 		 group;

	TAILQ_ENTRY(geom_record)	 entry;
};
TAILQ_HEAD(geom_recordq, geom_record);

struct config_client;
struct client_ctx {
	TAILQ_ENTRY(client_ctx)	 entry;
	TAILQ_ENTRY(client_ctx)	 group_entry;
	struct geom_recordq	 geom_recordq;
	struct screen_ctx	*sc;
	struct config_client	*c_cfg;
	Window			 win;
	Colormap		 colormap;
	unsigned int		 bwidth; /* border width */
	struct geom		 geom, savegeom, fullgeom;
	struct {
		long		 flags;	/* defined hints */
		int		 basew;	/* desired width */
		int		 baseh;	/* desired height */
		int		 minw;	/* minimum width */
		int		 minh;	/* minimum height */
		int		 maxw;	/* maximum width */
		int		 maxh;	/* maximum height */
		int		 incw;	/* width increment progression */
		int		 inch;	/* height increment progression */
		float		 mina;	/* minimum aspect ratio */
		float		 maxa;	/* maximum aspect ratio */
	} hint;
	struct {
		int		 x;	/* x position */
		int		 y;	/* y position */
	} ptr;
	struct {
		int		 h;	/* height */
		int		 w;	/* width */
	} dim;
#define CLIENT_HIDDEN			0x0001
#define CLIENT_IGNORE			0x0002
#define CLIENT_VMAXIMIZED		0x0004
#define CLIENT_HMAXIMIZED		0x0008
#define CLIENT_FREEZE			0x0010
#define CLIENT_GROUP			0x0020
#define CLIENT_UNGROUP			0x0040
#define CLIENT_INPUT			0x0080
#define CLIENT_WM_DELETE_WINDOW		0x0100
#define CLIENT_WM_TAKE_FOCUS		0x0200
#define CLIENT_URGENCY			0x0400
#define CLIENT_FULLSCREEN		0x0800
#define CLIENT_STICKY			0x1000
#define CLIENT_ACTIVE			0x2000
#define CLIENT_EXPANDED			0x4000
#define CLIENT_BORDER			0x8000

#define CLIENT_HIGHLIGHT		(CLIENT_GROUP | CLIENT_UNGROUP)
#define CLIENT_MAXFLAGS			(CLIENT_VMAXIMIZED | CLIENT_HMAXIMIZED)
#define CLIENT_MAXIMIZED		(CLIENT_VMAXIMIZED | CLIENT_HMAXIMIZED)
	int			 flags;
	int			 stackingorder;
	int			 extended_data;
	struct winname_q	 nameq;
#define CLIENT_MAXNAMEQLEN		5
	int			 nameqlen;
	char			*name;
	char			*label;
	char			*matchname;
	struct group_ctx	*group;
	XClassHint		ch;
	XWMHints		*wmh;
};
TAILQ_HEAD(client_ctx_q, client_ctx);

struct config_group;
struct group_ctx {
	TAILQ_ENTRY(group_ctx)	 entry;
	struct screen_ctx	*sc;
	struct config_group	*config_group;
	char			*name;
	int			 num;
#define GROUP_ACTIVE		 0x0001
#define GROUP_HIDDEN		 0x0002
	int			 flags;
	struct client_ctx_q	 clientq;
};
TAILQ_HEAD(group_ctx_q, group_ctx);

struct autogroupwin {
	TAILQ_ENTRY(autogroupwin)	 entry;
	char				*class;
	char				*name;
	int 				 num;
};
TAILQ_HEAD(autogroupwin_q, autogroupwin);

#define GLOBAL_SCREEN_NAME "global_monitor"
struct config_screen;
struct screen_ctx {
	TAILQ_ENTRY(screen_ctx)	 entry;
	const char		*name;
	int			 which;
	int			 is_primary;
	Window			 rootwin;
	Window			 menuwin;
	int			 cycling;
	int			 hideall;
	struct config_screen	*config_screen;
	struct geom		 view; /* viewable area */
	struct geom		 work; /* workable area, gap-applied */
	struct client_ctx_q	 clientq;
#define CALMWM_NGROUPS		 10
	struct group_ctx_q	 groupq;
	struct group_ctx	*group_current;
	XftDraw			*xftdraw;
};
TAILQ_HEAD(screen_ctx_q, screen_ctx);

enum binding_type
{
	BINDING_NONE = 0,
	BINDING_KEY,
	BINDING_MOUSE,
};

struct binding {
	TAILQ_ENTRY(binding)	 entry;
	void			(*callback)(struct client_ctx *, union arg *);
	union arg		 argument;
	unsigned int		 modmask;
	union press		 press;
	int			 flags;
	enum binding_type	 type;
};
TAILQ_HEAD(binding_q, binding);
TAILQ_HEAD(keybinding_q, binding);
TAILQ_HEAD(mousebinding_q, binding);

struct cmd_path {
	TAILQ_ENTRY(cmd_path)    entry;
	char                    *name;
	char                     path[PATH_MAX];
};
TAILQ_HEAD(cmd_path_q, cmd_path);

struct menu {
	TAILQ_ENTRY(menu)	 entry;
	TAILQ_ENTRY(menu)	 resultentry;
#define MENU_MAXENTRY		 200
	char			 text[MENU_MAXENTRY + 1];
	char			 print[MENU_MAXENTRY + 1];
	void			*ctx;
	short			 dummy;
	short			 abort;
};
TAILQ_HEAD(menu_q, menu);

#define CONF_FONT	"sans-serif:pixelsize=14:bold"
#define CONF_MAMOUNT	1
#define CONF_SNAPDIST	0
struct binding_q	 bindingq;
struct keybinding_q	 keybindingq;
struct mousebinding_q	 mousebindingq;
struct autogroupwin_q	 autogroupq;
struct ignore_q		 ignoreq;
struct rule_q		 ruleq;
struct rule_item_q	 ruleitemq;
struct cmd_path_q	 cmdpathq;

bool new_config;

struct config_group {
	int	 bwidth;
	char	*color[CWM_COLOR_NITEMS];
	XftColor xftcolor[CWM_COLOR_NITEMS];
	XftFont	*xftfont;
};

struct config_screen {
	Cursor		 cursor[CF_NITEMS];
	struct gap	 gap;
	int		 snapdist;
	char		*font;
	char		*panel_cmd;
};

struct config_client {
	struct autogroupwin_q	*autogroupq;
	struct ignore_q		*ignoreq;
};

/* MWM hints */
struct mwm_hints {
	unsigned long	flags;
	unsigned long	functions;
	unsigned long	decorations;
};
#define MWM_HINTS_ELEMENTS	3L

#define MWM_FLAGS_FUNCTIONS	(1<<0)
#define MWM_FLAGS_DECORATIONS	(1<<1)
#define MWM_FLAGS_INPUT_MODE	(1<<2)
#define MWM_FLAGS_STATUS	(1<<3)

#define MWM_FUNCS_ALL		(1<<0)
#define MWM_FUNCS_RESIZE	(1<<1)
#define MWM_FUNCS_MOVE		(1<<2)
#define MWM_FUNCS_MINIMIZE	(1<<3)
#define MWM_FUNCS_MAXIMIZE	(1<<4)
#define MWM_FUNCS_CLOSE		(1<<5)

#define	MWM_DECOR_ALL		(1<<0)
#define	MWM_DECOR_BORDER	(1<<1)
#define MWM_DECOR_RESIZE_HANDLE	(1<<2)
#define MWM_DECOR_TITLEBAR	(1<<3)
#define MWM_DECOR_MENU		(1<<4)
#define MWM_DECOR_MINIMIZE	(1<<5)
#define MWM_DECOR_MAXIMIZE	(1<<6)

/* Option table entries. */
enum options_table_type {
	OPTIONS_TABLE_STRING,
	OPTIONS_TABLE_NUMBER,
	OPTIONS_TABLE_KEY,
	OPTIONS_TABLE_COLOUR,
	OPTIONS_TABLE_FLAG,
};
enum options_table_scope {
	OPTIONS_TABLE_NONE,
	OPTIONS_TABLE_SCREEN,
	OPTIONS_TABLE_GROUP,
	OPTIONS_TABLE_CLIENT,
};

struct options_table_entry {
	const char		 *name;
	enum options_table_type	  type;
	enum options_table_scope  scope;

	u_int			  minimum;
	u_int			  maximum;

	const char		 *default_str;
	long long		  default_num;

};

extern Display				*X_Dpy;
extern Time				 Last_Event_Time;
extern struct screen_ctx_q		 Screenq;
extern const char			*homedir;
extern int				 Randr_ev;
char					*conf_path;
char					*cwm_pipe;
char					 known_hosts[PATH_MAX];
char					*conf_file;

struct name_func {
	const char	*tag;
	void		 (*handler)(struct client_ctx *, union arg *);
	int		 flags;
	union arg	 argument;
};

extern const struct name_func		 name_to_func[];

enum {
	WM_STATE,
	WM_DELETE_WINDOW,
	WM_TAKE_FOCUS,
	WM_PROTOCOLS,
	_MOTIF_WM_HINTS,
	UTF8_STRING,
	WM_CHANGE_STATE,
	CWMH_NITEMS
};
enum {
	_NET_SUPPORTED,
	_NET_SUPPORTING_WM_CHECK,
	_NET_ACTIVE_WINDOW,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_CURRENT_DESKTOP,
	_NET_DESKTOP_VIEWPORT,
	_NET_DESKTOP_GEOMETRY,
	_NET_VIRTUAL_ROOTS,
	_NET_SHOWING_DESKTOP,
	_NET_DESKTOP_NAMES,
	_NET_WORKAREA,
	_NET_WM_NAME,
	_NET_WM_DESKTOP,
	_NET_CLOSE_WINDOW,
	_NET_WM_STATE,
#define	_NET_WM_STATES_NITEMS	6
	_NET_WM_STATE_STICKY,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_DEMANDS_ATTENTION,
	EWMH_NITEMS
};
enum {
	_NET_WM_STATE_REMOVE,
	_NET_WM_STATE_ADD,
	_NET_WM_STATE_TOGGLE
};
extern Atom				 cwmh[CWMH_NITEMS];
extern Atom				 ewmh[EWMH_NITEMS];

void			 usage(void);

void			 client_applysizehints(struct client_ctx *);
void			 client_config(struct client_ctx *);
struct client_ctx	*client_current(void);
void			 client_cycle(struct screen_ctx *, int);
void			 client_cycle_leave(struct screen_ctx *);
void			 client_delete(struct client_ctx *);
void			 client_draw_border(struct client_ctx *);
void			 client_data_extend(struct client_ctx *);
void			 client_expand(struct client_ctx *);
struct client_ctx	*client_find(Window);
struct client_ctx	*client_find_win_str(struct screen_ctx *, const char *);
void			 client_record_geom(struct client_ctx *);
void			 client_restore_geom(struct client_ctx *, int);
long			 client_get_wm_state(struct client_ctx *);
void			 client_getsizehints(struct client_ctx *);
void			 client_hide(struct client_ctx *);
void			 client_none(struct screen_ctx *);
void 			 client_htile(struct client_ctx *);
void			 client_lower(struct client_ctx *);
void			 client_map(struct client_ctx *);
void			 client_msg(struct client_ctx *, Atom, Time);
void			 client_move(struct client_ctx *);
struct client_ctx	*client_init(Window, int);
void			 client_ptrsave(struct client_ctx *);
void			 client_ptrwarp(struct client_ctx *);
void			 client_raise(struct client_ctx *);
void			 client_resize(struct client_ctx *, int);
void			 client_scan_for_windows(void);
void			 client_send_delete(struct client_ctx *);
void			 client_set_wm_state(struct client_ctx *, long);
void			 client_setactive(struct client_ctx *);
void			 client_setname(struct client_ctx *);
void			 client_snap(struct client_ctx *, int);
int			 client_snapcalc(int, int, int, int, int);
void			 client_toggle_freeze(struct client_ctx *);
void			 client_toggle_fullscreen(struct client_ctx *);
void			 client_toggle_hidden(struct client_ctx *);
void			 client_toggle_hmaximize(struct client_ctx *);
void			 client_toggle_maximize(struct client_ctx *);
void			 client_toggle_sticky(struct client_ctx *);
void			 client_toggle_vmaximize(struct client_ctx *);
void			 client_transient(struct client_ctx *);
void			 client_unhide(struct client_ctx *);
void			 client_urgency(struct client_ctx *);
void 			 client_vtile(struct client_ctx *);
void			 client_warp(struct client_ctx *);
void			 client_wm_hints(struct client_ctx *);
void			 client_toggle_border(struct client_ctx *);
void			 client_log_debug(const char *, struct client_ctx *);
void			 group_assign(struct group_ctx *, struct client_ctx *);
void			 group_alltoggle(struct screen_ctx *);
void			 group_autogroup(struct client_ctx *);
void			 group_cycle(struct screen_ctx *, int);
void			 group_hide(struct group_ctx *);
void			 group_hidetoggle(struct screen_ctx *, int);
int			 group_holds_only_hidden(struct group_ctx *);
int			 group_holds_only_sticky(struct group_ctx *);
void			 group_init(struct screen_ctx *, int);
void			 group_movetogroup(struct client_ctx *, int);
void			 group_only(struct screen_ctx *, int);
void			 group_show(struct group_ctx *);
void			 group_setactive(struct group_ctx *);
void			 group_toggle_membership_enter(struct client_ctx *);
void			 group_toggle_membership_leave(struct client_ctx *);
void			 group_update_names(struct screen_ctx *);
struct group_ctx	*group_find_by_num(struct screen_ctx *, int);

/* options.c */
struct options	*options_create(struct options *);
void		 options_free(struct options *);
struct options_entry *options_first(struct options *);
struct options_entry *options_next(struct options_entry *);
struct options_entry *options_empty(struct options *,
		     const struct options_table_entry *);
struct options_entry *options_default(struct options *,
		     const struct options_table_entry *);
const char	*options_name(struct options_entry *);
const struct options_table_entry *options_table_entry(struct options_entry *);
struct options_entry *options_get_only(struct options *, const char *);
struct options_entry *options_get(struct options *, const char *);
void		 options_remove(struct options_entry *);
void		 options_array_clear(struct options_entry *);
const char	*options_array_get(struct options_entry *, u_int);
int		 options_array_set(struct options_entry *, u_int, const char *,
		     int);
int		 options_array_size(struct options_entry *, u_int *);
void		 options_array_assign(struct options_entry *, const char *);
int		 options_isstring(struct options_entry *);
const char	*options_tostring(struct options_entry *, int, int);
char		*options_parse(const char *, int *);
struct options_entry *options_parse_get(struct options *, const char *, int *,
		     int);
char		*options_match(const char *, int *, int *);
struct options_entry *options_match_get(struct options *, const char *, int *,
		     int, int *);
const char	*options_get_string(struct options *, const char *);
long long	 options_get_number(struct options *, const char *);
const struct grid_cell *options_get_style(struct options *, const char *);
struct options_entry *options_set_string(struct options *,
		     const char *, int, const char *, ...);
struct options_entry *options_set_number(struct options *, const char *,
		     long long);
struct options_entry *options_set_style(struct options *, const char *, int,
		     const char *);
/*
enum options_table_scope options_scope_from_flags(struct args *, int,
		     struct cmd_find_state *, struct options **, char **);
*/		     
void		 options_style_update_new(struct options *,
		     struct options_entry *);
void		 options_style_update_old(struct options *,
		     struct options_entry *);

/* options-table.c */
extern const struct options_table_entry options_table[];

void			 search_match_client(struct menu_q *, struct menu_q *,
			     char *);
void			 search_match_exec(struct menu_q *, struct menu_q *,
			     char *);
void			 search_match_exec_path(struct menu_q *,
			     struct menu_q *, char *);
void			 search_match_path_any(struct menu_q *, struct menu_q *,
			     char *);
void			 search_match_text(struct menu_q *, struct menu_q *,
			     char *);
void			 search_print_client(struct menu *, int);
void			 search_print_group(struct menu *, int);

struct screen_ctx	*screen_find(Window);
struct geom		 screen_find_xinerama(int, int, int);
struct screen_ctx	*screen_find_screen(int, int, struct screen_ctx *);
void			 screen_maybe_init_randr(void);
void			 screen_update_geometry(struct screen_ctx *);
void			 screen_updatestackingorder(struct screen_ctx *);
struct screen_ctx	*screen_find_by_name(const char *);
void			 screen_apply_ewmh(void);
struct screen_ctx	*screen_current_screen(struct client_ctx **);

void		 	 bindings_init(void);


void			 kbfunc_client_cycle(struct client_ctx *, union arg *);
void			 kbfunc_client_cyclegroup(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_delete(struct client_ctx *, union arg *);
void			 kbfunc_client_group(struct client_ctx *, union arg *);
void			 kbfunc_client_grouponly(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_grouptoggle(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_hide(struct client_ctx *, union arg *);
void			 kbfunc_client_label(struct client_ctx *, union arg *);
void			 kbfunc_client_lower(struct client_ctx *, union arg *);
void			 kbfunc_client_moveresize(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_movetogroup(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_nogroup(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_raise(struct client_ctx *, union arg *);
void			 kbfunc_client_rcycle(struct client_ctx *, union arg *);
void			 kbfunc_client_search(struct client_ctx *, union arg *);
void			 kbfunc_client_snap(struct client_ctx *, union arg *);
void			 kbfunc_client_toggle_border(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_toggle_freeze(struct client_ctx *,
    			     union arg *);
void			 kbfunc_client_toggle_fullscreen(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_toggle_hmaximize(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_toggle_maximize(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_toggle_sticky(struct client_ctx *,
    			     union arg *);
void			 kbfunc_client_toggle_vmaximize(struct client_ctx *,
			     union arg *);
void			 kbfunc_client_expand(struct client_ctx *, union arg *);
void			 kbfunc_cmdexec(struct client_ctx *, union arg *);
void			 kbfunc_cwm_status(struct client_ctx *, union arg *);
void			 kbfunc_exec(struct client_ctx *, union arg *);
void			 kbfunc_lock(struct client_ctx *, union arg *);
void			 kbfunc_menu_cmd(struct client_ctx *, union arg *);
void			 kbfunc_menu_group(struct client_ctx *, union arg *);
void			 kbfunc_ssh(struct client_ctx *, union arg *);
void			 kbfunc_term(struct client_ctx *, union arg *);
void 			 kbfunc_tile(struct client_ctx *, union arg *);

/* log.c */
void			 log_open(const char *);
void			 log_close(void);
void			 log_debug(const char *, ...);
__dead void		 log_fatal(const char *, ...);
__dead void		 log_fatalx(const char *, ...);

void			 mousefunc_client_move(struct client_ctx *,
    			    union arg *);
void			 mousefunc_client_resize(struct client_ctx *,
    			    union arg *);
void			 mousefunc_menu_cmd(struct client_ctx *, union arg *);
void			 mousefunc_menu_group(struct client_ctx *, union arg *);
void			 mousefunc_menu_unhide(struct client_ctx *,
    			    union arg *);

struct menu  		*menu_filter(struct screen_ctx *, struct menu_q *,
			     const char *, const char *, int,
			     void (*)(struct menu_q *, struct menu_q *, char *),
			     void (*)(struct menu *, int));
void			 menuq_add(struct menu_q *, void *, const char *, ...);
void			 menuq_clear(struct menu_q *);

void			 conf_atoms(void);
void			 conf_autogroup(int, const char *, const char *);
int			 conf_bind_kbd(const char *, const char *);
int			 conf_bind_mouse(const char *, const char *);
void			 conf_clear(void);
void			 conf_client(struct client_ctx *);
int			 conf_cmd_add(const char *, const char *);
void			 conf_cursor(struct screen_ctx *);
void			 conf_grab_kbd(Window);
void			 conf_grab_mouse(Window);
void			 conf_init(void);
void			 conf_ignore(const char *);
void			 conf_screen(struct screen_ctx *, struct group_ctx *);

void			 config_parse(void);

void			 config2(void);

void			 xev_process(void);

/* rules.c */
void			 rule_config(const char *, const char *, const char *);
void			 rule_apply(struct client_ctx *, const char *);
const char		*rule_print_rule(struct client_ctx *);
bool			 rule_validate_title(const char *);

void			 xu_btn_grab(Window, int, unsigned int);
void			 xu_btn_ungrab(Window);
int			 xu_getprop(Window, Atom, Atom, long, unsigned char **);
int			 xu_getstrprop(Window, Atom, char **);
void			 xu_key_grab(Window, unsigned int, KeySym);
void			 xu_key_ungrab(Window);
void			 xu_ptr_getpos(Window, int *, int *);
int			 xu_ptr_grab(Window, unsigned int, Cursor);
int			 xu_ptr_regrab(unsigned int, Cursor);
void			 xu_ptr_setpos(Window, int, int);
void			 xu_ptr_ungrab(void);
void			 xu_xft_draw(struct screen_ctx *, const char *,
			     int, int, int);
int			 xu_xft_width(XftFont *, const char *, int);
void 			 xu_xorcolor(XftColor, XftColor, XftColor *);

void			 xu_ewmh_net_supported(struct screen_ctx *);
void			 xu_ewmh_net_supported_wm_check(struct screen_ctx *);
void			 xu_ewmh_net_desktop_geometry(struct screen_ctx *);
void			 xu_ewmh_net_workarea(struct screen_ctx *);
void			 xu_ewmh_net_client_list(struct screen_ctx *);
void			 xu_ewmh_net_client_list_stacking(struct screen_ctx *);
void			 xu_ewmh_net_active_window(struct screen_ctx *, Window);
void			 xu_ewmh_net_wm_desktop_viewport(struct screen_ctx *);
void			 xu_ewmh_net_wm_number_of_desktops(struct screen_ctx *);
void			 xu_ewmh_net_showing_desktop(struct screen_ctx *);
void			 xu_ewmh_net_virtual_roots(struct screen_ctx *);
void			 xu_ewmh_net_current_desktop(struct screen_ctx *);
void			 xu_ewmh_net_desktop_names(struct screen_ctx *);

void			 xu_ewmh_net_wm_desktop(struct client_ctx *);
Atom 			*xu_ewmh_get_net_wm_state(struct client_ctx *, int *);
void 			 xu_ewmh_handle_net_wm_state_msg(struct client_ctx *,
			     int, Atom , Atom);
void 			 xu_ewmh_set_net_wm_state(struct client_ctx *);
void 			 xu_ewmh_restore_net_wm_state(struct client_ctx *);

void			 u_exec(char *);
void			 u_spawn(char *);
void			 u_init_pipe(void);
void			 u_put_status(void);

struct args 		*args_parse(const char *, int, char **);
void             	 args_free(struct args *);
size_t           	 args_print(struct args *, char *, size_t);
int             	 args_has(struct args *, u_char);
const char      	*args_get(struct args *, u_char);
long long        	 args_strtonum(struct args *, u_char, long long,
			    long long, char **);

/* cmd.c */
const struct cmd_entry	*cmd_find_cmd(const char *);
struct cmd_find		*cmd_find_target(struct cmd_q *, struct args *);
void			 cmd_find_print(struct cmd_find *);
char			**cmd_copy_argv(int, char *const *);
void			 cmd_free_argv(int, char **);
struct cmd		*cmd_parse(int, char **, const char *, u_int, char **);
struct cmd_list		*cmd_string_parse(const char *, const char *, u_int,
			    char **);
int			 cmd_string_split(const char *, int *, char ***);

/* cmd-list.c */
struct cmd_list		*cmd_list_parse(int, char **, const char *, u_int,
			    char **);
void		 	 cmd_list_free(struct cmd_list *);

/* cmd-queue.c */
struct cmd_q		*cmdq_new(void);
int			 cmdq_free(struct cmd_q *);
void printflike2	 cmdq_error(struct cmd_q *, const char *, ...);
void			 cmdq_run(struct cmd_q *, struct cmd_list *);
void			 cmdq_append(struct cmd_q *, struct cmd_list *);
int			 cmdq_continue(struct cmd_q *);
void			 cmdq_flush(struct cmd_q *);

void			*xcalloc(size_t, size_t);
void			*xmalloc(size_t);
void			*xrealloc(void *, size_t);
void			*xreallocarray(void *, size_t, size_t);
char			*xstrdup(const char *);
int			 xasprintf(char **, const char *, ...)
			    __attribute__((__format__ (printf, 2, 3)))
			    __attribute__((__nonnull__ (2)));
int			 xsnprintf(char *, size_t, const char *, ...);
int			 xvsnprintf(char *, size_t, const char *, va_list);
int			 xvasprintf(char **, const char *, va_list);
#endif /* _CALMWM_H_ */
