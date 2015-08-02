# cwm-new

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
		- Toggling all groups is available as a command.
	* Groups are per RandR output, and are separate for each output;
	* Status output sent to named FIFOs for each monitor detected:
		- Can be parsed and used with dzen2, for instance, check the
		  read_status.pl file for an example of this.

# Tasks

See the [TODO](TODO.md) file.

# Contact

You can reach me via the following:

* Email:  thomas.adam22@gmail.com
* IRC:	  ```thomas_adam``` on ```freenode.net```
