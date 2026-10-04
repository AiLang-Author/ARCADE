#!/usr/bin/env bash
# Circuit viewer. Leaves the cabinet and the level editor alone.
# Copyright (c) 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STATE=/dev/shm/circuit
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
if [[ ! -x "$ROOT/circuit.x" ]]; then
  ailang.x "$ROOT/Circuit/circuit.ailang" "$ROOT/circuit.x"
fi

# A previous circuit only. Cabinet shm and the editor shm stay up.
pkill -x circuit.x 2>/dev/null || true
while read -r pid rest; do
  case "$rest" in
    *"/dev/shm/circuit"*) kill "$pid" 2>/dev/null || true ;;
  esac
done < <(pgrep -af 'arcade_shell_gtk' || true)
sleep 0.2

: > "$STATE/cmd.txt"
printf '0 0 0 0 0 0 0 0 0\n' > "$STATE/keys.txt"
printf '900 720\n' > "$STATE/size.txt"
printf '0\n' > "$STATE/gen.txt"

echo "Circuit. Arrows walk. Hold one to run. Hold Enter for turbo. Up or fire jumps. Down crouches. X fires. Q or Esc leaves the picture. Close the window to exit."

setsid "$ROOT/circuit.x" >"$STATE/log.txt" 2>&1 < /dev/null &
APP_PID=$!
sleep 0.4
if ! kill -0 "$APP_PID" 2>/dev/null; then
  echo "ERROR: circuit.x exited immediately"
  cat "$STATE/log.txt" || true
  exit 1
fi
"$ROOT/host/arcade_shell_gtk" "$STATE" "Circuit" >"$STATE/host.log" 2>&1 &
HOST_PID=$!
while kill -0 "$APP_PID" 2>/dev/null && kill -0 "$HOST_PID" 2>/dev/null; do
  sleep 0.3
done
kill "$APP_PID" "$HOST_PID" 2>/dev/null || true
wait "$APP_PID" 2>/dev/null || true
wait "$HOST_PID" 2>/dev/null || true
