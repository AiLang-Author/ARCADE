/* arcade_shell_gtk — thin Gtk3 chrome. Kernel owns the playfield.
 *
 * Drawing area size IS the playfield. Kernel rasters SVG/TVG at that size.
 * Chrome: menu + blit + keys. No pixels stamped here.
 *
 *   ARCADE_APP_STATE=/dev/shm/arcade_app ./host/arcade_shell_gtk
 *
 * Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
 */
#include <gtk/gtk.h>
#include <cairo.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "audio.h"

static char dir[512];
static char path_meta[600], path_frame[600], path_gen[600];
static char path_cmd[600], path_keys[600], path_size[600], path_sfx[600], path_vol[600];

static GtkWidget *win, *draw_area, *status;
static cairo_surface_t *surf;
static int last_gen = -1, fw = 800, fh = 600;
static int k_l, k_r, k_u, k_d, k_f, k_s, k_p;
static int last_dw, last_dh;

static int le32(const uint8_t *p) {
    return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static void write_file(const char *path, const char *s) {
    char tmp[600];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fputs(s, f);
    if (s[0] && s[strlen(s) - 1] != '\n') fputc('\n', f);
    fclose(f);
    rename(tmp, path);
}

static void write_cmd(const char *s) {
    write_file(path_cmd, s);
}

static void write_keys(void) {
    char buf[64];
    snprintf(buf, sizeof buf, "%d %d %d %d %d %d %d\n",
             k_l, k_r, k_u, k_d, k_f, k_s, k_p);
    write_file(path_keys, buf);
}

static void write_size(int w, int h) {
    if (w < 1 || h < 1) return;
    char buf[64];
    snprintf(buf, sizeof buf, "%d %d\n", w, h);
    write_file(path_size, buf);
}

static int read_gen(void) {
    FILE *f = fopen(path_gen, "r");
    if (!f) return -1;
    int g = -1;
    if (fscanf(f, "%d", &g) != 1) g = -1;
    fclose(f);
    return g;
}

static int load_frame(void) {
    int fd = open(path_meta, O_RDONLY);
    if (fd < 0) return 0;
    uint8_t hdr[16];
    ssize_t nr = read(fd, hdr, 16);
    close(fd);
    if (nr < 12) return 0;
    int w = le32(hdr + 0);
    int h = le32(hdr + 4);
    int p = le32(hdr + 8);
    if (w < 16 || h < 16 || w > 4096 || h > 4096) return 0;
    fw = w;
    fh = h;
    fd = open(path_frame, O_RDONLY);
    if (fd < 0) return 0;
    if (!surf || cairo_image_surface_get_width(surf) != w ||
        cairo_image_surface_get_height(surf) != h) {
        if (surf) cairo_surface_destroy(surf);
        surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    }
    uint8_t *dst = cairo_image_surface_get_data(surf);
    int dp = cairo_image_surface_get_stride(surf);
    int ok = 1;
    if (dp == p) {
        size_t n = (size_t)p * (size_t)h;
        if (read(fd, dst, n) != (ssize_t)n) ok = 0;
    } else {
        for (int y = 0; y < h && ok; y++) {
            if (read(fd, dst + y * dp, (size_t)w * 4) != (ssize_t)w * 4) ok = 0;
        }
    }
    close(fd);
    if (!ok) return 0;
    cairo_surface_mark_dirty(surf);
    gtk_widget_queue_draw(draw_area);
    return 1;
}

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer) {
    GtkAllocation a;
    gtk_widget_get_allocation(w, &a);
    cairo_set_source_rgb(cr, 0.02, 0.04, 0.10);
    cairo_paint(cr);
    if (!surf || fw < 1 || fh < 1 || a.width < 1 || a.height < 1) return FALSE;
    /* Playfield is the window: stretch the kernel frame to the drawing area. */
    cairo_scale(cr, (double)a.width / (double)fw, (double)a.height / (double)fh);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    return FALSE;
}

static gboolean on_configure(GtkWidget *w, GdkEventConfigure *, gpointer) {
    GtkAllocation a;
    gtk_widget_get_allocation(w, &a);
    if (a.width != last_dw || a.height != last_dh) {
        last_dw = a.width;
        last_dh = a.height;
        write_size(a.width, a.height);
    }
    return FALSE;
}

