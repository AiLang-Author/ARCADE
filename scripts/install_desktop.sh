#!/usr/bin/env bash
# Install NOWAY HOME to the Linux applications menu and Desktop.
# Prebuilt arcade_app.x is static. The GTK host needs shared libraries,
# and held keys need a readable /dev/input device. This script installs both.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

as_root() {
  if [[ "${EUID}" -eq 0 ]]; then
    "$@"
  elif command -v sudo >/dev/null; then
    sudo "$@"
  else
    echo "ERROR: root or sudo is required: $*"
    exit 1
  fi
}

apt_have() {
  apt-cache show "$1" 2>/dev/null | grep -q '^Package:'
}

pkg_installed() {
  local st
  st=$(dpkg-query -W -f='${Status}' "$1" 2>/dev/null || true)
  [[ "$st" == "install ok installed" ]]
}

first_pkg() {
  local p
  for p in "$@"; do
    if apt_have "$p"; then
      printf '%s\n' "$p"
      return 0
    fi
  done
  return 1
}

so_present() {
  ldconfig -p 2>/dev/null | awk -v so="$1" '$1 == so { found = 1 } END { exit !found }'
}

install_pkgs() {
  local missing=() p
  for p in "$@"; do
    [[ -n "$p" ]] || continue
    if ! pkg_installed "$p"; then
      missing+=("$p")
    fi
  done
  if [[ ${#missing[@]} -eq 0 ]]; then
    return 0
  fi
  echo "Installing: ${missing[*]}"
  as_root apt-get update
  as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y "${missing[@]}"
}

ensure_deps() {
  local pkgs=() gtk asound p
  if ! command -v apt-get >/dev/null; then
    echo "No apt-get. Install GTK 3, Cairo, libasound.so.2, and libpulse.so.0, then re-run."
    return 0
  fi
  gtk=$(first_pkg libgtk-3-0 libgtk-3-0t64 || true)
  asound=$(first_pkg libasound2 libasound2t64 || true)
  [[ -n "$gtk" ]] && pkgs+=("$gtk")
  if apt_have libcairo2; then pkgs+=(libcairo2); fi
  [[ -n "$asound" ]] && pkgs+=("$asound")
  if apt_have libpulse0; then pkgs+=(libpulse0); fi
  if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
    echo "Host binary missing. Installing the compiler and GTK headers."
    for p in g++ make pkg-config libgtk-3-dev libcairo2-dev; do
      if apt_have "$p"; then pkgs+=("$p"); fi
    done
  fi
  if [[ ${#pkgs[@]} -eq 0 ]]; then
    echo "ERROR: apt has no GTK 3 package for this host."
    exit 1
  fi
  install_pkgs "${pkgs[@]}"
}

verify_host_libs() {
  local missing
  missing=$(ldd "$ROOT/host/arcade_shell_gtk" | awk '/not found/ { print $1 }')
  if [[ -n "$missing" ]]; then
    echo "ERROR: host is still missing libraries:"
    printf '%s\n' "$missing"
    exit 1
  fi
  local so
  for so in libasound.so.2 libpulse.so.0; do
    if ! so_present "$so"; then
      echo "ERROR: audio library $so is not installed."
      exit 1
    fi
  done
}

ensure_input() {
  local ev readable=0
  for ev in /dev/input/event*; do
    [[ -e "$ev" ]] || continue
    if [[ -r "$ev" ]]; then
      readable=1
      break
    fi
  done
  if [[ "$readable" -eq 1 ]]; then
    return 0
  fi
  if ! getent group input >/dev/null; then
    echo "WARN: /dev/input is not readable and there is no input group."
    return 0
  fi
  if id -nG "${USER}" | tr ' ' '\n' | grep -qx input; then
    echo "WARN: ${USER} is in the input group, but it is not active in this session."
    echo "Log out and back in so held fire and movement keys work."
    return 0
  fi
  echo "Adding ${USER} to the input group so held keys can be read."
  as_root usermod -aG input "${USER}"
  echo "Log out and back in so held fire and movement keys work."
}

ensure_deps

if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
  echo "Building GTK host..."
  make -C "$ROOT/host"
fi
if [[ ! -x "$ROOT/arcade_app.x" ]]; then
  if ! command -v ailang.x >/dev/null; then
    echo "ERROR: arcade_app.x is not built, and ailang.x is not on PATH."
    echo "The kernel binary is static. To rebuild it, install Ailang-Self-Hosting and re-run."
    exit 1
  fi
  echo "Building kernel..."
  ailang.x "$ROOT/arcade_app.ailang" "$ROOT/arcade_app.x"
fi
chmod 755 "$ROOT/scripts/launch_arcade.sh" "$ROOT/arcade_app.x" "$ROOT/host/arcade_shell_gtk"
verify_host_libs
ensure_input

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
