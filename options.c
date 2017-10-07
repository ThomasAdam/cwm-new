/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicholas.marriott@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "calmwm.h"

/*
 * Option handling; each option has a name, type and value and is stored in
 * a red-black tree.
 */

struct options_entry {
	struct options				 *owner;

	const char				 *name;
	const struct options_table_entry	 *tableentry;

	union {
		char				 *string;
		long long			  number;
	};

	RB_ENTRY(options_entry)			  entry;
};

struct options {
	RB_HEAD(options_tree, options_entry)	 tree;
	struct options				*parent;
};

static struct options_entry	*options_add(struct options *, const char *);

#define OPTIONS_IS_STRING(o)						\
	((o)->tableentry == NULL ||					\
	    (o)->tableentry->type == OPTIONS_TABLE_STRING)
#define OPTIONS_IS_NUMBER(o) \
	((o)->tableentry != NULL &&					\
	    ((o)->tableentry->type == OPTIONS_TABLE_NUMBER ||		\
	    (o)->tableentry->type == OPTIONS_TABLE_KEY ||		\
	    (o)->tableentry->type == OPTIONS_TABLE_COLOUR ||		\
	    (o)->tableentry->type == OPTIONS_TABLE_FLAG))

static int	options_cmp(struct options_entry *, struct options_entry *);
RB_GENERATE_STATIC(options_tree, options_entry, entry, options_cmp);

static int
options_cmp(struct options_entry *lhs, struct options_entry *rhs)
{
	return (strcmp(lhs->name, rhs->name));
}

static const struct options_table_entry *
options_parent_table_entry(struct options *oo, const char *s)
{
	struct options_entry	*o;

	if (oo->parent == NULL)
		log_debug("no parent options for %s", s);
	o = options_get_only(oo->parent, s);
	if (o == NULL)
		log_debug("%s not in parent options", s);
	return (o->tableentry);
}

struct options *
options_create(struct options *parent)
{
	struct options	*oo;

	oo = xcalloc(1, sizeof *oo);
	RB_INIT(&oo->tree);
	oo->parent = parent;
	return (oo);
}

void
options_free(struct options *oo)
{
	struct options_entry	*o, *tmp;

	RB_FOREACH_SAFE(o, options_tree, &oo->tree, tmp)
		options_remove(o);
	free(oo);
}

struct options_entry *
options_first(struct options *oo)
{
	return (RB_MIN(options_tree, &oo->tree));
}

struct options_entry *
options_next(struct options_entry *o)
{
	return (RB_NEXT(options_tree, &oo->tree, o));
}

struct options_entry *
options_get_only(struct options *oo, const char *name)
{
	struct options_entry	o;

	o.name = name;
	return (RB_FIND(options_tree, &oo->tree, &o));
}

struct options_entry *
options_get(struct options *oo, const char *name)
{
	struct options_entry	*o;

	o = options_get_only(oo, name);
	while (o == NULL) {
		oo = oo->parent;
		if (oo == NULL)
			break;
		o = options_get_only(oo, name);
	}
	return (o);
}

struct options_entry *
options_empty(struct options *oo, const struct options_table_entry *oe)
{
	struct options_entry	*o;

	o = options_add(oo, oe->name);
	o->tableentry = oe;

	return (o);
}

struct options_entry *
options_default(struct options *oo, const struct options_table_entry *oe)
{
	struct options_entry	*o;

	o = options_empty(oo, oe);
	if (oe->type == OPTIONS_TABLE_STRING)
		o->string = xstrdup(oe->default_str);
	else
		o->number = oe->default_num;
	return (o);
}

static struct options_entry *
options_add(struct options *oo, const char *name)
{
	struct options_entry	*o;

	o = options_get_only(oo, name);
	if (o != NULL)
		options_remove(o);

	o = xcalloc(1, sizeof *o);
	o->owner = oo;
	o->name = xstrdup(name);

	RB_INSERT(options_tree, &oo->tree, o);
	return (o);
}

void
options_remove(struct options_entry *o)
{
	struct options	*oo = o->owner;

	if (OPTIONS_IS_STRING(o))
		free((void *)o->string);

	RB_REMOVE(options_tree, &oo->tree, o);
	free(o);
}

const char *
options_name(struct options_entry *o)
{
	return (o->name);
}

const struct options_table_entry *
options_table_entry(struct options_entry *o)
{
	return (o->tableentry);
}

int
options_isstring(struct options_entry *o)
{
	if (o->tableentry == NULL)
		return (1);
	return (OPTIONS_IS_STRING(o));
}

