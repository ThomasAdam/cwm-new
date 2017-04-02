Examples
========

In this directory are a few files:

`.`:

* panel -- a wrapper sh script which chains together a perl script and
  lemonbar, so that a panel can be formed.  Has a crude understanding of job
  control so that only one instance can be started at any one time.  It will
  also start conky.

* read_status.pl -- listens on any defined FIFOs from CWM, and parses the
  JSON information sent down them.  It formats the information into
  something which `lemonbar` can understand.  It will also send the conky output
  to `lemonbar`.

* .conkyrc -- example RC file for use with conky.

`./config`
* Example config(s)

**It is expected that `panel` should be changed to match the path of the
other scripts.  Currently, $PWD (./) is assumed.**
