# Arcade — AILANG kernel + Gtk3 chrome.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.

AILANG ?= $(shell command -v ailang.x)
ROOT := $(CURDIR)

.PHONY: all host kernel run clean

all: host kernel

host:
	$(MAKE) -C host

kernel: arcade_app.x

arcade_app.x: arcade_app.ailang App/*.ailang Game/*.ailang
	@test -n "$(AILANG)" || (echo "ailang.x not on PATH"; exit 1)
	$(AILANG) arcade_app.ailang arcade_app.x

run: all
	./scripts/run_arcade.sh

clean:
	$(MAKE) -C host clean
	rm -f arcade_app.x a.out
