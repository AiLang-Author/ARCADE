# Side-scroller level file

The level editor reads and writes this text. The cabinet does not load it
yet. NOWAY HOME and GYRE do not use it. There is no player physics here.

Pictures are files you already have. An `asset` line names an SVG or a TVG
on disk, the same way the cabinet loads a ship or a GYRE strip. The editor
rasterizes that file into the tile rectangle and keeps the bitmap for the
session. The text file stores the path. It does not store pixels. A missing
file stays an empty outlined cell. The editor does not invent a picture.

Run it with `./scripts/run_editor.sh`. That uses `/dev/shm/level_edit` and
does not stop the cabinet. Arrows move the cursor and the camera follows.
Z or Space paints the current brush. Enter cycles the brush, including an
eraser. N switches levels. The File button in the upper left, or P, opens Save, Save as,
New, Edit, and Delete. Save writes the open file. Q quits, except while
a file name is being typed.

Blocks, Enemies, Objects, Entities, and Level are windows on the right, the same kind
of window as the desk and the datalogger: a title bar, a close box, and a
body. Drag the title. Click a block, an enemy, or an object to choose the
brush. An entity is listed in its window and is not painted onto the map.
Click a level line, type digits, and Enter stores that line. Esc leaves the
line. Right click a map cell to clear that cell. Right click off the map to
open a closed window again. The map sits to the left of the windows.
Left click and drag still paint. Row 0 is the ground.

## Units

The map is a grid. Column 0 is the left edge. Row 0 is the ground. Row
numbers count up from the ground, so a floor is row 0 and ten tiles above
the ground is row 10.

`tile` is how big one cell is in the real world. The size is per level. A
bare number is millimeters. A suffix may be `mm`, `cm`, `m`, `in`, or `ft`.
`tile 20cm` and `tile 200` are the same cell. The default is 200 mm.

`width` and `height` are the world. `view` is the camera, in tiles wide and
tiles tall. A bare number is already tiles. A number with a unit is a
length, divided by `tile`. Put `tile` before a width that uses a unit.

```
tile 200
width 40
height 16
view 16 10
```

At 200 mm a tile, that world is 8 m by 3.2 m, and the screen shows 16 by 10
tiles. The editor caps a map at 128 by 32 tiles and eight levels.

## Header

Order inside a level does not matter. The editor writes this order.

| Line | Meaning |
|------|---------|
| `levels <n>` | How many levels are in the file. Once, at the top. |
| `level <n>` | Which level the following lines belong to. `1` is the first. |
| `tile <size>` | World size of one cell. Default 200 mm. |
| `width <length>` | World width. |
| `height <length>` | World height. |
| `view <wide> <tall>` | Camera. Default 16 by 10. |
| `gravity <m/s²>` | Level gravity. Circuit plays at four times this. Default 30. |
| `fall <m/s>` | Fall cap. Default 20. |
| `run <m/s>` | Turbo speed, held with Enter. The windup will not pass this. Default 8. |
| `jump <m/s>` | Jump takeoff. Default 14. |
| `walk <m/s>` | Walking speed. `0` means a quarter of run, and at least 1. |
| `wind <seconds>` | Seconds to build from walk to twice that walk. `0` means 3. |
| `leap <times>` | How far a jump at the end of that windup goes, measured in walk jumps. `0` means 2. |
| `music <path>` | Track for this level. Path is relative to the Arcade directory and has no spaces. No line means silence. |
| `asset <name> <block\|enemy\|object\|entity> <path> [w h] [frames]` | An external picture in one group. |
| `attr <name> break <0\|1> grav <n> spring <n>` | Stored on that asset. Circuit applies the player-facing keys. |

`w` and `h` are the stamp size in tiles when you paint that brush. Default
`1 1`. `frames` is how many cells are in a horizontal SVG strip. The editor
cycles those frames while you edit so a repeating tile does not show a seam.
A path that ends in `.tvg` is square and one frame. Default frames is 1.
Names are one word. The group word is the category: block, enemy, object,
or entity. A line from before groups, `asset Name path`, still loads, and
that picture is a block until you save. The folder in the path does not
pick the group. An entity stays in the file for a later character screen.
It is not a cell on the map.

Paths are relative to the Arcade directory, the same as `assets/ships/player.svg`.

## Placement

A cell is a name at a column and a row. A run of the same tile is one line,
so a floor is not one line per cell.

```
span Ship 0 0 40 1
put Bee 6 2
Bee 14 2 3 1
```

`span` and `put` are the same statement. `Name col row` works once that name
is an asset. `col` and `row` are the lower-left cell. The last two numbers
are width and height in tiles. The editor saves horizontal runs as `span`.
A span that names an entity is ignored on load, and a save does not write
one. Right click on a cell clears that one cell.

`layer <path>` is a PNG drawn behind the map, back to front, up to four
lines on a level. The editor decodes that file with the PNG library. An
empty cell is a grid outline so the picture shows through. A layer is not
a brush. The bars on the right of Blocks, Enemies, Objects, and Entities
scroll the pictures when a window cannot show all of them.