const char *
options_tostring(struct options_entry *o, int idx, int numeric)
{
	static char	 s[1024];
	const char	*tmp;

	if (OPTIONS_IS_NUMBER(o)) {
		tmp = NULL;
		switch (o->tableentry->type) {
		case OPTIONS_TABLE_NUMBER:
			xsnprintf(s, sizeof s, "%lld", o->number);
			break;
		case OPTIONS_TABLE_KEY:
			break;
		case OPTIONS_TABLE_COLOUR:
			break;
		case OPTIONS_TABLE_FLAG:
			if (numeric)
				xsnprintf(s, sizeof s, "%lld", o->number);
			else
				tmp = (o->number ? "on" : "off");
			break;
		case OPTIONS_TABLE_STRING:
			break;
		}
		if (tmp != NULL)
			xsnprintf(s, sizeof s, "%s", tmp);
		return (s);
	}
	if (OPTIONS_IS_STRING(o))
		return (o->string);
	return (NULL);
}

char *
options_parse(const char *name, int *idx)
{
	if (*name == '\0')
		return (NULL);
	return ((char *)name);
}

struct options_entry *
options_parse_get(struct options *oo, const char *s, int *idx, int only)
{
	struct options_entry	*o;
	char			*name;

	name = options_parse(s, idx);
	if (name == NULL)
		return (NULL);
	if (only)
		o = options_get_only(oo, name);
	else
		o = options_get(oo, name);
	free(name);
	return (o);
}

char *
options_match(const char *s, int *idx, int* ambiguous)
{
	const struct options_table_entry	*oe, *found;
	char					*name;
	size_t					 namelen;

	name = options_parse(s, idx);
	if (name == NULL)
		return (NULL);
	namelen = strlen(name);

	if (*name == '@') {
		*ambiguous = 0;
		return (name);
	}

	found = NULL;
	for (oe = options_table; oe->name != NULL; oe++) {
		if (strcmp(oe->name, name) == 0) {
			found = oe;
			break;
		}
		if (strncmp(oe->name, name, namelen) == 0) {
			if (found != NULL) {
				*ambiguous = 1;
				free(name);
				return (NULL);
			}
			found = oe;
		}
	}
	free(name);
	if (found == NULL) {
		*ambiguous = 0;
		return (NULL);
	}
	return (xstrdup(found->name));
}

struct options_entry *
options_match_get(struct options *oo, const char *s, int *idx, int only,
    int* ambiguous)
{
	char			*name;
	struct options_entry	*o;

	name = options_match(s, idx, ambiguous);
	if (name == NULL)
		return (NULL);
	*ambiguous = 0;
	if (only)
		o = options_get_only(oo, name);
	else
		o = options_get(oo, name);
	free(name);
	return (o);
}

const char *
options_get_string(struct options *oo, const char *name)
{
	struct options_entry	*o;

	o = options_get(oo, name);
	if (o == NULL)
		log_debug("missing option %s", name);
	if (!OPTIONS_IS_STRING(o))
		log_debug("option %s is not a string", name);
	return (o->string);
}

long long
options_get_number(struct options *oo, const char *name)
{
	struct options_entry	*o;

	o = options_get(oo, name);
	if (o == NULL)
		log_debug("missing option %s", name);
	if (!OPTIONS_IS_NUMBER(o))
	    log_debug("option %s is not a number", name);
	return (o->number);
}

struct options_entry *
options_set_string(struct options *oo, const char *name, int append,
    const char *fmt, ...)
{
	struct options_entry	*o;
	va_list			 ap;
	char			*s, *value;

	va_start(ap, fmt);
	xvasprintf(&s, fmt, ap);
	va_end(ap);

	o = options_get_only(oo, name);
	if (o != NULL && append && OPTIONS_IS_STRING(o)) {
		xasprintf(&value, "%s%s", o->string, s);
		free(s);
	} else
		value = s;
	if (o == NULL && *name == '@')
		o = options_add(oo, name);
	else if (o == NULL) {
		o = options_default(oo, options_parent_table_entry(oo, name));
		if (o == NULL)
			return (NULL);
	}

	if (!OPTIONS_IS_STRING(o))
		log_debug("option %s is not a string", name);
	free(o->string);
	o->string = value;
	return (o);
}

struct options_entry *
options_set_number(struct options *oo, const char *name, long long value)
{
	struct options_entry	*o;

	if (*name == '@')
		log_debug("user option %s must be a string", name);

	o = options_get_only(oo, name);
	if (o == NULL) {
		o = options_default(oo, options_parent_table_entry(oo, name));
		if (o == NULL)
			return (NULL);
	}

	if (!OPTIONS_IS_NUMBER(o))
		log_debug("option %s is not a number", name);
	o->number = value;
	return (o);
}
