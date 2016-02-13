Proposal
========

Currently, CWM uses YACC to define its own configuation file syntax.  Whilst
this is OK, it's a little cumbersome to change.  This document outlines how
a new configuration file might look like using `libconfuse` instead.

Why?

YACC is very powerful for defining your own grammar and structure of
configuration files, and even means such definitions are self-contained.
However, it is rather fiddly to not only define this but write validation
routines to satisfy the parsing of such a file.

There are many external libraries which can be used for [configuration
parsing](http://arxiv.org/pdf/1103.3021.pdf).  Many of them allow for some
kind of structured data and validation routines.  `libconfuse` is fairly
lightweight and provides the necessary sections to help structure how CWM
currently works, and how it might change in the future, without the need for
having to define custom grammars, like YACC would need. 

Current Format
==============

`~/.cwmrc` is read in line-by-line.  The line is sent through `yyparse()`.

## Current Commands

```
activeborder
autogroup
bind
borderwidth
color
command
gap
groupborder
ignore
inaciveborder
mousebind
moveamount
snapdist
sticky
```

# Format
## Screen

The following section applies to screens.

```
screen $SCREEN {
	gap {0,0,0,0}
	snapdist 0	
}
```

Where `$SCREEN` can be the specific name of an output (from `xrandr(1)`, or
the literal `*` to indicate any/all screens, regardless of name.  Given the
way `cwm-new` parses the configuration file, the generic `*` screen **must**
come before any specific screen sections.

### Gap
Describes how many pixels from the `top/bottom/left/right` (respectively)
the screen is "shrunk" such that windows won't overlap over this area.

### Snapdist
Describes in pixels, how many pixels from the gap a window should be before
it is moved to it.  This only applies during window movement.

## Groups

### Bindings

These have a section to themselves, and are global to the window manager
(that is, they do not apply to individual screens, or groups).

The syntax is as follows:

```
bindings {
	key		C-m { command = maximize }
	mouse	4	{ command = hide }
}
```

There are two sections allowed: `key` and `mouse`.  Each section has a
title which denotes the modifier and key (or mouse button, if the section is
`mouse`), and then a body containing the `command`.

### Clients

Code and documentation TBD.

Differences / Deprecations
==========================

* Clients:
	- Belong to more than one group (changes `autogroup`)
	- Matched in a specific order + regexp matching (changes `autogroup`)

* Groups:
	- `color`, `borderwidth`, etc., are now per-group 