The Circuit Runner pack is `assets/circuit`. The editor keeps that game in
`games/circuit/levels/`. `levels/edit.lvl` is the older single file. If the
game file is missing, the editor still reads the older file and does not
delete it. Hero sheets are entities, so they stay in the Entities window
and are not stamped on the map.

```
span <name> <col> <row> <w> <h>
```

Row 0 is the ground. The editor draws that row at the bottom of the view.

## Bitmap

The file stores the path and the tile size. When the level opens, and again
when the window changes the on-screen size of a cell, each used file is
rasterized once:

```
tile_px = the editor's cell, capped at 32 device pixels
bitmap   = frames * tile_px  by  tile_px     (SVG strip)
bitmap   = tile_px by tile_px                (TVG)
```

The bitmap is kept for the session under that path and pixel size. It is
thrown out on a resize that changes the cell. The coordinates in the file
stay put. The picture is scaled to the cell. It is not measured to invent a
hit box.

## Example

This is the file the editor writes when no level file is found. The
three paths already exist in the cabinet. They prove the loader. A real
floor is a file you drop in and name on an `asset` line.

```
levels 2
asset Ship block assets/ships/player.svg 1 1 1
asset Bee enemy assets/enemies/bee.svg 1 1 1
asset Kestrel entity assets/gyre/hero-kestrel-strip.svg 1 1 8
level 1
tile 200
width 40
height 16
view 16 10
gravity 30
fall 20
run 8
jump 14
span Ship 0 0 40 1
span Bee 6 2 1 1
span Bee 14 2 3 1
level 2
tile 200
width 20
height 8
view 16 8
gravity 30
fall 20
run 8
jump 14
span Bee 3 1 1 1
span Bee 9 1 1 1
```

Kestrel stays in the Entities window. It is not stamped on the map. The
strip is eight cells for a later character screen. Ship and Bee are single
pictures you can paint.

## Attributes

The Attributes window edits the asset selected as the brush. Each effect is
a number on that asset. `0` means off, except `grav 0`, which means the
level gravity. A smaller gravity is lighter and a larger gravity is heavier.

| Key | Effect |
|-----|--------|
| `break` | Destructible. Click toggles yes or no. |
| `grav` | How heavy it is. |
| `spring` | How springy it is. |
| `reward` | Click cycles `none`, `1up`, `points`. `1up` is an extra life and the number is how many lives. `points` is the score. Click the number, type it, and Enter stores it. |
| `hurt` | How much it hurts on touch. The row shows the number, including 0. Click it, type, and Enter stores it. |
| `heal` | How much it heals on touch. Same as hurt: click, type, Enter. |
| `slip` | How slippery it is. Same as hurt. |
| `climb` | Can be climbed. Click toggles yes or no. |
| `goal` | Touching it finishes. Click toggles yes or no. |
| `crumble` | Gone after one touch. Click toggles yes or no. |
| `emit` | Which object this asset releases. Click cycles the objects. `none` releases nothing. |
| `dir` | Blocks and objects only. `none`, `left`, `right`, `up`, or `down`. A conveyor set left pushes that way, which turns a player who was moving right back the other way. Click cycles it. |
| `check` | Blocks and objects. `1` stores the respawn here. Click toggles yes or no. |
| `boost` | Blocks and objects. Touch clears the cell and raises speed for this many frames. `0` is off. Click, type, Enter. |

Circuit applies spring, climb, goal, reward, hurt, crumble, and conveyor direction. Slip and emit are stored only. The file keeps the old `break grav spring`
line and adds lines for the other keys, including `emit <object>`.
The reward style is saved on the crumble line as `rkind <none|1up|points>`.
The reward number stays on the reward line. A save writes all of them.

An extra life is the object, not a separate attribute type. Select
`extra-life` and click reward until it says `1up`. The number next to it
is how many lives, and the first click sets that number to 1. Select a
block and click emit until the name is `extra-life`. The block releases
that object. The object's own reward is the 1-up. Points are the same
click on reward, then click the number and type the score. Enter stores
it. That number stays on the asset. It does not change the level view.
The released object's behavior is edited on the Behavior screen.

Blocks and objects show the effect list. Enemies do not. An enemy's
attributes are hit points, defense, and offense, then motion, speed,
and zone, then the spawn row. Hit points is the same number as behavior
health. Defense is how much it resists. Offense is how hard it hits.
Click a stat, type, and Enter. Motion is a click: still, walk, run, fly,
hop, chase. Speed is how fast, and 0 means it does not move on its own.
Zone is how many cells it may travel from its spawn. 0 means it stays
on that cell. A save adds one line for enemies:

```
attr <name> hp <n> def <n> off <n>
```

An enemy's spawn points are the cells where that enemy already sits. The
where row is which of those cells, starting at 1. col and row move that
cell. Click where again to step to the next spawn. If the enemy has no
cell yet, type a col and a row and Enter on each. The editor writes one
cell there. Save, on the File screen, writes the attr lines and the spans.

