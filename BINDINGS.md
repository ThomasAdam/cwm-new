# Introduction

Currently, there are a number of overlapping/duplicate command which are
present in `cwm-new`.  Furthermore, almost all of them are exclusive to key
bindings, with there being only a very small subset available to bind commands
to mouse.

# Proposal

With the ever-increasing rise in popularity to script actions, it seems
conceivable that `cwm-new` should have some means of unifying actions, and to
have the same set of actions available to both key and mouse bindings.
Additionally, each command would allow for external scripting, assuming
`cwm-new` ever listened on a comms socket.

# Current Commands

The following commands are available.

## Key Bindings

* lower
* raise
* search
* menusearch
* groupsearch
* hide
* expand
* cycle
* rcycle
* label
* delete
* group0
* group1
* group2
* group3
* group4
* group5
* group6
* group7
* group8
* group9
* grouponly0
* grouponly1
* grouponly2
* grouponly3
* grouponly4
* grouponly5
* grouponly6
* grouponly7
* grouponly8
* grouponly9
* [X] movetogroup0
* [X] movetogroup1
* [X] movetogroup2
* [X] movetogroup3
* [X] movetogroup4
* [X] movetogroup5
* [X] movetogroup6
* [X] movetogroup7
* [X] movetogroup8
* [X] movetogroup9
* nogroup
* cyclegroup
* rcyclegroup
* cycleingroup
* rcycleingroup
* grouptoggle
* sticky
* fullscreen
* maximize
* vmaximize
* hmaximize
* freeze
* restart
* quit
* exec
* exec_wm
* ssh
* terminal
* lock
* [X] moveup
* [X] movedown
* [X] moveright
* [X] moveleft
* [X] bigmoveup
* [X] bigmovedown
* [X] bigmoveright
* [X] bigmoveleft
* [X] resizeup
* [X] resizedown
* [X] resizeright
* [X] resizeleft
* [X] bigresizeup
* [X] bigresizedown
* [X] bigresizeright
* [X] bigresizeleft
* ptrmoveup
* ptrmovedown
* ptrmoveleft
* ptrmoveright
* bigptrmoveup
* bigptrmovedown
* bigptrmoveleft
* bigptrmoveright
* [X] snapup
* [X] snapdown
* [X] snapleft
* [X] snapright
* htile
* vtile
* window_lower
* window_raise
* window_hide
* window_move
* window_resize
* window_grouptoggle
* menu_group
* menu_unhide
* menu_cmd
* toggle_border

## Mouse Bindings

```
window_move
window_resize
window_lower
window_raise
window_hide
window_grouptoggle
cyclegroup
rcyclegroup
menu_group
menu_unhide
menu_cmd
```

# Consolidation

The following table shows how to unify the commands...

| Original Cmd(s)	      | New Cmd                               |
|-----------------------------|---------------------------------------|
| movetogroup[0-9]            | move -s`win_id` -t0                   |
| snap[up,down,left,right]    | move -s`win_id` -d [up,down,left,right] -w|
| move[up,down,left,right]    | move -s`win_id` -d [up,down,left,right]|
| bigmove[up,down,left,right] | move -s`win_id` -d [up,down,left,right] -p [n]|
| resize[up,down,left,right]  | resize -s`win_id` -d [up,down,left,right] -p [n]|
| bigresize ...               |                  ...                   |


# Syntax

Mention `-t`, `-s`, etc.
