/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Marius Aamodt Eriksen <marius@monkey.org>
 * Copyright (c) 2015 Thomas Adam <thomas@xteddy.org>
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

#include <stdio.h>
#include <string.h>

#include "calmwm.h"

static void	 config_apply(void);
static void	 config_default(cfg_t *, bool);
static void	 config_internalise(struct screen_ctx *, cfg_t *);
static void	 config_intern_group(struct config_group *, cfg_t *);
static void	 config_intern_screen(struct config_screen *, cfg_t *);
static void	 config_intern_bindings(cfg_t *);

cfg_opt_t	 color_opts[] = {
	CFG_STR("activeborder", "#CCCCCC", CFGF_NONE),
	CFG_STR("inactiveborder", "#666666", CFGF_NONE),
	CFG_STR("groupborder", "blue", CFGF_NONE),
	CFG_STR("ungroupborder", "red", CFGF_NONE),
	CFG_STR("urgencyborder", "#FC8814", CFGF_NONE),
	CFG_STR("font", "pink", CFGF_NONE),
	CFG_STR("fontsel", "", CFGF_NONE),
	CFG_STR("menufg", "yellow", CFGF_NONE),
	CFG_STR("menubg", "green", CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 group_opts[] = {
	CFG_INT("borderwidth", 4, CFGF_NONE),
	CFG_SEC("color", color_opts, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 groups_opts[] = {
	CFG_SEC("group", group_opts,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

cfg_opt_t	 key_mouse_opts[] = {
	CFG_STR("command", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 bind_opts[] = {
	CFG_SEC("key", key_mouse_opts, 
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_SEC("mouse", key_mouse_opts, 
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

#define DEFAULT_BINDINGS				\
	"key 4S-Down	{command = snapdown}"		\
	"key 4S-Left	{command = snapleft}"		\
	"key 4S-Right	{command = snapright}"		\
	"key 4S-Up	{command = snapup}"		\
	"key C-Down	{command = ptrmovedown}"	\
	"key C-Left	{command = ptrmoveleft}"	\
	"key C-Right	{command = ptrmoveright}"	\
	"key C-Up	{command = ptrmoveup}"		\
	"key C-slash	{command = menusearch}"		\
	"key CM-0	{command = group0}"		\
	"key CM-1	{command = group1}"		\
	"key CM-2	{command = group2}"		\
	"key CM-3	{command = group3}"		\
	"key CM-4	{command = group4}"		\
	"key CM-5	{command = group5}"		\
	"key CM-6	{command = group6}"		\
	"key CM-7	{command = group7}"		\
	"key CM-8	{command = group8}"		\
	"key CM-9	{command = group9}"		\
	"key CM-B	{command = toggle_border}"	\
	"key CM-Delete	{command = lock}"		\
	"key CM-H	{command = bigresizeleft}"	\
	"key CM-J	{command = bigresizedown}"	\
	"key CM-K	{command = bigresizeup}"	\
	"key CM-L	{command = bigresizeright}"	\
	"key CM-Return	{command = terminal}"		\
	"key CM-a	{command = nogroup}"		\
	"key CM-equal	{command = vmaximize}"		\
	"key CM-f	{command = fullscreen}"		\
	"key CM-g	{command = grouptoggle}"	\
	"key CM-h	{command = resizeleft}"		\
	"key CM-j	{command = resizedown}"		\
	"key CM-k	{command = resizeup}"		\
	"key CM-l	{command = resizeright}"	\
	"key CM-m	{command = maximize}"		\
	"key CM-n	{command = label}"		\
	"key CM-s	{command = sticky}"		\
	"key CM-x	{command = delete}"		\
	"key CMS-equal	{command = hmaximize}"		\
	"key CMS-f	{command = freeze}"		\
	"key CMS-q	{command = quit}"		\
	"key CMS-r	{command = restart}"		\
	"key CS-Down	{command = bigptrmovedown}"	\
	"key CS-Left	{command = bigptrmoveleft}"	\
	"key CS-Right	{command = bigptrmoveright}"	\
	"key CS-Up	{command = bigptrmoveup}"	\
	"key M-Down	{command = lower}"		\
	"key M-H	{command = bigmoveleft}"	\
	"key M-J	{command = bigmovedown}"	\
	"key M-K	{command = bigmoveup}"		\
	"key M-L	{command = bigmoveright}"	\
	"key M-Left	{command = rcyclegroup}"	\
	"key M-Return	{command = hide}"		\
	"key M-Right	{command = cyclegroup}"		\
	"key M-Tab	{command = cycle}"		\
	"key M-Up	{command = raise}"		\
	"key M-h	{command = moveleft}"		\
	"key M-j	{command = movedown}"		\
	"key M-k	{command = moveup}"		\
	"key M-l	{command = moveright}"		\
	"key M-period	{command = ssh}"		\
	"key M-question	{command = exec}"		\
	"key M-slash	{command = search}"		\
	"key MS-Tab	{command = rcycle}"		\
	"mouse 1	{command = menu_unhide}"	\
	"mouse 2	{command = menu_group}"		\
	"mouse 3	{command = menu_cmd}"		\
	"mouse M-1	{command = window_move}"	\
	"mouse CM-1	{command = window_grouptoggle}"	\
	"mouse M-2	{command = window_resize}"	\
	"mouse M-3	{command = window_lower}"	\
	"mouse CMS-3	{command = window_hide}"

cfg_opt_t	 screen_opts[] = {
	CFG_SEC("groups", groups_opts, CFGF_NONE),
	CFG_INT_LIST("gap", "{0,0,0,0}", CFGF_NONE),
	CFG_INT("snapdist", 0, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 all_cfg_opts[] = {
	CFG_SEC("bindings", bind_opts, CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_SEC("screen", screen_opts,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

#define DEFAULT_CONFIG_BINDINGS \
	({ char str[8192]; \
	snprintf(str, sizeof(str), \
	"bindings {" \
		"%s" \
	"}", (DEFAULT_BINDINGS)); str; })

#define DEFAULT_CONFIG_SCR(s) \
	({ char str[8192]; \
	snprintf(str, sizeof(str), \
	"screen %s {" \
		"groups {" \
			"group 0 {}" \
			"group 1 {}" \
			"group 2 {}" \
			"group 3 {}" \
			"group 4 {}" \
			"group 5 {}" \
			"group 6 {}" \
			"group 7 {}" \
			"group 8 {}" \
			"group 9 {}" \
		"}" \
	"}", (s)); str; })

static void
config_apply(void)
{
	struct screen_ctx	*sc;
	struct group_ctx	*gc;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (screen_should_ignore_global(sc))
			continue;
		screen_update_geometry(sc);

		TAILQ_FOREACH(gc, &sc->groupq, entry) {
			conf_screen(sc, gc);
		}
	}
}

static void
config_intern_bindings(cfg_t *cfg)
{
	cfg_t			*bind_sec, *key_mouse_sec;
	const char		*key, *cmd;
	size_t			 i, j;

	/* Parse mouse/key bindings. */
	for (i = 0; i < cfg_size(cfg, "bindings"); i++) {
		bind_sec = cfg_getnsec(cfg, "bindings", i);

		for (j = 0; j < cfg_size(bind_sec, "key"); j++) {
			key_mouse_sec = cfg_getnsec(bind_sec, "key", j);
			key = cfg_title(key_mouse_sec);
			cmd = cfg_getstr(key_mouse_sec, "command");
			conf_bind_kbd(key, cmd);
		}

		for (j = 0; j < cfg_size(bind_sec, "mouse"); j++) {
			key_mouse_sec = cfg_getnsec(bind_sec, "mouse", j);
			key = cfg_title(key_mouse_sec);
			cmd = cfg_getstr(key_mouse_sec, "command");
			conf_bind_mouse(key, cmd);
		}

	}
}

static void
config_default(cfg_t *cfg, bool include_default_config)
{
	struct screen_ctx	*sc;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (screen_should_ignore_global(sc))
			continue;

		if (include_default_config) {
			cfg_parse_buf(cfg, DEFAULT_CONFIG_SCR(sc->name));
			if (cfg == NULL)
				log_fatal("Unable to load DEFAULT_CONFIG");
		}

		config_internalise(sc, cfg);
	}
}

static void
config_internalise(struct screen_ctx *sc, cfg_t *cfg)
{
	cfg_t			*sc_sec, *groups_sec, *gc_sec;
	struct group_ctx	*gc;
	size_t			 i, j;
	const char		*sc_sec_title, *gc_sec_title;

	for (i = 0; i < cfg_size(cfg, "screen"); i++) {
		sc_sec = cfg_getnsec(cfg, "screen", i);
		sc_sec_title = cfg_title(sc_sec);

		if (sc_sec_title != NULL && strcmp(sc->name, sc_sec_title)) {
			log_debug("Screen section '%s' found without a "
				"corresponding screen to apply it to");
			continue;
		}
		config_intern_screen(sc->config_screen, sc_sec);

		groups_sec = cfg_getsec(sc_sec, "groups");
		for (j = 0; j < cfg_size(groups_sec, "group"); j++) {
			gc_sec = cfg_getnsec(groups_sec, "group", j);
			gc_sec_title = cfg_title(gc_sec);

			TAILQ_FOREACH(gc, &sc->groupq, entry) {
				/*
				 * When dealing with group options, get the
				 * title for the group and look that up
				 * specifically for the
				 * options being applied.
				 */
				if (gc->num != atoi(gc_sec_title))
					continue;

				config_intern_group(gc->config_group, gc_sec);
			}
		}
	}
}

static void
config_intern_group(struct config_group *cg, cfg_t *cfg)
{
	cfg_t		*colour_sec;
	char		*colour;
	size_t		 i;
	const struct {
		const char	*name;
		int		 type;
	} colour_lookup[] = {
		{ "activeborder",	CWM_COLOR_BORDER_ACTIVE },
		{ "inactiveborder",	CWM_COLOR_BORDER_INACTIVE },
		{ "urgencyborder",	CWM_COLOR_BORDER_URGENCY },
		{ "groupborder",	CWM_COLOR_BORDER_GROUP },
		{ "ungroupborder",	CWM_COLOR_BORDER_UNGROUP },
		{ "menufg",		CWM_COLOR_MENU_FG },
		{ "menubg",		CWM_COLOR_MENU_BG },
		{ "font",		CWM_COLOR_MENU_FONT },
		{ "fontsel",		CWM_COLOR_MENU_FONT_SEL },
	};

	/* XXX - validation: < 0 > UINT_MAX */
	cg->bwidth = cfg_getint(cfg, "borderwidth");

	colour_sec = cfg_getsec(cfg, "color");
	for (i = 0; i < nitems(colour_lookup); i++) {
		colour = cfg_getstr(colour_sec, colour_lookup[i].name);
		free(cg->color[colour_lookup[i].type]);
		cg->color[colour_lookup[i].type] = xstrdup(colour);
	}
}

static void
config_intern_screen(struct config_screen *cs, cfg_t *cfg)
{
	/* XXX - validation: < 0 > INT_MAX == bad */
	cs->gap.top = cfg_getnint(cfg, "gap", 0);
	cs->gap.bottom = cfg_getnint(cfg, "gap", 1);
	cs->gap.left = cfg_getnint(cfg, "gap", 2);
	cs->gap.right = cfg_getnint(cfg, "gap", 3);

	/* XXX - validation: < 0 > INT_MAX == bad */
	cs->snapdist = cfg_getint(cfg, "snapdist");
}

void
config_bindings(void)
{
	cfg_t	*cfg_default, *cfg;

	TAILQ_INIT(&keybindingq);
	TAILQ_INIT(&mousebindingq);

	if ((cfg_default = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init config options");

	cfg_parse_buf(cfg_default, DEFAULT_CONFIG_BINDINGS);
	if (cfg_default == NULL)
		log_fatal("Couldn't parse default key bindings...");

	config_intern_bindings(cfg_default);

	if (conf_path == NULL)
		return;

	if ((cfg = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init config options");

	if (cfg_size(cfg, "bindings") > 0)
		config_intern_bindings(cfg);
}

void
config_parse(void)
{
	cfg_t	*cfg_default, *cfg;

	TAILQ_INIT(&autogroupq);
	TAILQ_INIT(&ignoreq);
	TAILQ_INIT(&cmdq);

	/* FIXME: known_hosts for SSH menu. */

	if ((cfg_default = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init  config options");

	config_default(cfg_default, true);

	if (conf_path == NULL) {
		log_debug("No user-supplied config file present.");
		goto apply;
	}

	if ((cfg = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init config options");
	if (cfg_parse(cfg, conf_path) == CFG_PARSE_ERROR) {
		log_debug("Couldn't parse '%s'", conf_path);
	}

	if (cfg_size(cfg, "screen") > 0)
		config_default(cfg, false);

apply:
	config_apply();
}
