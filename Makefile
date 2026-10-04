# Arcade — AILANG kernel + Gtk3 chrome.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.

AILANG ?= $(shell command -v ailang.x)
ROOT := $(CURDIR)

# One ailang.x at a time. These three binaries must not compile together.
.NOTPARALLEL: arcade_app.x circuit.x level_edit.x

.PHONY: all host kernel circuit editor run clean

all: host kernel circuit editor

host:
	$(MAKE) -C host

kernel: arcade_app.x

circuit: circuit.x

editor: level_edit.x

arcade_app.x: arcade_app.ailang App/*.ailang Game/*.ailang
	@test -n "$(AILANG)" || (echo "ailang.x not on PATH"; exit 1)
	$(AILANG) arcade_app.ailang arcade_app.x

circuit.x: Circuit/circuit.ailang Circuit/Body.ailang Circuit/View.ailang Circuit/Ipc.ailang
	@test -n "$(AILANG)" || (echo "ailang.x not on PATH"; exit 1)
	$(AILANG) Circuit/circuit.ailang circuit.x

level_edit.x: Editor/level_edit.ailang Editor/Edit.ailang Editor/Ui.ailang Editor/Ipc.ailang
	@test -n "$(AILANG)" || (echo "ailang.x not on PATH"; exit 1)
	$(AILANG) Editor/level_edit.ailang level_edit.x

run: all
	./scripts/run_arcade.sh

clean:
	$(MAKE) -C host clean
	rm -f arcade_app.x circuit.x level_edit.x a.out
