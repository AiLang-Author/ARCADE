# Circuit

Circuit is a 2D side-scroller and its own program, `circuit.x`. The cabinet
lists it as the third game. Enter on that row starts it in the same window.
You can also open it by itself. NOWAY HOME and GYRE stay inside the cabinet
program. Circuit is not imported into that program.

From the Arcade directory:

```
./scripts/run_circuit.sh
```

That starts `circuit.x` and a window titled Circuit, 900 by 720. It reads
`games/circuit/levels/edit.lvl` and does not write that file. Q or Esc leaves
that window. Closing the window leaves too.

On the cabinet window, Q or Esc quits the cabinet. Enter on Circuit's pause
screen returns to the game list. The editor is a different program. How to
use it is `docs/EDITOR.md`. The level-file lines are `docs/LEVEL.md`.

## Play

| Key | Action |
|-----|--------|
| Left and right | Walk. Hold one and the pace builds up to a run, about one second on this level. |
| Enter | Reach that run immediately. |
| Up, Z, or Space | Jump. A jump at the end of the windup goes farther. |
| Down | Crouch, on the ground, while not climbing and not holding Up. |
| X | Fire the ray. |
| P | Pause. Press P again to resume. |
| Q or Esc | Leave this window. On the cabinet window, the same keys quit the cabinet. |

A and D match left and right. W matches up. S matches down, so S crouches
and the run stops.

The status line shows hit points. The player starts with 3. A side touch
from an enemy, or a hurt tile, spends one. At 0 the player returns to the
last checkpoint and the count refills. An extra life adds one to the
current count and to the maximum. Falling into a pit does not refill them.

Landing on an enemy spends one of that enemy's hit points when it can be
stomped. At 0 the enemy is gone. A sweeper walks, a scrubbit hops, an arc
beetle chases inside its zone, and a zapfly flies and cannot be stomped.
Other enemies stay put. The editor can store defense and offense. Circuit
does not read them. A hit spends one point.

The ray is a short cyan beam. It spends one hit point on the first enemy
it reaches. It stops at a solid cell and at a ramp.

These tiles from the city file are live: a jump pad launches, a ladder can
be climbed, the goal gate plays its clip and the run stays on screen, a bit
is worth 100 points, an extra life is one life, a checkpoint stores the
respawn, and an overclock chip removes that cell and holds run speed for
300 frames.

## Pause and volume

P freezes the player, the enemies, and the ray. The picture stays up, and
a panel covers the middle of it. The music holds on the note it had
reached and continues from there when you resume. Effects stay quiet while
nothing is moving.

Up and down pick Music or Effects. Left and right change the highlighted
one by one step. Each one runs from 0 to 10. Ten pips show the level.
The numbers start at music 4 and effects 8, which is about how the window
sounded before the sliders existed. 10 is as loud as the track and the
clips get. 0 is silent. People who want it hotter turn it up. People who
want it quieter turn it down.

The choice is written to `games/circuit/volume.txt` and used the next time
the window opens. That file is this machine's preference. It is not part
of the level.

Q and Esc still leave while the panel is up. On the cabinet window that quits the cabinet. On the standalone window it leaves Circuit. A direction key that was
already held when you pressed P does not move a slider. Release it, then
press it, to change the level.

## What you hear

Level 1 loops `assets/circuit/audio/music/Reassemble Me.mp3`.
Circuit plays level 1 only. A music line saved on level 2, 3, 4, or 5
stays in the file and is not played.

The city file plays these clips, from `assets/circuit/audio/sfx/`:

| Moment | Clip |
|--------|------|
| Jump | `jump.wav` |
| Land | `stomp.wav` |
| Hurt | `hurt.wav` |
| Stomp | `block-bump.wav` |
| Ray | `Firepower/Hit_Enamy_3.wav` |
| Goal | `goal.wav` |
| Checkpoint | `goal.wav` |
| Powerup | `powerup.wav` |
| Player dropped | `derez.wav` |
| Crouch | `land.wav` |

## Cabinet

The cabinet list has a Circuit row. Up and down reach it. Enter replaces the menu with Circuit in the same window. The other two games stay in the cabinet program. Circuit is still its own program.

P opens the same pause panel. Enter on that panel returns to the game list. Q or Esc quits the cabinet window. The standalone window, started with `./scripts/run_circuit.sh`, is unchanged: Q or Esc leaves that window, and Enter does not return anywhere.

## Editor

From the Arcade directory:

```
./scripts/run_editor.sh
```

That opens the level editor in its own window. It does not stop the cabinet,
and it does not stop a Circuit window. P in the editor opens Files. P in
the game pauses. The full guide is `docs/EDITOR.md`.

Opening the editor reads `games/circuit/levels/edit.lvl` into memory and
does not write it. If that file is missing, it reads `levels/edit.lvl` and
still does not write. Save is the write. Save as and New also write. If
neither file is on disk, the editor writes a small sample so there is a
file to edit. Choosing Edit reads the file you click and does not write it.

The Level window stores walk, wind, and leap. Walk is meters per second.
Wind is how many seconds a held direction takes to build from that walk to
the run. Leap is stored with the level and does not change the jump. The
jump keeps the speed already built. This city file has walk 2, wind 1,
jump 18, and run 8. Enter reaches the run immediately.

The same window has a music row. Click it to cycle the tracks in
`assets/circuit/audio/music`, then none. A track name may contain spaces.
Save writes `music <path>` on that level, relative to the Arcade directory.
Circuit plays level 1's line. No line on level 1 stays silent.

Sound assigns a clip to an event. Play, Z, or Space hears the highlighted
clip once. The path is relative to `assets/circuit/audio/sfx/`. Use one
word. A reload keeps the first word.

Attributes, on a block or an object, stores the tile effects, including
checkpoint and boost. A block has a slope row. Clicking it cycles none
and six ramps. The file stores `sleft` and `sright` for that block, so a custom
picture can be a ramp. The picture name does not decide it. Behavior stores
how an enemy moves, how many hits it has, whether it can be stomped, and
whether it turns at a ledge. Character picks an entity sheet. The editor
writes `character anim` before the first level. Circuit reads that key only
inside level 1, so the saved line is kept and not applied. The built-in
idle, run, and jump sheets play. The same line placed under level 1 replaces
the idle sheet when that entity exists. The run sheet stays `hero-run-sheet`
and the jump sheet stays `hero-jump`.

The line format is `docs/LEVEL.md`.
