Examples
========

In this directory are a few files:

* panel -- a wrapper sh script which chains together a perl script and
  lemonbar, so that a panel can be formed.  Has a crude understanding of job
  control so that only one instance can be started at any one time.  It will
  also start conky.

* read_status.pl -- listens on any defined FIFOs from CWM, and parses the
  JSON information sent down them.  It formats the information into
  something which lemonbar can understand.

* barpanel -- aggregates together output from either conky or read_status.pl
  and adds any additional formatting for use in lemonbar.

* .conkyrc -- example RC file for use with conky. 
