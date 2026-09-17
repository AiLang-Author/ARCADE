#!/bin/sh
# NOWAY HOME / Arcade install for Haiku.
# Native GUI + Linux ELF kernel via sys_compat + Deskbar leaf.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
set -e
HERE=`dirname "$0"`
cd "$HERE"
HERE=`pwd`
ROOT=`dirname "$HERE"`

APPDIR="/boot/home/config/non-packaged/apps"
BINDIR="/boot/home/config/non-packaged/bin"
DATDIR="/boot/home/config/non-packaged/data/ailang_arcade"
DRVBIN="/boot/home/config/non-packaged/add-ons/kernel/drivers/bin"
DRVMISC="/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc"
SYSBIN="/boot/system/non-packaged/add-ons/kernel/drivers/bin"
SYSMISC="/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc"
MENU="/boot/home/config/non-packaged/data/deskbar/menu/Applications"
LEAF="/boot/home/config/settings/deskbar/menu"

find_file() {
	for p in "$@"; do
		if [ -f "$p" ]; then
			echo "$p"
			return 0
		fi
	done
	return 1
}

echo "=== NOWAY HOME / Arcade Haiku install ==="

DRV=`find_file \
	"$HERE/linux_abi/sys_compat" \
	"$HERE/../linux_abi/sys_compat" \
	"/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/sys_compat" \
	"/boot/system/non-packaged/add-ons/kernel/drivers/bin/sys_compat"` || {
	echo "WARN: sys_compat driver not found."
	echo "      Install AILang CAD Haiku first, or drop linux_abi/sys_compat here."
	DRV=""
}
RUN=`find_file \
	"$HERE/linux_abi/sys_compat_run" \
	"$HERE/bin/sys_compat_run" \
	"/boot/home/sys_compat_run" \
	"/boot/home/config/non-packaged/bin/sys_compat_run"` || {
	echo "WARN: sys_compat_run not found (needed to exec arcade_app.x)."
	RUN=""
}
KERN=`find_file \
	"$HERE/bin/arcade_app.x" \
	"$ROOT/arcade_app.x"` || {
	echo "FAIL: arcade_app.x not in this tree (build the kernel on Linux first)."
	exit 1
}
GUI=`find_file \
	"$HERE/arcade_shell_haiku" \
	"$HERE/apps/NOWAY HOME"` || {
	echo "FAIL: arcade_shell_haiku not built. On Haiku: make -C Haiku-Arcade"
	exit 1
}

if [ -n "$DRV" ]; then
	echo "--- Linux ABI (sys_compat) ---"
	mkdir -p "$DRVBIN" "$DRVMISC" "$BINDIR"
	cp -f "$DRV" "$DRVBIN/sys_compat"
	chmod 755 "$DRVBIN/sys_compat"
	ln -sfn "$DRVBIN/sys_compat" "$DRVMISC/sys_compat"
	mimeset -f "$DRVBIN/sys_compat" 2>/dev/null || true
	if mkdir -p "$SYSBIN" "$SYSMISC" 2>/dev/null; then
		cp -f "$DRV" "$SYSBIN/sys_compat"
		chmod 755 "$SYSBIN/sys_compat"
		ln -sfn "$SYSBIN/sys_compat" "$SYSMISC/sys_compat"
		echo "  driver: $SYSBIN/sys_compat"
	fi
	echo "  driver: $DRVBIN/sys_compat"
fi

if [ -n "$RUN" ]; then
	mkdir -p "$BINDIR"
	cp -f "$RUN" "$BINDIR/sys_compat_run"
	cp -f "$RUN" /boot/home/sys_compat_run
	chmod 755 "$BINDIR/sys_compat_run" /boot/home/sys_compat_run
	echo "  runner: /boot/home/sys_compat_run"
fi

mkdir -p /dev/shm 2>/dev/null || true

echo "--- Arcade app ---"
mkdir -p "$APPDIR" "$DATDIR" "$DATDIR/assets" "$DATDIR/fonts" "$MENU" "$LEAF"
cp -f "$GUI" "$APPDIR/NOWAY HOME"
chmod 755 "$APPDIR/NOWAY HOME"
mimeset -f "$APPDIR/NOWAY HOME" 2>/dev/null || true
cp -f "$KERN" "$DATDIR/arcade_app.x"
cp -f "$KERN" /boot/home/arcade_app.x
chmod 755 "$DATDIR/arcade_app.x" /boot/home/arcade_app.x

if [ -d "$ROOT/assets" ]; then
	cp -a "$ROOT/assets/." "$DATDIR/assets/"
fi
if [ -d "$ROOT/fonts" ]; then
	cp -a "$ROOT/fonts/." "$DATDIR/fonts/"
fi

ln -sfn "$APPDIR/NOWAY HOME" "$MENU/NOWAY HOME"
ln -sfn "$APPDIR/NOWAY HOME" "$LEAF/NOWAY HOME"
echo "  app: $APPDIR/NOWAY HOME"
echo "  data: $DATDIR"

if [ -e /dev/misc/sys_compat ]; then
	echo "  /dev/misc/sys_compat is present"
else
	echo "  /dev/misc/sys_compat not up yet — reboot once after this install."
fi

echo
echo "Installed. Deskbar leaf → NOWAY HOME"
echo "  Kernel is a Linux ELF; sys_compat must be loaded."
