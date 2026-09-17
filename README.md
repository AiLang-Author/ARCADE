# Arcade

This repo is the shelf for **fun, fast 2D games** built on the AILANG arcade
engine (GTK window, kernel owns the pixels). **NOWAY HOME is the first title.**
More games go here the same way — new `Game/` module, same host.

---

# NOWAY HOME *(game 1)*

The hive tore a hole near Earth, swallowed a carrier, and farms the fleet as
food. You are a fighter off that carrier. Ten queens hold the hyperdrive. Get
home.

Title track: **Turn Us Around** (Suno, original lyrics). Queen theme: **Queen Ship
Protocol**. Lyric crawl on the idle rift.

**Attract**

![Title](noway-home/Intro.png)

**Stage**

![Play](noway-home/gameplay.png)

**Queen**

![Queen](noway-home/bossbattle.png)

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

## Next games

Arcade is not a one-game repo. NOWAY HOME is **the first**. The next title is
`Game/<Name>.ailang` with `Name_Init` / `Name_Start` / `Name_Tick`. The engine
already has window-sized playfield, on-demand vector sprites, entity pool,
formation slots, starfield, HUD fonts, and boom RNG. 1942, Xevious, Gyruss, …
drop in as another module. Each game can keep shots in its own folder
(`noway-home/` for this one).

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
