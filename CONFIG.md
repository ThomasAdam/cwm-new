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
autogroup
bind
borderwidth
color
command
fontname
gap
ignore
mousebind
moveamount
snapdist
sticky
```

Proposed Format
===============

```
screen $SCREEN {
	group {
		1 {
			borderwidth
			color
		}
		2 {
			borderwidth
			color
		}
	}
	gap
}
```

Settings which should be global, are given priority and put inside a
`global` block.  These apply across all screens by default, and can be
overwritten by specific screen blocks, as shown above.

```
global {
	...
}
```

### Bindings

```
bind {
	C-m maximize
}
```

```
mousebind {
	1 menu_unhide
}
```

### Clients

```
client {
	class/resource/title {
		autogroup 1,2,3
		color ...
	}
}
```

Differences / Deprecations
==========================

* Clients:
	- Belong to more than one group (changes `autogroup`)
	- Matched in a specific order + regexp matching (changes `autogroup`)

* Groups:
	- `color`, `borderwidth`, etc., are now per-group 
