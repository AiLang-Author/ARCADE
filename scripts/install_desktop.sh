#!/usr/bin/env bash
# Install ARCADE and the level editor to the Linux applications menu and Desktop.
# The three AILANG programs are static. The GTK host needs shared libraries,
# and held keys need a readable /dev/input device. This script installs them.
# It builds a missing binary and does not open a window.
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

# One compiler at a time. A second ailang.x beside the first fills the machine.
build_one() {
  local src="$1" out="$2" label="$3"
  if [[ -x "$out" ]]; then
    return 0
  fi
  if ! command -v ailang.x >/dev/null; then
    echo "ERROR: $label is not built, and ailang.x is not on PATH."
    echo "The program is static. Install Ailang-Self-Hosting and re-run."
    exit 1
  fi
  echo "Building $label..."
  ailang.x "$src" "$out"
}

if [[ ! -x "$ROOT/host/arcade_shell_gtk" ]]; then
  echo "Building GTK host..."
  make -C "$ROOT/host"
fi
build_one "$ROOT/arcade_app.ailang" "$ROOT/arcade_app.x" "cabinet"
build_one "$ROOT/Circuit/circuit.ailang" "$ROOT/circuit.x" "Circuit"
build_one "$ROOT/Editor/level_edit.ailang" "$ROOT/level_edit.x" "level editor"
chmod 755 \
  "$ROOT/scripts/launch_arcade.sh" \
  "$ROOT/scripts/run_circuit.sh" \
  "$ROOT/scripts/run_editor.sh" \
  "$ROOT/arcade_app.x" \
  "$ROOT/circuit.x" \
  "$ROOT/level_edit.x" \
  "$ROOT/host/arcade_shell_gtk"
verify_host_libs
ensure_input

if [[ ! -e "$ROOT/assets/circuit" ]]; then
  echo "WARN: assets/circuit does not reach an art pack."
  echo "      The link in a GitHub clone points at the machine where the city was built."
  echo "      Point it at your copy: ln -sfn /path/to/circuit-runner-assets assets/circuit"
  echo "      NOWAY HOME and GYRE do not need that pack. The cabinet still installs."
fi

ICON="$ROOT/assets/hud/icons/hicolor/512x512/apps/arcade-cabinet.png"
[[ -f "$ICON" ]] || ICON="$ROOT/assets/hud/arcade.png"
[[ -f "$ICON" ]] || ICON="$ROOT/assets/hud/noway-home.svg"
[[ -f "$ICON" ]] || ICON="$ROOT/noway-home/Intro.png"
LAUNCH="$ROOT/scripts/launch_arcade.sh"
EDIT="$ROOT/scripts/run_editor.sh"
[[ -x "$LAUNCH" ]]
[[ -x "$EDIT" ]]
[[ -x "$ROOT/scripts/run_circuit.sh" ]]
[[ -x "$ROOT/arcade_app.x" ]]
[[ -x "$ROOT/circuit.x" ]]
[[ -x "$ROOT/level_edit.x" ]]
[[ -x "$ROOT/host/arcade_shell_gtk" ]]
[[ -f "$ROOT/assets/ships/player.svg" ]]
[[ -f "$ROOT/assets/enemies/bee.svg" ]]
[[ -d "$ROOT/assets/sfx" ]]

write_entry() {
  local dest="$1" name="$2" generic="$3" comment="$4" execp="$5" categories="$6" keywords="$7"
  cat > "$dest" << EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=$name
GenericName=$generic
Comment=$comment
Exec=$execp
TryExec=$execp
Icon=$ICON
Terminal=false
Categories=$categories
Keywords=$keywords
StartupNotify=true
StartupWMClass=arcade_shell_gtk
Path=$ROOT
EOF
  chmod 755 "$dest"
}

trust_entry() {
  local dest="$1"
  if ! command -v gio >/dev/null; then
    return 0
  fi
  gio set "$dest" metadata::trusted true 2>/dev/null || true
  if command -v sha256sum >/dev/null; then
    sum=$(sha256sum "$dest" | awk '{print $1}')
    gio set -t string "$dest" metadata::xfce-exe-checksum "$sum" 2>/dev/null || true
  fi
}

APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$APPS"
DESK="$APPS/arcade.desktop"
EDITOR_DESK="$APPS/arcade-level-editor.desktop"
write_entry "$DESK" "ARCADE" "Arcade" \
  "ARCADE — AILANG cabinet. NOWAY HOME, GYRE, and CIRCUIT." \
  "$LAUNCH" "Game;ArcadeGame;" "Arcade;NOWAY;HOME;GYRE;CIRCUIT;Ailang;"
write_entry "$EDITOR_DESK" "ARCADE Level Editor" "Level editor" \
  "Edit side-scroller levels. Circuit's city is games/circuit/levels/edit.lvl." \
  "$EDIT" "Game;Development;" "Arcade;Circuit;Level;Editor;Ailang;"
rm -f "$APPS/noway-home.desktop"

DESKTOP="${XDG_DESKTOP_DIR:-$HOME/Desktop}"
if [[ -d "$DESKTOP" ]]; then
  cp -f "$DESK" "$DESKTOP/arcade.desktop"
  cp -f "$EDITOR_DESK" "$DESKTOP/arcade-level-editor.desktop"
  chmod 755 "$DESKTOP/arcade.desktop" "$DESKTOP/arcade-level-editor.desktop"
  rm -f "$DESKTOP/noway-home.desktop"
  trust_entry "$DESKTOP/arcade.desktop"
  trust_entry "$DESKTOP/arcade-level-editor.desktop"
  echo "Desktop: $DESKTOP/arcade.desktop"
  echo "Desktop: $DESKTOP/arcade-level-editor.desktop"
fi

update-desktop-database "$APPS" 2>/dev/null || true
echo "Menu:     $DESK"
echo "Menu:     $EDITOR_DESK"
echo "Launch:   $LAUNCH"
echo "Editor:   $EDIT"
echo "Cabinet:  $ROOT/arcade_app.x"
echo "Circuit:  $ROOT/circuit.x"
echo "Editor x: $ROOT/level_edit.x"
echo "Host:     $ROOT/host/arcade_shell_gtk"
echo "OK — Applications → Games → ARCADE"
echo "OK — Applications → Games → ARCADE Level Editor"
