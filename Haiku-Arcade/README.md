# Haiku-Arcade

Native **Haiku** front end for the Arcade shelf (NOWAY HOME first).

This is **not** the Linux GTK host. GTK stays in `host/arcade_shell_gtk`.
This folder is the Interface Kit blit window: `BBitmap` → `app_server`,
same `meta.bin` / `frame.raw` / `keys.txt` contract as CAD.

The CAD Haiku **installer did not** land in the first Arcade GitHub push —
only the Linux deskbar launcher (`scripts/launch_arcade.sh`) did. The
Haiku installer lives here: `install.sh`.

## Build (on Haiku)

```
g++ -O2 -o arcade_shell_haiku arcade_shell_haiku.cxx -lbe
# or
make
```

Needs `arcade_app.x` (Linux ELF kernel, built with `ailang.x` on Linux)
plus `sys_compat` / `sys_compat_run` from the AILang CAD Haiku drop so
the kernel can execute.

## Install (on Haiku)

```
cd Haiku-Arcade
sh install.sh
```

Deskbar leaf → **NOWAY HOME**. Copies kernel, assets, fonts, and the GUI
into `~/config/non-packaged/`. Reuses `sys_compat` if CAD is already
installed; otherwise drop `linux_abi/sys_compat` next to this README.

If `/dev/misc/sys_compat` is missing after install, reboot once.

## Keys

Same as Linux: arrows move, Z/Space fire, Enter start, P settings, Esc quit.

Audio (miniaudio/ALSA) is Linux-host only for now. The kernel still emits
`sfx.bin`; Haiku playback comes later.

Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
