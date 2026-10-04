#!/usr/bin/env bash
# Level editor. Leaves the cabinet processes alone.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STATE=/dev/shm/level_edit
mkdir -p "$STATE"

if [[ -z "${DISPLAY:-}" ]]; then
  if [[ -S /tmp/.X11-unix/X0 ]]; then export DISPLAY=:0
  elif [[ -S /tmp/.X11-unix/X1 ]]; then export DISPLAY=:1
  else echo "ERROR: no DISPLAY"; exit 1
  fi
fi
if [[ -z "${XAUTHORITY:-}" && -f "$HOME/.Xauthority" ]]; then
  export XAUTHORITY="$HOME/.Xauthority"
fi
export XMODIFIERS="@im=none"
export GTK_IM_MODULE="gtk-im-context-simple"
export QT_IM_MODULE="simple"

if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
  make -C "$ROOT/host"
fi
if [[ ! -x "$ROOT/level_edit.x" ]]; then
  ailang.x "$ROOT/Editor/level_edit.ailang" "$ROOT/level_edit.x"
fi

# A previous editor only. The cabinet host uses a different directory.
pkill -x level_edit.x 2>/dev/null || true
while read -r pid rest; do
  case "$rest" in
    *"/dev/shm/level_edit"*) kill "$pid" 2>/dev/null || true ;;
  esac
done < <(pgrep -af 'arcade_shell_gtk' || true)
sleep 0.2

: > "$STATE/cmd.txt"
printf '0 0 0 0 0 0 0 0\n' > "$STATE/keys.txt"
printf '900 720\n' > "$STATE/size.txt"
printf '0\n' > "$STATE/gen.txt"

echo "Level editor. Blocks, Enemies, Objects, Entities, and Level are windows."
echo "Drag a title bar. Click a level field and type. Enter stores."
echo "Right click a map cell to clear it. Right click off the map to reopen a window."
echo "Right click and choose Sound to assign a clip. Save writes it into the level."

setsid "$ROOT/level_edit.x" >"$STATE/log.txt" 2>&1 < /dev/null &
APP_PID=$!
sleep 0.4
if ! kill -0 "$APP_PID" 2>/dev/null; then
  echo "ERROR: level_edit.x exited immediately"
  cat "$STATE/log.txt" || true
  exit 1
fi
exec "$ROOT/host/arcade_shell_gtk" "$STATE" "Level editor"
