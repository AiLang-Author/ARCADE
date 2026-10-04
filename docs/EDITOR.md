# Level editor

The level editor is its own program, `level_edit.x`. It edits a side-scroller
level file. It does not run the player, and it does not stop the cabinet or
a Circuit window. Play is `docs/CIRCUIT.md`. The lines in the file are
`docs/LEVEL.md`.

From the Arcade directory:

```
./scripts/run_editor.sh
```

That opens a window titled Level editor, 900 by 720, on
`/dev/shm/level_edit`. `./scripts/install_desktop.sh` also adds an
applications-menu entry named ARCADE Level Editor. That entry runs the same
script.

The open file starts as `games/circuit/levels/edit.lvl`. Opening the editor
reads that file and does not write it. If it is missing, the editor reads
`levels/edit.lvl` into memory and leaves that older file where it is. Save
is the write. If neither file is on disk, the editor writes a small sample
at the game path so there is something to edit.

## Map

Six windows start open: Blocks, Enemies, Objects, Entities, Level, and
Attributes. Drag a title bar. The close box hides that window. Right click
off the map, or on a window, to open the menu titled Open. The menu lists
Blocks, Enemies, Objects, Entities, Level, Attributes, Behavior, Character,
Sound, and File. The first six show that window. The last four replace the
map. Esc, or the Map button, returns to the map.

Row 0 is the ground. It sits on the bottom of the view until the camera
scrolls. An empty cell is an outline when the level has a backdrop. With
no backdrop, the cell is filled and then outlined. A PNG `layer` line is
drawn behind the cells, up to four. The
editor draws a picture in the cell rectangle. It does not draw a ramp as a
slope, and it does not move the player.

Left click a block, an enemy, or an object to choose the brush. An entity
is listed and is not a brush. Left click and drag on the map stamps the
brush. The eraser is brush 0 and clears one cell. Right click a map cell
to clear that one cell.

## Keys

| Key | On the map |
|-----|------------|
| Arrows, or A D W S | Move the cursor. Up raises the row. The camera follows. |
| Z or Space | Paint, while you are not editing a field. Held, so it stamps while the key stays down. After a click on music, Esc leaves that row before these keys paint again. |
| Enter | Cycle the brush, while you are not editing a field. Entities are skipped. 0 is erase. |
| P | Open Files, unless Files is already open. |
| N | Store this level, then show the next. It wraps. |
| Q | Quit, unless a file name is being typed. |
| Esc | Close the menu, leave a page, or leave a number field. It does not quit. |
| Digits, Backspace | A number on Level, Attributes, or Behavior. At most 8 digits. Sound has no number to type. |
| Close the window | Quit. |

On a file name, the accepted characters are letters, digits, and a hyphen,
at most 24. Q is a letter there.

## Level

The Level window starts with the tile field ready for digits. Click a row,
type, and Enter stores it and moves to the next row through leap. The rows
are tile mm, width, height, view wide, view tall, gravity, fall, run, jump,
walk, wind s, leap x, and music.

Enter on a 0 for walk, wind, or leap fills the current feel. Walk becomes a
quarter of run, and that becomes 1 when run is at least 1. If run is 0,
walk stays 0. Wind becomes 3 seconds. Leap becomes 2.
The map is capped at 128 by 32 tiles. The view stays inside the map. A tile
typed below 1 becomes 200 mm when Enter stores it. A tile line that loads
below 1 is kept as 1 mm. A file holds at most 8 levels. N switches among the
levels already in the file. The map does not add a level.

Click the music row to cycle wav, mp3, and ogg files in
`assets/circuit/audio/music`, up to 16 names, then none. The editor does not
play the track. Save writes `music <path>` when one is chosen. A name may
contain spaces. Circuit plays level 1's line only.

Wind is how many seconds a held direction takes to build from walk to run
when Circuit plays the file. Leap is stored. Circuit does not use it as a
jump multiplier.

## Attributes

Attributes follows the current brush. An empty brush says empty.

Blocks and objects list destruct, gravity, spring, reward, hurt, heal, slip,
climb, goal, and crumble, then direction and emit, then checkpoint and
boost. Click a yes/no row to toggle it. Click a number, type, and Enter
stores it. Reward cycles none, 1up, and points. The first 1up sets the
number to 1 when it was 0. Direction cycles none, left, right, up, and down.
Emit cycles the objects, or none.

