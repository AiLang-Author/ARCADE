# Arcade

Arcade is a collection of fast 2D games made with the AILANG arcade engine.
**NOWAY HOME** is the first game; future games can share the same engine and
host.

## NOWAY HOME

A hive has captured a carrier near Earth and is using the fleet for food. You
are a fighter left behind. Defeat ten queens, recover the hyperdrive, and get
home.

The game includes:

- Side-to-side fighter movement and shooting
- Enemy formations, diving attacks, and formation fire
- Captures that can be reversed by destroying the captor
- Ten increasingly difficult queen battles
- Drive-part pickups, shields, extra lives, and score bonuses
- Music, sound effects, and an attract screen

![Title screen](noway-home/Intro.png)

![Gameplay](noway-home/gameplay.png)

![Queen battle](noway-home/bossbattle.png)

## How it works

The game runs in an AILANG process and displays through a small GTK host. The
AILANG side draws the game frame and handles the game logic; the host provides
the window, keyboard input, resizing, and audio support.

The playfield uses a square coordinate system and expands to fill the window.
Vector artwork is rasterized for the current window size, so sprites remain
sharp when the window is resized.

## Performance

On an AMD FX-8370 system with 64 GB of RAM at 3.2 GHz, the game typically uses
3–5% CPU and 9–20 MB of memory. Memory use depends on the window size; a
1024×1024 window typically uses 9–11 MB, while larger windows use more.

## Repository layout

```text
arcade_app.ailang     Application entry point
App/                  Shared engine code
Game/                 Game modules
assets/               Artwork, sound effects, and music
fonts/                Game fonts
host/                 GTK window and audio host
scripts/              Run and installation scripts
noway-home/           NOWAY HOME screenshots
```

## Run

```bash
./scripts/run_arcade.sh
```

You need `ailang.x` on your `PATH` and GTK3 installed. `ailang.x` is provided
by [Ailang-Self-Hosting](https://github.com/AiLang-Author/Ailang-Self-Hosting).

| Key | Action |
|-----|--------|
| ← → | Move |
| z / Space | Fire |
| Enter | Start or continue |
| p | Pause and settings |
| Esc / q | Quit |

To add a desktop launcher on Linux:

```bash
./scripts/install_desktop.sh
```

## Haiku

The Haiku window host is maintained in a separate repository:

https://github.com/AiLang-Author/Haiku-Arcade

Clone it next to this repository so its shared-file links can find `assets/`,
`fonts/`, and `arcade_app.x`. See that repository's instructions for building
and installing the Haiku version.

## Adding games

Arcade is intended to hold more than one game. A new game can be added under
`Game/` and can reuse the shared window, entity, formation, starfield, font, and
effect systems.

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
