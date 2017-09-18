# cwm-new

[![Build
Status](https://travis-ci.org/ThomasAdam/cwm-new.svg?branch=new)](https://travis-ci.org/ThomasAdam/cwm-new)

This is a working title for what will become a new window manager.  Ideas
for a name are welcome.

This is based from
[CWM](http://cvsweb.openbsd.org/cgi-bin/cvsweb/xenocara/app/cwm/) in
OpenBSD.

# Requirements

* pkg-config
* Xft
* RandR

# Features

Current features which differ from cwm are:

* RandR is used for monitor detection, not Xinerama;
* group0 is no longer special, it's just another group;
  * Toggling all groups is available as a command.
  * Groups are per RandR output, and are separate for each output;
  * Status output sent to a named FIFO (`/tmp/cwm.fifo`):
    * Can be parsed and used with dzen2/lemonbar, for instance, check the
	  [read_status.pl](examples/read_status.pl) file for an example of this.
* Hooks
  * Clients have a few hooks which can be defined, and multiple actions occur
    against said client.

# Status bar

Although any bar can be used, there have been some improvements to `lemonbar`
which mean that it's preferred over the official `lemonbar repository`.
Changes include:

* Xft support
* Not clearing output on monitors
* Support for per-screen (RandR) messages.

The [example script](examples/read_status.pl) included with `cwm-new` makes
use of these features which are not found in the official `lemonbar`
repository.

For the forked version containing these changes,
[see this repository](https://github.com/ThomasAdam/bar/tree/ta/keep-output-monitor)

# Screenshot

Obligatory screenshot below!  The bar at the top is lemonbar, with conky
output on the right.

![screenshot](www/screenshot.png)

# Tasks

See the [TODO](TODO.md) file.

# Contact

You can reach me via the following:

* Email:  thomas.adam22@gmail.com
* IRC:	  ```thomas_adam``` on ```freenode.net```