## Behavior

Right click and choose Behavior. That screen is separate from the map.
Esc, or the Map button, returns to the map. Up and down move through the
list. Save, on the File screen, writes the behavior lines.

Enemies and objects are both listed. A mushroom is an object: Attributes
on a block sets `emit` to that object, and this screen stores how that
object acts. The same fields are stored for an enemy.

| Key | Effect |
|-----|--------|
| `motion` | 0 still, 1 walk, 2 run, 3 fly, 4 hop, 5 chase. Click cycles them. |
| `speed` | How fast. 0 means it does not move on its own. |
| `health` | How many hits. 0 means one hit. |
| `stomp` | An enemy can be landed on. On an object the same field is pickup. |
| `ledge` | Turns around at an edge. |
| `zone` | How many cells from the spawn. 0 means it stays on that cell. |

Circuit moves an enemy from these lines. A missing line means still,
speed 0, and one hit. The city file stores the motions in
`games/circuit/levels/edit.lvl`: a sweeper walks, a scrubbit hops, an arc
beetle chases inside its zone, and a zapfly flies. Any other enemy stays
put. Speed is meters per second.
Walk and run stay on the ground and turn at a wall. Ledge also turns them
at an edge. Run is twice the speed. Fly ignores the floor and patrols the
zone. Hop is a walk with a short hop. Chase moves toward the player and
will not leave the zone. The enemy's cell is the hit box. A side overlap pushes the player out of
that cell, holds the shove, and spends one of the player's hit points. The
player starts with 3, shown on the status line. At 0 the player returns to
the last checkpoint and the count refills. An extra life adds to that count.
Falling onto the center bounces the player when stomp is set, and that
spends one enemy hit point. A missing health, or 0, means one hit. At 0
the enemy is gone. Defense and offense stay stored.

A save writes two lines, and only for enemies and objects:

```
behave <name> motion <0-5> speed <n> zone <n>
behave <name> health <n> stomp <0|1> ledge <0|1>
```

Blocks and objects also save `attr <name> dir <none|left|right|up|down>`.

## Character

Right click and choose Character. That screen is separate from the map.
It lists the entity sheets, which are the character's animations. Click
one to select it. Esc, or Map, returns to the map. Up and down move
through the list. Save, on the File screen, writes one line:

```
character anim <entity-name>
```

The little picture cycles that sheet's frames so you can see which
animation you picked. It does not put the character on the map.

## Sound

Right click and choose Sound. That screen is separate from the map.
The left column is the event. The right column is a clip from
`assets/circuit/audio/sfx/`. Play hears the highlighted clip once and
does not repeat while the button stays down. Add stores that clip on
the highlighted event. Clear removes it, and Circuit keeps the clip it
already ships for that event. Up and down move through the clips. Left
and right change the event. Enter adds. Z plays. Esc, or Map, returns
to the map.
Save, on the File screen, writes one line per assignment:

```
sound <event> <path>
```

`event` is one of `jump`, `land`, `hurt`, `stomp`, `ray`, `goal`,
`checkpoint`, `powerup`, `derez`, `crouch`. `path` is relative to
`assets/circuit/audio/sfx/` and contains no spaces, such as
`Actions/Jump.wav` or `jump.wav`. A missing line keeps the built-in clip.

The Level window's music row cycles the files in
`assets/circuit/audio/music` (wav, mp3, or ogg). Click it again to move
to the next track, and once more after the last track to choose none.
Save writes `music <path>` on that level. Circuit plays level 1's track
in a loop. None stays silent.

## Files

The File button is in the upper left of the map. Right click and choose
File, or press P. Esc, or Map, returns to the map.

A game is a folder. The open game starts as `circuit`. Its levels are
files under `games/circuit/levels/`. The open file starts as
`games/circuit/levels/edit.lvl`. If that file is not on disk yet and
`levels/edit.lvl` is, the editor loads the older file into memory and
leaves it where it is. The next Save writes the game path.

The word `game` on the Files bar lists the folders under `games/`. Click
one to make it the open game. Type a name and Enter to create
`games/<name>/levels/`. A name is letters, digits, and a hyphen.

Opening the editor reads the open file into memory and does not write it.
Edit does the same for the file you click. Save is what writes the open
file. Save as asks for a name, then writes
`games/<game>/levels/<name>.lvl` and that file stays open. New asks for
a name, keeps every picture and its attributes, clears the map, and
writes that file. Delete removes the file you click. Enter stores a
typed name. Q still quits, and while the name line is active Q is a letter.

## Not in this editor

The cabinet does not simulate these keys. Circuit does. It reads gravity,
fall, run, jump, walk, wind, and leap, and it moves enemies from behavior.
Emit does not release an object during play. Slip is stored and not
applied. Hit points, defense, and offense are stored and do not remove an
enemy. The character animation is stored and Circuit draws the saved hero
sheet. Slopes and loops are a different game.
