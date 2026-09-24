# Arcade Gtk3 blit host. Same header tree as CAD/Paint/ECU/HalCodeGTK.
# Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.

AILANG_ROOT ?= $(HOME)/Ailang-Self-Hosting-
GTKH := $(AILANG_ROOT)/third_party/gtk3-headers
GLIB_INC := /usr/include/glib-2.0
GLIB_GEN := /usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBDIR := /usr/lib/x86_64-linux-gnu

PKG := $(shell pkg-config --exists gtk+-3.0 cairo && echo yes)

ifeq ($(PKG),yes)
  GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 cairo)
  GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 cairo)
else
  GTK_CFLAGS := \
	-I$(GTKH)/usr/include/gtk-3.0 \
	-I$(GTKH)/usr/include/cairo \
	-I$(GTKH)/usr/include/pango-1.0 \
	-I$(GTKH)/usr/include/gdk-pixbuf-2.0 \
	-I$(GTKH)/usr/include/atk-1.0 \
	-I$(GTKH)/usr/include/harfbuzz \
	-I$(GTKH)/usr/include/fribidi \
	-I$(GTKH)/usr/include/freetype2 \
	-I$(GTKH)/usr/include \
	-I$(GLIB_INC) \
	-I$(GLIB_GEN)
  GTK_LIBS := \
	$(LIBDIR)/libgtk-3.so.0 \
	$(LIBDIR)/libgdk-3.so.0 \
	$(LIBDIR)/libcairo.so.2 \
	$(LIBDIR)/libpango-1.0.so.0 \
	$(LIBDIR)/libpangocairo-1.0.so.0 \
	$(LIBDIR)/libgdk_pixbuf-2.0.so.0 \
	-lgobject-2.0 -lglib-2.0 -lm
endif

.PHONY: all clean

all: arcade_shell_gtk

arcade_shell_gtk: arcade_shell_gtk.cxx audio.cc audio.h miniaudio.h
	g++ -O2 -std=c++17 -o $@ arcade_shell_gtk.cxx audio.cc $(GTK_CFLAGS) $(GTK_LIBS) -lpthread -ldl -lm

clean:
	rm -f arcade_shell_gtk
