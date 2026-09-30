# Arcade engine

## Two processes (CAD/Paint/ECU)

```
┌─────────────────────────────────────────────┐
│ arcade_shell_gtk   GtkWindow chrome only    │
│  menu + drawing area                        │
│  keys / size  →  /dev/shm/arcade_app/*.txt      │
│  XShm put     ←  frame0/frame1 + gen.txt    │
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

The host reads the finished slot and `XShmPutImage`s it onto the drawing
area on an 8 ms timer. GTK draws the menu and the status line. Cairo remains
the fallback when X shared memory is unavailable. The GTK frame clock is not
on the present path: waiting on it dropped frames at larger window sizes
while the kernel was still inside its 16 ms budget.

## Unit space

`World.UW × World.UH` = 1024×1024. The short side of the window is that
square, in device pixels, centered in the framebuffer. X and Y share the
scale, so a ship stays square at any window size. A craft is 56 units and
is rasterized from the vectors at `round(56 * span / 1024)` pixels. The
host copies the buffer 1:1 and does not stretch it. Entities stay in
units; blit converts at draw time. Resize does not rewrite positions.

## On-demand sprites

`Gfx.cell` is the craft's pixel size at the current window. Sheets are cached
by path and pixel size, so `Gfx_Reload` returns while the cell is unchanged.
When the cell changes, `Gfx_Reload`:

1. Try `assets/**/*.svg` via `SVG_RasterFile` (then `.tvg` via `TVG_RenderFile`).
2. If a file is missing, build a procedural strip at the same cell size.
3. Pack five views in one strip: front, back, top, bottom, side.
4. Boom sheets are 8-frame strips; `Boom_Spawn` picks one with `RNG.LCG_Range`.

## IPC (`/dev/shm/arcade_app`)

| File | Who | Meaning |
|------|-----|---------|
| `meta.bin` | kernel | int32 le width, height, pitch |
| `frame0.raw`, `frame1.raw` | kernel | BGRA, pitch×height. The kernel fills one while the host reads the other. |
| `gen.txt` | kernel | `count slot`. `slot` is the finished buffer. |
| `size.txt` | host | `W H` of the drawing area |
| `keys.txt` | host | `L R U D F S P` as 0/1 |
| `cmd.txt` | host | `quit` / `pause` / `start` |
| `prof.txt` | kernel | one timing line per second, average/max microseconds |
| `hprof.txt` | host | tick gap, idle, paint, and kernel frames not shown |

## Adding a game

`Import.Game.YourGame` from `arcade_app.ailang`, call `YourGame_Init` after
`Gfx_Reload`, `YourGame_Tick` in the loop. Reuse `Ent_*`, `Form_*`, `Boom_*`,
`Star_*`, `Hud_*`.
