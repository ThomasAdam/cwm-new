Rough ideas for CWM
===================

* Consider tmux to be a good model for window behaviour:
  * Configuration:  defaults hard-coded, then overridden in config file.
    * Perhaps using liblfg:  http://liblcfg.carnivore.it/
* linking a window (read:  "sticky") should have separate
  geometry per workspace; when switching workspaces, the client's
  geometries are relocated.
  * Clients could have some undo list for last known positions.
* Hooks?  Events to be fired off after certain actions.
*  Window/workspace actions via command-prompt (just like ratpoison).
* Internal structs:
  * Don't use typedefs.
  * Create a geometry struct to hold size/position information and
    update that for operations including maximise.
* Client geometry is per group; hence if more than one group is displayed at
  any one time, and a client is moved, its new geometry is not recorded.
* Windows should snap to each other and to screen edges.
* No special-casing of maximised state, it's just another geometry set which
  is added to the list, and popped when no longer in that state.
