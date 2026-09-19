#!/usr/bin/env bash
# Install NOWAY HOME to the Linux applications menu and Desktop.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
  echo "Building GTK host..."
  make -C "$ROOT/host"
fi
if [[ ! -x "$ROOT/arcade_app.x" ]]; then
  if ! command -v ailang.x >/dev/null; then
    echo "ERROR: arcade_app.x missing and ailang.x not on PATH"
    exit 1
  fi
  echo "Building kernel..."
  ailang.x "$ROOT/arcade_app.ailang" "$ROOT/arcade_app.x"
fi
chmod 755 "$ROOT/scripts/launch_arcade.sh" "$ROOT/arcade_app.x" "$ROOT/host/arcade_shell_gtk"

ICON="$ROOT/assets/hud/noway-home.svg"
[[ -f "$ICON" ]] || ICON="$ROOT/noway-home/Intro.png"
LAUNCH="$ROOT/scripts/launch_arcade.sh"
[[ -x "$LAUNCH" ]]
[[ -x "$ROOT/arcade_app.x" ]]
[[ -x "$ROOT/host/arcade_shell_gtk" ]]
[[ -f "$ROOT/assets/ships/player.svg" ]]
[[ -f "$ROOT/assets/enemies/bee.svg" ]]
[[ -d "$ROOT/assets/sfx" ]]

APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$APPS"
DESK="$APPS/noway-home.desktop"
cat > "$DESK" << EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=NOWAY HOME
GenericName=Arcade
Comment=NOWAY HOME — AILANG arcade fighter. Get home.
Exec=$LAUNCH
TryExec=$LAUNCH
Icon=$ICON
Terminal=false
Categories=Game;ArcadeGame;
Keywords=Arcade;Galaga;NOWAY;HOME;Ailang;
StartupNotify=true
StartupWMClass=arcade_shell_gtk
Path=$ROOT
EOF
chmod 755 "$DESK"

DESKTOP="${XDG_DESKTOP_DIR:-$HOME/Desktop}"
if [[ -d "$DESKTOP" ]]; then
  cp -f "$DESK" "$DESKTOP/noway-home.desktop"
  chmod 755 "$DESKTOP/noway-home.desktop"
  if command -v gio >/dev/null; then
    gio set "$DESKTOP/noway-home.desktop" metadata::trusted true 2>/dev/null || true
    if command -v sha256sum >/dev/null; then
      sum=$(sha256sum "$DESKTOP/noway-home.desktop" | awk '{print $1}')
      gio set -t string "$DESKTOP/noway-home.desktop" metadata::xfce-exe-checksum "$sum" 2>/dev/null || true
    fi
  fi
  echo "Desktop: $DESKTOP/noway-home.desktop"
fi

update-desktop-database "$APPS" 2>/dev/null || true
echo "Menu:     $DESK"
echo "Launch:   $LAUNCH"
echo "Kernel:   $ROOT/arcade_app.x"
echo "Host:     $ROOT/host/arcade_shell_gtk"
echo "OK — Applications → Games → NOWAY HOME"
