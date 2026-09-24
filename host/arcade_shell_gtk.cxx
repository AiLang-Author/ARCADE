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

#include <linux/input.h>

#include <dirent.h>
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

/* Bug: GTK/X autorepeat arrives as a release, so a held fire key dropped
 * to 0 and the gun stopped, then came back on the next press. Evdev matches
 * Display Evdev_HandleKey: value 0 up, 1 down, 2 repeat-still-down.
 * No grab, so the rest of the desktop keeps the keyboard. */
static int ev_fd[16];
static int ev_n;

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

static int ev_bit(const unsigned char *b, int bit) {
    return (b[bit >> 3] & (unsigned char)(1u << (bit & 7))) != 0;
}

static int ev_is_keyboard(int fd) {
    unsigned char ev[(EV_MAX + 7) / 8];
    unsigned char keys[(KEY_MAX + 7) / 8];
    memset(ev, 0, sizeof ev);
    memset(keys, 0, sizeof keys);
    if (ioctl(fd, EVIOCGBIT(0, sizeof ev), ev) < 0) return 0;
    if (!ev_bit(ev, EV_KEY)) return 0;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof keys), keys) < 0) return 0;
    return ev_bit(keys, KEY_A) || ev_bit(keys, KEY_Z) || ev_bit(keys, KEY_SPACE);
}

static void evdev_open_all(void) {
    int i;
    DIR *d;
    struct dirent *de;
    for (i = 0; i < ev_n; i++) if (ev_fd[i] >= 0) close(ev_fd[i]);
    ev_n = 0;
    d = opendir("/dev/input");
    if (!d) return;
    while ((de = readdir(d)) != NULL && ev_n < 16) {
        char path[320];
        int fd;
        if (strncmp(de->d_name, "event", 5) != 0) continue;
        snprintf(path, sizeof path, "/dev/input/%s", de->d_name);
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        if (!ev_is_keyboard(fd)) { close(fd); continue; }
        ev_fd[ev_n++] = fd;
        fprintf(stderr, "arcade: key %s\n", path);
    }
    closedir(d);
    if (ev_n == 0) fprintf(stderr, "arcade: no evdev keyboard\n");
}

static void note_key(int code, int down, int edge) {
    int *slot = NULL;
    if (code == KEY_LEFT || code == KEY_A) slot = &k_l;
    else if (code == KEY_RIGHT || code == KEY_D) slot = &k_r;
    else if (code == KEY_UP || code == KEY_W) slot = &k_u;
    else if (code == KEY_DOWN || code == KEY_S) slot = &k_d;
    else if (code == KEY_Z || code == KEY_SPACE) slot = &k_f;
    else if (code == KEY_ENTER || code == KEY_KPENTER) slot = &k_s;
    else if (code == KEY_P) slot = &k_p;
    else if (edge && (code == KEY_ESC || code == KEY_Q)) {
        if (win && gtk_window_is_active(GTK_WINDOW(win))) {
            write_cmd("quit");
            gtk_main_quit();
        }
        return;
    }
    if (!slot) return;
    *slot = down ? 1 : 0;
}

static void evdev_poll(void) {
    int i;
    for (i = 0; i < ev_n; i++) {
        for (;;) {
            struct input_event ev;
            ssize_t n = read(ev_fd[i], &ev, sizeof ev);
            if (n != (ssize_t)sizeof ev) break;
            if (ev.type != EV_KEY) continue;
            if (ev.value == 0) note_key(ev.code, 0, 1);
            else if (ev.value == 1) note_key(ev.code, 1, 1);
            else if (ev.value == 2) note_key(ev.code, 1, 0);
        }
    }
}

static void publish_keys(void) {
    if (!win || !gtk_window_is_active(GTK_WINDOW(win))) {
        write_file(path_keys, "0 0 0 0 0 0 0\n");
        return;
    }
    write_keys();
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
    evdev_poll();
    publish_keys();
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

static void chdir_to_data(void) {
    char exe[512], probe[640];
    if (access("assets/ships/player.svg", R_OK) == 0) return;
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n <= 0) return;
    exe[n] = 0;
    char *slash = strrchr(exe, '/');
    if (!slash) return;
    *slash = 0;
    /* binary is ROOT/host/arcade_shell_gtk */
    slash = strrchr(exe, '/');
    if (slash && strcmp(slash, "/host") == 0) *slash = 0;
    snprintf(probe, sizeof probe, "%s/assets/ships/player.svg", exe);
    if (access(probe, R_OK) == 0) {
        if (chdir(exe) == 0)
            fprintf(stderr, "arcade: data %s\n", exe);
        return;
    }
}

int main(int argc, char **argv) {
    chdir_to_data();
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
    gtk_widget_set_can_focus(menu, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);

    draw_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(draw_area, TRUE);
    gtk_widget_set_vexpand(draw_area, TRUE);
    gtk_widget_set_size_request(draw_area, 320, 240);
    g_signal_connect(draw_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(draw_area, "configure-event", G_CALLBACK(on_configure), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), draw_area, TRUE, TRUE, 0);

    status = gtk_label_new("Arrows move · Z/Space fire · Enter start · P pause/settings · Esc quit");
    gtk_widget_set_halign(status, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 2);

    evdev_open_all();
    write_keys();
    write_size(900, 720);
    arcade_audio_init(".");
    g_timeout_add(16, on_tick, NULL);
    gtk_widget_show_all(win);
    gtk_widget_grab_focus(draw_area);
    load_frame();
    gtk_main();
    write_cmd("quit");
    arcade_audio_shutdown();
    if (surf) cairo_surface_destroy(surf);
    return 0;
}
