# Arcade

# Large-window stutter fixed

Ships jumped when the window grew, and the game felt slow. The kernel was
still finishing a frame in about 6 ms and ticking at 60 Hz. The GTK host was
handing that frame to Cairo on the toolkit frame clock, and the clock stopped
calling the redraw for a few hundred milliseconds at a time, so the window
skipped those frames. The window is only the wrapper. The kernel's finished
buffer is put on the drawing area with X shared memory, 1:1, on an 8 ms timer.

Arcade is a collection of fast, lightweight 2D games made with the AILANG
programming language. **NOWAY HOME** is the first game, with more titles
planned for the same engine and host.

AILANG compiles to bare-metal x86_64 using a full self-hosting compiler written
from the ground up for optimized code generation. The language and toolchain
are designed to make programs easy to read, reason about, and generate with the
help of large language models.

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

The game is written in AILANG and compiled for x86_64. A small GTK host
provides the desktop window, keyboard input, resizing, and audio support,
while the compiled game handles the game logic and drawing.

The playfield is a square raster in the window's device pixels. The short side
of the window is the 1024-unit square, centered. Vector artwork is rasterized
once at that size, cached, and reused until the window is resized. The host
puts that buffer on the window 1:1 with X shared memory, so a larger window
stays on the pixel grid.



## Performance

In testing on an AMD FX-8370 system with 64 GB of RAM at 3.2 GHz, the game used
3–5% CPU and 9–20 MB of memory. A 1024×1024 window typically used 9–11 MB;
larger windows use more memory because the framebuffer grows with the window
size. A later look on a 1280×1024 panel showed the remaining stutter was the
host dropping frames, not the kernel falling behind: after the shared-memory
put, a 1280×923 window kept every kernel frame.

## Development

The game engine and NOWAY HOME were developed from start to finish in two work
days, followed by a week of post-production testing and a final four-hour
session of tweaks and improvements based on feedback.

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

`./scripts/install_desktop.sh` uses the prebuilt `arcade_app.x` (static) and
`host/arcade_shell_gtk`. On Debian, Ubuntu, and Pop it installs the GTK 3,
Cairo, ALSA, and PulseAudio libraries that host needs, and adds your user to
the `input` group when `/dev/input` is not readable. Log out once if it says
the group is not active yet.

Rebuilding the kernel needs `ailang.x` on your `PATH`, from
[Ailang-Self-Hosting](https://github.com/AiLang-Author/Ailang-Self-Hosting).
Rebuilding the host needs `g++`, `make`, `pkg-config`, and the GTK 3 headers.
The installer adds those compiler packages when the host binary is absent.

| Key | Action |
|-----|--------|
| ← → | Move |
| z / Space | Fire |
| Enter | Start or continue |
| p | Pause and settings |
| Esc / q | Quit |

To install the menu and desktop launcher, and the libraries it needs:

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

Arcade is designed to support more than one game. A new title can be added
under `Game/` and reuse the shared window, entity, formation, starfield, font,
and effect systems.

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
