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
#include <errno.h>

#include "calmwm.h"

static void	 config_apply(void);
static void	 config_default(cfg_t *, bool);
static void	 config_internalise(cfg_t *);
static void	 config_internalise_groups(struct screen_ctx *, cfg_t *);
static void	 config_intern_clients(cfg_t *);
static void	 config_intern_group(struct config_group *, cfg_t *);
static void	 config_intern_screen(struct config_screen *, cfg_t *);
static void	 config_intern_menu(cfg_t *);

cfg_opt_t	 color_opts[] = {
	CFG_STR("activeborder", "#CCCCCC", CFGF_NONE),
	CFG_STR("inactiveborder", "#666666", CFGF_NONE),
	CFG_STR("groupborder", "blue", CFGF_NONE),
	CFG_STR("ungroupborder", "red", CFGF_NONE),
	CFG_STR("urgencyborder", "red", CFGF_NONE),
	CFG_STR("font", "white", CFGF_NONE),
	CFG_STR("fontsel", "", CFGF_NONE),
	CFG_STR("menufg", "white", CFGF_NONE),
	CFG_STR("menubg", "#66BA66", CFGF_NONE),
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

cfg_opt_t	 menu_item_opts[] = {
	CFG_STR("command", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 menu_opts[] = {
	CFG_SEC("item", menu_item_opts,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

cfg_opt_t	 rule_item_types[] = {
	CFG_STR_LIST("command", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 rules_item_opts[] = {
	CFG_SEC("rule", rule_item_types,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};


cfg_opt_t	 client_item_opts[] = {
	CFG_BOOL("ignore", cfg_false, CFGF_NONE),
	CFG_STR("autogroup", NULL, CFGF_NONE),
	CFG_SEC("rules", rules_item_opts, CFGF_MULTI),
	CFG_END()
};

cfg_opt_t	 client_opts[] = {
	CFG_SEC("client", client_item_opts,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

cfg_opt_t	 screen_opts[] = {
	CFG_SEC("groups", groups_opts, CFGF_NONE),
	CFG_INT_LIST("gap", "{0,0,0,0}", CFGF_NONE),
	CFG_INT("snapdist", 0, CFGF_NONE),
	CFG_STR("font", CONF_FONT, CFGF_NONE),
	CFG_STR("panel-cmd", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t	 all_cfg_opts[] = {
	CFG_SEC("clients", client_opts, CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_SEC("menu", menu_opts, CFGF_MULTI),
	CFG_SEC("screen", screen_opts,
		CFGF_TITLE | CFGF_NO_TITLE_DUPES | CFGF_MULTI),
	CFG_END()
};

#define DEFAULT_CONFIG_SCR(s) \
	({ char str[8192]; \
	snprintf(str, sizeof(str), \
	"menu {" \
		"item term {" \
			"command = \"xterm\"" \
		"}" \
		"item lock {" \
			"command = \"lock\"" \
		"}" \
	"}" \
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

	sc = TAILQ_FIRST(&Screenq);

	TAILQ_FOREACH(sc, &Screenq, entry) {
		screen_update_geometry(sc);

		TAILQ_FOREACH(gc, &sc->groupq, entry) {
			conf_screen(sc, gc);
		}
	}
}

static void
config_intern_menu(cfg_t *cfg)
{
	cfg_t		*menu_sec, *item_sec;
	const char	*entry, *cmd;
	size_t		 i, j;

	for (i = 0; i < cfg_size(cfg, "menu"); i++) {
		if ((menu_sec = cfg_getnsec(cfg, "menu", i)) == NULL)
			continue;

		for (j = 0; j < cfg_size(menu_sec, "item"); j++) {
			item_sec = cfg_getnsec(menu_sec, "item", j);
			entry = cfg_title(item_sec);
			cmd = cfg_getstr(item_sec, "command");
			if (cmd == NULL)
				continue;
			conf_cmd_add(entry, cmd);
		}
	}
}

static void
config_default(cfg_t *cfg, bool include_default_config)
{
	struct screen_ctx	*sc;

	if (!include_default_config) {
		/* This is a user-config which is always applied last, so we
		 * can then also process other things after it, once the
		 * config has been read.
		 */
		config_intern_clients(cfg);
		config_internalise(cfg);
		config_intern_menu(cfg);

		return;
	}

	TAILQ_FOREACH(sc, &Screenq, entry) {
		cfg_parse_buf(cfg, DEFAULT_CONFIG_SCR(sc->name));
		if (cfg == NULL)
			log_fatal("Unable to load DEFAULT_CONFIG");

		config_intern_clients(cfg);
		config_internalise(cfg);
		config_intern_menu(cfg);
	}
}

static void
config_intern_clients(cfg_t *cfg)
{
	cfg_t		*clients_sec, *c_sec, *r_sec, *rule_sec;
	const char	*client_title, *errstr, *rule_title;
	char		*client_res[2], *tmp, *ctitle, *class_or_res;
	char		*grp;
	int		 t_grp;
	size_t		 i, j, r, rs;

	for (i = 0; i < cfg_size(cfg, "clients"); i++) {
		clients_sec = cfg_getnsec(cfg, "clients", i);

		for (j = 0; j < cfg_size(clients_sec, "client"); j++) {
			c_sec = cfg_getnsec(clients_sec, "client", j);

			client_title = cfg_title(c_sec);
			ctitle = (char *)client_title;

			if (strchr(client_title, ',') != NULL) {
				/* This contains both the class and the
				 * resource.
				 */
				tmp = strtok(ctitle, ",");
				client_res[0] = tmp;

				tmp = strtok(NULL, ",");
				client_res[1] = tmp;
			} else {
				/* Just the class. */
				client_res[0] = NULL;
				client_res[1] = ctitle;
			}

			if (cfg_getstr(c_sec, "autogroup") != NULL) {
				grp = cfg_getstr(c_sec, "autogroup");

				t_grp = strtonum(grp, 0, 9, &errstr);
				if (errstr != NULL) {
					log_debug("Group '%s' not valid; %s",
					    grp, errstr);
				}
				conf_autogroup(t_grp, client_res[0],
				    client_res[1]);
			}

			if (cfg_getbool(c_sec, "ignore"))
				conf_ignore(client_res[1]);

			/* Process any rules here. */
			r_sec = cfg_getsec(c_sec, "rules");
			class_or_res = client_res[0] != NULL ?
				client_res[0] : client_res[1];

			for (r = 0; r < cfg_size(r_sec, "rule"); r++) {
				rule_sec = cfg_getnsec(r_sec, "rule", r);
				rule_title = cfg_title(rule_sec);

				if (!rule_validate_title(rule_title)) {
					log_debug("%s: rule '%s' has invalid "
					    "title", __func__, rule_title);
					continue;
				}

				for (rs = 0; rs < cfg_size(rule_sec, "command");
				    rs++) {
					rule_config(class_or_res, rule_title,
					    cfg_getnstr(rule_sec, "command",
					    rs));
				}
			}
		}
	}
}

static void
config_internalise_groups(struct screen_ctx *sc, cfg_t *sc_sec)
{
	cfg_t			*groups_sec, *gc_sec;
	struct group_ctx	*gc;
	const char		*gc_sec_title, *errstr;
	char			*cpy, *t_cpy, *item;
	size_t			 j;
	int			 t_grp;

	groups_sec = cfg_getsec(sc_sec, "groups");
	for (j = 0; j < cfg_size(groups_sec, "group"); j++) {
		gc_sec = cfg_getnsec(groups_sec, "group", j);
		gc_sec_title = cfg_title(gc_sec);

		if (strcmp(gc_sec_title, "*") == 0) {
			TAILQ_FOREACH(gc, &sc->groupq, entry) {
				log_debug("Applying config to all groups "
						"(screen: %s, group: %d",
						sc->name, gc->num);

				config_intern_group(gc->config_group, gc_sec);
			}
		} else {
			/* Go through a possible comma-separated list
			 * of items, and apply those to the groups in
			 * question.
			 */
			cpy = t_cpy = xstrdup(gc_sec_title);
			while ((item = strsep(&t_cpy, ",")) != NULL) {
				if (*item == '\0')
					continue;

				t_grp = strtonum(item, 0, 9, &errstr);
				if (errstr != NULL) {
					log_debug("Group '%s' "
					    "not valid; %s", item, errstr);
					continue;
				}
				gc = group_find_by_num(sc, t_grp);
				config_intern_group(gc->config_group, gc_sec);
			}
			free(cpy);
		}
	}
}

static void
config_internalise(cfg_t *cfg)
{
	cfg_t			*sc_sec;
	struct screen_ctx	*sc;
	size_t			 i;
	const char		*sc_sec_title;
	char			*t_cpy, *cpy, *item;

	for (i = 0; i < cfg_size(cfg, "screen"); i++) {
		sc_sec = cfg_getnsec(cfg, "screen", i);
		sc_sec_title = cfg_title(sc_sec);

		if (sc_sec_title != NULL) {
			if (strcmp(sc_sec_title, "*") == 0) {
				/* Applies to all screens. */
				TAILQ_FOREACH(sc, &Screenq, entry) {
					config_intern_screen(sc->config_screen,
					    sc_sec);
					config_internalise_groups(sc, sc_sec);
				}
			} else 	{
				cpy = t_cpy = xstrdup(sc_sec_title);
				while ((item = strsep(&t_cpy, ",")) != NULL) {
					if (*item == '\0')
						continue;
					sc = screen_find_by_name(item);
					if (sc == NULL || strcmp(sc->name, item)) {
						log_debug("Screen section '%s' "
						   "found without a "
						   "corresponding screen to "
						   "apply it to", item);
						continue;
					}
					config_intern_screen(sc->config_screen,
					    sc_sec);
					config_internalise_groups(sc, sc_sec);
				}
				free(cpy);
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
	cs->font = cfg_getstr(cfg, "font");
	cs->panel_cmd = cfg_getstr(cfg, "panel-cmd");
}

void
config_parse(void)
{
	cfg_t	*cfg_default, *cfg;

	TAILQ_INIT(&ignoreq);
	TAILQ_INIT(&autogroupq);
	TAILQ_INIT(&ruleq);
	TAILQ_INIT(&cmdpathq);

	(void)snprintf(known_hosts, sizeof(known_hosts), "%s/%s",
	    homedir, ".ssh/known_hosts");

	if ((cfg_default = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init  config options");

	config_default(cfg_default, true);
#if 0
	if (conf_path == NULL) {
		log_debug("No user-supplied config file present.");
		goto apply;
	}

	if ((cfg = cfg_init(all_cfg_opts, CFGF_NONE)) == NULL)
		log_fatal("Couldn't init config options");
	if (cfg_parse(cfg, conf_path) == CFG_PARSE_ERROR) {
		log_debug("Couldn't parse '%s': %s", conf_path, strerror(errno));
	}

	if (cfg_size(cfg, "screen") > 0)
		config_default(cfg, false);
#endif

apply:
	config_apply();

#if 0
	/* FIXME: This is causing segfaults. */
	cfg_free(cfg_default);
	cfg_free(cfg);
#endif
}
