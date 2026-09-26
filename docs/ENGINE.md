# Arcade engine

## Two processes (CAD/Paint/ECU)

```
┌─────────────────────────────────────────────┐
│ arcade_shell_gtk   GtkWindow chrome only    │
│  menu + drawing area                        │
│  keys / size  →  /dev/shm/arcade_app/*.txt      │
│  Cairo blit   ←  frame.raw + gen.txt        │
└────────────────────┬────────────────────────┘
                     │
┌────────────────────▼────────────────────────┐
│ arcade_app.x   AILANG kernel                │
│  FB = window size (headless mmap today,     │
│       /dev/fb0 on AOS tomorrow)             │
│  SVG/TVG → PIXEL_32 sheet → blit            │
│  VFont HUD, formation, boom RNG, Galaga     │
└─────────────────────────────────────────────┘
```

AOS path: skip the Gtk host, `FB_Init` instead of `FB_InitHeadless`, same
`Draw_*` / `Ent_*` / `Galaga_*`. Chrome is a window head, not the game.

## Unit space

`World.UW × World.UH` = 1024×1024. That square is rasterized at
`k × 1024` device pixels (`k` = 1, 2, … as the window allows) and centered
in the framebuffer. X and Y share that scale, so a unit lands on a pixel.
Sprites are rasterized from the vectors at that cell size. The host copies
the buffer 1:1 in device pixels and does not stretch it. Entities stay in
units; blit converts at draw time. Resize does not rewrite positions.

## On-demand sprites

`Gfx.cell = max(12, min(fw,fh) / 18)`. When that changes, `Gfx_Reload`:

1. Try `assets/**/*.svg` via `SVG_RasterFile` (then `.tvg` via `TVG_RenderFile`).
2. If a file is missing, build a procedural strip at the same cell size.
3. Pack five views in one strip: front, back, top, bottom, side.
4. Boom sheets are 8-frame strips; `Boom_Spawn` picks one with `RNG.LCG_Range`.

## IPC (`/dev/shm/arcade_app`)

| File | Who | Meaning |
|------|-----|---------|
| `meta.bin` | kernel | int32 le width, height, pitch |
| `frame.raw` | kernel | BGRA, pitch×height |
| `gen.txt` | kernel | frame generation |
| `size.txt` | host | `W H` of the drawing area |
| `keys.txt` | host | `L R U D F S P` as 0/1 |
| `cmd.txt` | host | `quit` / `pause` / `start` |

## Adding a game

`Import.Game.YourGame` from `arcade_app.ailang`, call `YourGame_Init` after
`Gfx_Reload`, `YourGame_Tick` in the loop. Reuse `Ent_*`, `Form_*`, `Boom_*`,
`Star_*`, `Hud_*`.
