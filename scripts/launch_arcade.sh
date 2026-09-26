#!/usr/bin/env bash
# Deskbar / desktop launcher. Does not rebuild unless binaries are missing.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STATE="${ARCADE_APP_STATE:-/dev/shm/arcade_app}"
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
# IBus eats a held fire key once repeat starts. Do not keep the session IM.
export XMODIFIERS="@im=none"
export GTK_IM_MODULE="gtk-im-context-simple"
export QT_IM_MODULE="simple"

if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
  make -C "$ROOT/host"
fi
if ldd "$ROOT/host/arcade_shell_gtk" | grep -q 'not found'; then
  echo "ERROR: GTK host is missing libraries. Run ./scripts/install_desktop.sh"
  exit 1
fi
if [[ ! -x "$ROOT/arcade_app.x" ]]; then
  ailang.x "$ROOT/arcade_app.ailang" "$ROOT/arcade_app.x"
fi

pkill -x arcade_app.x 2>/dev/null || true
pkill -x arcade_shell_gtk 2>/dev/null || true
sleep 0.2
: > "$STATE/cmd.txt"
printf '0 0 0 0 0 0 0\n' > "$STATE/keys.txt"
printf '900 720\n' > "$STATE/size.txt"
printf '0\n' > "$STATE/gen.txt"

setsid "$ROOT/arcade_app.x" >/dev/null 2>&1 < /dev/null &
APP_PID=$!
sleep 0.4
if ! kill -0 "$APP_PID" 2>/dev/null; then
  echo "ERROR: arcade_app.x exited immediately"
  exit 1
fi
exec "$ROOT/host/arcade_shell_gtk" "$STATE"
