#!/usr/bin/env bash
# Launch arcade_app.x + thin Gtk blit host.
# Playfield is the drawing area. Kernel rasters SVG/TVG at that size.
#
#   ./scripts/run_arcade.sh
#
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STATE="${ARCADE_APP_STATE:-/dev/shm/arcade_app}"
mkdir -p "$STATE"
pkill -x arcade_app.x 2>/dev/null || true
pkill -x arcade_shell_gtk 2>/dev/null || true
sleep 0.2
: > "$STATE/cmd.txt"
: > "$STATE/keys.txt"
: > "$STATE/size.txt"
printf '0\n' > "$STATE/gen.txt"

if [[ -z "${DISPLAY:-}" ]]; then
  if [[ -S /tmp/.X11-unix/X0 ]]; then export DISPLAY=:0
  elif [[ -S /tmp/.X11-unix/X1 ]]; then export DISPLAY=:1
  else echo "ERROR: no DISPLAY"; exit 1
  fi
fi
if [[ -z "${XAUTHORITY:-}" && -f "$HOME/.Xauthority" ]]; then
  export XAUTHORITY="$HOME/.Xauthority"
fi
export XMODIFIERS="${XMODIFIERS:-@im=none}"
export GTK_IM_MODULE="${GTK_IM_MODULE:-}"

echo "run_arcade: building gtk host..."
make -C host
echo "run_arcade: building arcade_app..."
ailang.x arcade_app.ailang arcade_app.x

./arcade_app.x &
APP_PID=$!
sleep 0.4
if ! kill -0 "$APP_PID" 2>/dev/null; then
  echo "ERROR: arcade_app.x exited immediately"
  exit 1
fi
./host/arcade_shell_gtk "$STATE"
kill "$APP_PID" 2>/dev/null || true
wait "$APP_PID" 2>/dev/null || true
