/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2017 Thomas Adam <thomas@xteddy.org>
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
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "calmwm.h"

static struct binding	*rule_make_binding(const char *);

static struct binding *
rule_make_binding(const char *action)
{
	struct binding		*b = NULL;
	const struct name_func	*nf;

	for (nf = name_to_func; nf->tag != NULL; nf++) {
		if (strcmp(nf->tag, action) != 0)
			continue;

		b = xcalloc(1, sizeof(*b));
		b->callback = nf->handler;
		b->flags = nf->flags;
		b->argument = nf->argument;
	}

	return (b);
}

bool
rule_validate_title(const char *rule_title)
{
	const char	*rule_names[] = {
		"on-map",
		"on-focus",
		"on-close",
		"on-net-active",
		NULL,
	};

	const char	**rt;

	for (rt = rule_names; *rt != NULL; rt++) {
		if (strcmp(*rt, rule_title) == 0)
			return (true);
	}

	return (false);
}

void
rule_config(const char *class, const char *rname, const char *action)
{
	struct rule		*rule = NULL, *r_find = NULL;
	struct rule_item	*ritem;

	if (action == NULL)
		return;

	if (!TAILQ_EMPTY(&ruleq)) {
		TAILQ_FOREACH(r_find, &ruleq, entry) {
			if (strcmp(r_find->rule_name, rname) == 0 &&
			    strcmp(r_find->client_class, class) == 0) {
				rule = r_find;
				break;
			}
		}

		if (rule == NULL) {
			log_debug("%s: {r: %s, c: %s, a: %s} is new",
			    __func__, rname, class, action);

			goto make_rule;
		}

		ritem = xcalloc(1, sizeof(*ritem));
		ritem->name = xstrdup(action);
		if ((ritem->b = rule_make_binding(action)) == NULL) {
			log_debug("%s: action '%s' doesn't exist",
					__func__, action);

			free((char *)ritem->name);
			free(ritem);
			return;
		}
		log_debug("%s: adding rule: {r: %s, c: %s, a: %s}",
				__func__, rname, class, action);
		rule->ri_size++;
		TAILQ_INSERT_TAIL(&rule->rule_item, ritem, entry);

		return;
	}

make_rule:
	rule = xmalloc(sizeof(*rule));
	TAILQ_INIT(&rule->rule_item);

	rule->rule_name = xstrdup(rname);
	rule->client_class = xstrdup(class);
	rule->ri_size = 0;

	ritem = xcalloc(1, sizeof(*ritem));
	ritem->name = xstrdup(action);
	if ((ritem->b = rule_make_binding(action)) == NULL) {
		log_debug("%s: action '%s' doesn't exist", __func__,
				action);

		free((char *)ritem->name);
		free(ritem);

		return;
	}
	TAILQ_INSERT_TAIL(&ruleq, rule, entry);
	log_debug("%s: adding rule: {r: %s, c: %s, a: %s}",
			__func__, rname, class, action);

	rule->ri_size++;
	TAILQ_INSERT_TAIL(&rule->rule_item, ritem, entry);
}

void
rule_apply(struct client_ctx *cc, const char *rule_name)
{
	char			*class = cc->ch.res_class;
	struct rule		*rule;
	struct rule_item	*rule_i;

	if (class == NULL)
		return;

	TAILQ_FOREACH(rule, &ruleq, entry) {
		if (strcmp(rule->rule_name, rule_name) != 0)
			continue;
		if (strcmp(class, rule->client_class) != 0)
			continue;

		log_debug("%s: for client '%s', rule '%s' applies",
		    __func__, class, rule->rule_name);

		rule->cc = cc;

		/* Otherwise, we found the rule we need. */
		TAILQ_FOREACH(rule_i, &rule->rule_item, entry) {
			/* Go through each binding and apply the rules in
			 * order.
			 */
			log_debug("%s:\tapplying '%s' cmd", __func__,
			     rule_i->name);

			(*rule_i->b->callback)(cc, &rule_i->b->argument);
		}
	}
}

const char *
rule_print_rule(struct client_ctx *cc)
{
	struct rule		*r_find = NULL, *rule = NULL;
	struct rule_item	*rule_i;
	char			*rule_str;
	char			*ra, *rule_actions;

	TAILQ_FOREACH(r_find, &ruleq, entry) {
		if (cc == r_find->cc) {
			rule = r_find;
			break;
		}
	}

	if (rule == NULL) {
		xasprintf(&rule_str, "%s", "none"); 
		return (rule_str);
	}

	ra = rule_actions = malloc(rule->ri_size + 1);
	TAILQ_FOREACH(rule_i, &rule->rule_item, entry) {
		ra += sprintf(ra, "%s,", rule_i->name);
	}
	ra[-1] = '\0';

	xasprintf(&rule_str, "[pointer: %p]:\n\t"
	    "\tname: %s\n\t"
	    "\tactions: {%s}",
	    rule, rule->rule_name, rule_actions);

	return (rule_str);
}
