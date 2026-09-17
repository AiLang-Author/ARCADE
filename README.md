# Arcade

AILANG house for **fast 2D games**. GTK window, kernel owns the pixels. First
title: **NOWAY HOME**.

---

# NOWAY HOME

The hive tore a hole near Earth, swallowed a carrier, and farms the fleet as
food. You are a fighter off that carrier. Ten queens hold the hyperdrive. Get
home.

Title track: **Turn Us Around** (Suno, original lyrics). Queen theme: **Queen Ship
Protocol**. Lyric crawl on the idle rift.

![Title](noway-home/Intro.png)
![Play](noway-home/gameplay.png)
![Queen](noway-home/bossbattle.png)

AILANG arcade engine with a GTK chrome window. Formation lines, dive attacks,
and RNG explosion clips.

Same contract as CAD, Paint, ECU dash, and HalCodeGTK:

- **Kernel** (`arcade_app.x`) owns the playfield, sprites, fonts, and game.
- **Host** (`host/arcade_shell_gtk`) is native chrome + Cairo blit + keys/resize.
- Pixels travel as CAD’s `meta.bin` + `frame.raw` + `gen.txt`.
- Later the same kernel presents straight to `/dev/fb0` on AOS — swap the window
  head, keep the engine.

## Playfield = the window

There is no fixed internal resolution. The kernel framebuffer is the drawing
area. Unit space is `1024×1024`, stretched to the full window (portrait or
landscape). SVG/TVG craft are rasterized **on demand** at a cell size derived
from the current window, then blitted from a view strip (front / back / top /
bottom / side).

Resize the window → kernel picks a new cell → vectors re-raster → sprites stay
sharp. No baked 32×32 sheets.

## Layout

```
arcade_app.ailang     thin Main
App/                  engine (window, ipc, entities, sheets, formation, boom, hud)
Game/Galaga.ailang    first title — more games plug in the same way
assets/               SVG source (TVG siblings optional)
fonts/                AlteixSans.vif + DejaVuSans.vif (native VFont)
host/                 Gtk3 blit chrome
```

## Run

```bash
./scripts/run_arcade.sh
```

Needs `ailang.x` on `PATH` (from Ailang-Self-Hosting `install_compiler.sh`) and Gtk3.

| Key | Action |
|-----|--------|
| ← → | move |
| z / space | fire |
| Enter | start / next wave |
| p | pause / settings (music + SFX volume) |
| Esc / q | quit |

Deskbar / desktop: `scripts/launch_arcade.sh` (also `noway-home.desktop`).

## Screenshots

Shots live in `noway-home/` (this game’s folder). Later titles get their own.

| File | Shot |
|---|---|
| `noway-home/Intro.png` | attract — NOWAY HOME + lyric crawl |
| `noway-home/gameplay.png` | stage |
| `noway-home/bossbattle.png` | queen fight |

## Next games

A title is `Game/<Name>.ailang` that implements `Name_Init` / `Name_Start` /
`Name_Tick`. The engine already has window-sized playfield, on-demand vector
sprites, entity pool, formation slots, starfield, HUD fonts, and boom RNG.
1942, Xevious, Gyruss, … drop in as another `Game/` module.

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