Blocks also have a slope row. Click cycles none, up, down, up low, up high,
down high, and down low. Those are the edge pairs 0/0, 0/8, 8/0, 0/4, 4/8,
8/4, and 4/0. The numbers are eighths of the tile, left edge then right
edge. A pair that is not one of those still loads, shows as the two numbers,
and is kept on the next save. The next click then starts again at up.
Objects and enemies do not have this row. `0` and `0` stays a square. Save
writes `sleft` and `sright` only when one of them is not 0.

Enemies do not show that effect list. Their rows are hit points, defense,
offense, motion, speed, and zone, then where, col, and row. Motion cycles
still, walk, run, fly, hop, and chase. Where steps through the cells that
enemy already occupies. Col and row move that cell. If it has no cell yet,
type a column and a row and Enter on each. That writes one cell.

## Behavior

Right click and choose Behavior. The left list is enemies. The middle list
is objects. The right side is motion, speed, health, stomp, ledge, and zone.
On an object, stomp is labeled pickup. Motion, stomp, and ledge are clicks.
Speed, health, and zone are typed. Up and down move through the combined
list. Zone is how many cells from the spawn. 0 keeps a walk and a run on
that cell. A hop still hops there. A fly still bobs.

The editor stores these numbers. Circuit is what moves the enemy.

## Character

Right click and choose Character. The list is the entity sheets. Click one,
or use up and down. The side picture cycles that sheet's frames. It does not
stamp the character on the map. Save writes `character anim <name>`, or
`character anim none` when nothing is selected. That line is written once,
before the first `level` line.

Circuit reads `character` only after `level 1`, so the line the editor
writes is kept and not applied. The built-in idle, run, and jump sheets
play. The same line placed under level 1 replaces the idle sheet when that
entity exists. The run sheet stays `hero-run-sheet` and the jump sheet
stays `hero-jump`.

## Sound

Right click and choose Sound. The left column is the event, in this order:
jump, land, hurt, stomp, ray, goal, checkpoint, powerup, derez, crouch. The
right column is a wav under `assets/circuit/audio/sfx/`, including one
folder down. An event with no clip says built in.

Play, Z, or Space hears the highlighted clip once. Holding the button does
not play it again. Add, or Enter, stores that clip on the highlighted event.
Clear removes the assignment, and the save omits that line so Circuit keeps
the clip it already ships. Up and down move through the clips. Left and
right change the event.

Save writes `sound <event> <path>`. The path is relative to
`assets/circuit/audio/sfx/`. Use one word. A reload keeps the first word,
so a space in the name does not come back.

## Files

The File button is the upper left of the map. Right click and choose File,
or press P. The buttons are Save, Save as, New, Edit, and Delete.

A game is a folder. The open game starts as `circuit`. Its levels are
`games/circuit/levels/`. The word `game` lists the folders under `games/`.
Click one to open that game. Type a name and Enter to create
`games/<name>/levels/`. A name is lowercase letters, digits, and a hyphen, at most 24 characters.
Creating a game does not write the level until Save, and it does not remove
`games/circuit`. The editor does not add a cabinet row.

Save writes the open file. Save as asks for a name, writes
`games/<game>/levels/<name>.lvl`, and that file stays open. New asks for a
name, keeps every picture and its attributes, clears the map and the
character choice, keeps the music and the layer paths, resets the level
numbers, and writes that file. Edit reads the file you click and does not
write it. Delete removes the file you click.

## What a save writes

Save overwrites the open file. The order is the level count, then each
asset with its attribute lines and, for enemies and objects, two behavior
lines, then one `sound` line per assigned event, then one `character anim`
line, then each level. A level is its header, `music` when a track is set,
up to four `layer` lines, and horizontal `span` lines. Empty cells are
skipped. An entity is not written as a span.

The grammar of those lines is `docs/LEVEL.md`.

## What the editor does not do

It does not run gravity, the windup, a jump, a ramp, a conveyor, or hit
points. It does not preview the level's music. The sound page plays one
clip. There is no pause screen and no volume slider. N changes which level
is on screen. It does not play it.

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