static int map_key(guint kv, int on) {
    int *slot = NULL;
    if (kv == GDK_KEY_Left || kv == GDK_KEY_a || kv == GDK_KEY_A) slot = &k_l;
    else if (kv == GDK_KEY_Right || kv == GDK_KEY_d || kv == GDK_KEY_D) slot = &k_r;
    else if (kv == GDK_KEY_Up || kv == GDK_KEY_w || kv == GDK_KEY_W) slot = &k_u;
    else if (kv == GDK_KEY_Down || kv == GDK_KEY_s || kv == GDK_KEY_S) slot = &k_d;
    else if (kv == GDK_KEY_z || kv == GDK_KEY_Z || kv == GDK_KEY_space) slot = &k_f;
    else if (kv == GDK_KEY_Return || kv == GDK_KEY_KP_Enter) slot = &k_s;
    else if (kv == GDK_KEY_p || kv == GDK_KEY_P) slot = &k_p;
    else return 0;
    *slot = on ? 1 : 0;
    write_keys();
    return 1;
}

static gboolean on_key(GtkWidget *, GdkEventKey *e, gpointer) {
    if (e->keyval == GDK_KEY_Escape || e->keyval == GDK_KEY_q || e->keyval == GDK_KEY_Q) {
        write_cmd("quit");
        gtk_main_quit();
        return TRUE;
    }
    if (map_key(e->keyval, 1)) return TRUE;
    return FALSE;
}

static gboolean on_key_up(GtkWidget *, GdkEventKey *e, gpointer) {
    if (map_key(e->keyval, 0)) return TRUE;
    return FALSE;
}

static gboolean on_tick(gpointer) {
    int g = read_gen();
    if (g != last_gen && g >= 0) {
        if (load_frame()) last_gen = g;
    }
    GtkAllocation a;
    gtk_widget_get_allocation(draw_area, &a);
    if (a.width > 0 && a.height > 0)
        write_size(a.width, a.height);
    write_keys();
    arcade_audio_poll(path_sfx);
    arcade_audio_poll_vol(path_vol);
    return TRUE;
}

static gboolean on_delete(GtkWidget *, GdkEvent *, gpointer) {
    write_cmd("quit");
    gtk_main_quit();
    return TRUE;
}

static void cb_menu(GtkMenuItem *, gpointer data) {
    if (data) write_cmd((const char *)data);
}

int main(int argc, char **argv) {
    const char *d = "/dev/shm/arcade_app";
    if (argc > 1 && argv[1] && argv[1][0]) d = argv[1];
    snprintf(dir, sizeof dir, "%s", d);
    mkdir(dir, 0755);
    snprintf(path_meta, sizeof path_meta, "%s/meta.bin", dir);
    snprintf(path_frame, sizeof path_frame, "%s/frame.raw", dir);
    snprintf(path_gen, sizeof path_gen, "%s/gen.txt", dir);
    snprintf(path_cmd, sizeof path_cmd, "%s/cmd.txt", dir);
    snprintf(path_keys, sizeof path_keys, "%s/keys.txt", dir);
    snprintf(path_size, sizeof path_size, "%s/size.txt", dir);
    snprintf(path_sfx, sizeof path_sfx, "%s/sfx.bin", dir);
    snprintf(path_vol, sizeof path_vol, "%s/vol.txt", dir);

    gtk_init(&argc, &argv);
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Arcade");
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 720);
    gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
    g_signal_connect(win, "delete-event", G_CALLBACK(on_delete), NULL);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_key), NULL);
    g_signal_connect(win, "key-release-event", G_CALLBACK(on_key_up), NULL);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background-color: #050a18; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *menu = gtk_menu_bar_new();
    GtkWidget *game_item = gtk_menu_item_new_with_label("Game");
    GtkWidget *game_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(game_item), game_menu);
    GtkWidget *mi;
    mi = gtk_menu_item_new_with_label("Start");
    g_signal_connect(mi, "activate", G_CALLBACK(cb_menu), (gpointer)"start");
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), mi);
    mi = gtk_menu_item_new_with_label("Pause / Settings");
    g_signal_connect(mi, "activate", G_CALLBACK(cb_menu), (gpointer)"pause");
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), mi);
    mi = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(mi, "activate", G_CALLBACK(cb_menu), (gpointer)"quit");
    g_signal_connect(mi, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), game_item);
    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);

    draw_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(draw_area, TRUE);
    gtk_widget_set_vexpand(draw_area, TRUE);
    gtk_widget_set_size_request(draw_area, 320, 240);
    gtk_widget_set_can_focus(draw_area, TRUE);
    gtk_widget_add_events(draw_area, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    g_signal_connect(draw_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(draw_area, "configure-event", G_CALLBACK(on_configure), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), draw_area, TRUE, TRUE, 0);

    status = gtk_label_new("Arrows move · Z/Space fire · Enter start · P pause/settings · Esc quit");
    gtk_widget_set_halign(status, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 2);

    write_keys();
    write_size(900, 720);
    arcade_audio_init(".");
    g_timeout_add(16, on_tick, NULL);
    gtk_widget_show_all(win);
    load_frame();
    gtk_main();
    write_cmd("quit");
    arcade_audio_shutdown();
    if (surf) cairo_surface_destroy(surf);
    return 0;
}
