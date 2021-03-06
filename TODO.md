Rough ideas for CWM
===================

* Consider tmux to be a good model for window behaviour:
  * [x] Configuration:  defaults hard-coded, then overridden in config file.
    * [x] Perhaps using liblfg:  http://liblcfg.carnivore.it/
* [x] linking a window (read:  "sticky") should have separate
  geometry per workspace; when switching workspaces, the client's
  geometries are relocated.
  * [ ] Clients could have some undo list for last known positions.
* Hooks?  Events to be fired off after certain actions.
  * [x] client hooks
  * [ ] group hooks
*  [ ] Window/workspace actions via command-prompt (just like ratpoison).
* Internal structs:
  * [x] Don't use typedefs.
  * [ ] Create a geometry struct to hold size/position information and
    update that for operations including maximise.
* [ ] Client geometry is per group; hence if more than one group is displayed at
  any one time, and a client is moved, its new geometry is not recorded.
* [ ] Windows should snap to each other and to screen edges.
* [ ] No special-casing of maximised state, it's just another geometry set which
  is added to the list, and popped when no longer in that state.
* [ ] Redo stacking orders.  Would allow for _NET_WM_ACTION_ABOVE/BELOW, etc.,
  as well as proper window layers.
* [ ] New command `info` which pops up a little window (same as when resizing)
  which lists information about the window.  Think FvwmIdent from FVWM.
* [ ] Groups could be "global" across monitors, and switching to a group on a
  differnet monitor switches it to that monitor.  Much like how XMonad or
  herbstluftwm works.
* [ ] Handle primary output and add first to the list of screens we manage.

Documentation
-------------

* Document JSON format (cwm.1)
