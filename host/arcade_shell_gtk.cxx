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
#include <gdk/gdkx.h>
#include <cairo.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <linux/input.h>

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "audio.h"

static char dir[512];
static char path_meta[600], path_frame[600], path_gen[600];
static char path_cmd[600], path_keys[600], path_size[600], path_sfx[600], path_vol[600];
static char path_ptr[600];
static int fd_ptr = -1;
static int ed_down = 0;
static int ed_x = 0;
static int ed_y = 0;
static uint32_t ed_kseq = 0;

static GtkWidget *win, *draw_area, *menu_bar, *status;
static cairo_surface_t *surf;
static Display *xdpy;
static XImage *ximg;
static XShmSegmentInfo xshm;
static GC xgc;
static int use_shm, xw, xh;
static int last_gen = -1, fw = 800, fh = 600;
static int k_l, k_r, k_u, k_d, k_f, k_s, k_p, k_n, k_x;
static char win_title[128];
static int editor_keys;
static int last_dw, last_dh;

/* Host side of prof.txt. gap is the tick period. idle is the wait until
 * GTK calls the tick. busy is the tick body. audio is inside busy. */
static long long hp_win, hp_prev, hp_tick_end;
static long long hp_gap_acc, hp_gap_max, hp_load_acc, hp_load_max;
static long long hp_paint_acc, hp_paint_max;
static long long hp_busy_acc, hp_busy_max, hp_idle_acc, hp_idle_max;
static long long hp_audio_acc, hp_audio_max;
static int hp_ticks, hp_loads, hp_skips, hp_paints, hp_idles;

static void write_file(const char *path, const char *s);

static long long mono_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + (long long)ts.tv_nsec / 1000LL;
}

static void hprof_paint(long long us) {
    if (us < 0) us = 0;
    hp_paint_acc += us;
    if (us > hp_paint_max) hp_paint_max = us;
    hp_paints++;
}

static void hprof_flush(long long now) {
    char path[600];
    char buf[480];
    long long gap_avg, load_avg, paint_avg, busy_avg, idle_avg, audio_avg;
    if (hp_win == 0) {
        hp_win = now;
        return;
    }
    if (now - hp_win < 1000000) return;
    gap_avg = hp_ticks > 0 ? hp_gap_acc / hp_ticks : 0;
    load_avg = hp_loads > 0 ? hp_load_acc / hp_loads : 0;
    paint_avg = hp_paints > 0 ? hp_paint_acc / hp_paints : 0;
    busy_avg = hp_ticks > 0 ? hp_busy_acc / hp_ticks : 0;
    idle_avg = hp_idles > 0 ? hp_idle_acc / hp_idles : 0;
    audio_avg = hp_ticks > 0 ? hp_audio_acc / hp_ticks : 0;
    snprintf(buf, sizeof buf,
             "ticks=%d gap=%lld/%lld idle=%lld/%lld busy=%lld/%lld audio=%lld/%lld load=%lld/%lld skip=%d paint=%lld/%lld\n",
             hp_ticks, gap_avg, hp_gap_max, idle_avg, hp_idle_max,
             busy_avg, hp_busy_max, audio_avg, hp_audio_max,
             load_avg, hp_load_max, hp_skips, paint_avg, hp_paint_max);
    snprintf(path, sizeof path, "%s/hprof.txt", dir);
    write_file(path, buf);
    hp_win = now;
    hp_gap_acc = hp_gap_max = 0;
    hp_load_acc = hp_load_max = 0;
    hp_paint_acc = hp_paint_max = 0;
    hp_busy_acc = hp_busy_max = 0;
    hp_idle_acc = hp_idle_max = 0;
    hp_audio_acc = hp_audio_max = 0;
    hp_ticks = hp_loads = hp_skips = hp_paints = hp_idles = 0;
}

/* Device pixels of the drawing area. GTK allocation is logical; the
 * framebuffer is alloc * scale so the SVG raster is the monitor's pixels. */
static int device_px(GtkWidget *w, int *pw, int *ph) {
    GtkAllocation a;
    int s;
    gtk_widget_get_allocation(w, &a);
    s = gtk_widget_get_scale_factor(w);
    if (s < 1) s = 1;
    *pw = a.width * s;
    *ph = a.height * s;
    return s;
}

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
    snprintf(buf, sizeof buf, "%d %d %d %d %d %d %d %d %d\n",
             k_l, k_r, k_u, k_d, k_f, k_s, k_p, k_n, k_x);
    write_file(path_keys, buf);
}

/* Pointer file: 32-byte head, then 32 edges of 16 bytes.
 * Motions update the head only. Presses, releases, and keys append an
 * edge so a click is still there when the editor next reads. */
enum { ED_RING = 32 };
struct EdEv { uint32_t x, y, btn, cmd; };
static EdEv ed_ring[ED_RING];
static uint32_t ed_edge = 0;
static uint32_t cur_x = 0, cur_y = 0, cur_btn = 0;

static void publish_ptr(void) {
    uint32_t head[8];
    if (fd_ptr < 0) return;
    head[0] = ed_edge;
    head[1] = cur_x;
    head[2] = cur_y;
    head[3] = cur_btn;
    head[4] = 0;
    head[5] = 0;
    head[6] = 0;
    head[7] = 0;
    if (lseek(fd_ptr, 32, SEEK_SET) < 0) return;
    if (write(fd_ptr, ed_ring, sizeof ed_ring) < 0) return;
    if (lseek(fd_ptr, 0, SEEK_SET) < 0) return;
    if (write(fd_ptr, head, sizeof head) < 0) return;
}

static void push_edge(int btn, int cmd) {
    EdEv *e;
    if (fd_ptr < 0) return;
    e = &ed_ring[ed_edge % ED_RING];
    e->x = cur_x;
    e->y = cur_y;
    e->btn = (uint32_t)btn;
    e->cmd = (uint32_t)cmd;
    ed_edge++;
    publish_ptr();
}

static void write_ptr(int x, int y, int btn, int cmd) {
    cur_x = (uint32_t)x;
    cur_y = (uint32_t)y;
    ed_x = x;
    ed_y = y;
    if (cmd) {
        push_edge(btn, cmd);
        return;
    }
    cur_btn = (uint32_t)btn;
    push_edge(btn, 0);
}

static gboolean on_motion(GtkWidget *, GdkEventMotion *ev, gpointer) {
    cur_x = (uint32_t)ev->x;
    cur_y = (uint32_t)ev->y;
    ed_x = (int)ev->x;
    ed_y = (int)ev->y;
    publish_ptr();
    return FALSE;
}

static gboolean on_button(GtkWidget *w, GdkEventButton *ev, gpointer) {
    gtk_widget_grab_focus(w);
    if (ev->button == 1) {
        ed_down = (ev->type == GDK_BUTTON_PRESS) ? 1 : 0;
        write_ptr((int)ev->x, (int)ev->y, ed_down, 0);
        return TRUE;
    }
    if (ev->button == 3) {
        int btn = (ev->type == GDK_BUTTON_PRESS) ? 2 : ed_down;
        write_ptr((int)ev->x, (int)ev->y, btn, 0);
        return TRUE;
    }
    return FALSE;
}

/* Keypad scan codes are not in numeric order. KEY_KP7 is 71 and KEY_KP1
 * is 79, so a range from KP1 to KP9 never matches. Map each key. */
static int editor_char(int code) {
    if (code >= KEY_1 && code <= KEY_9) return '1' + (code - KEY_1);
    if (code == KEY_0 || code == KEY_KP0) return '0';
    if (code == KEY_KP1) return '1';
    if (code == KEY_KP2) return '2';
    if (code == KEY_KP3) return '3';
    if (code == KEY_KP4) return '4';
    if (code == KEY_KP5) return '5';
    if (code == KEY_KP6) return '6';
    if (code == KEY_KP7) return '7';
    if (code == KEY_KP8) return '8';
    if (code == KEY_KP9) return '9';
    if (code == KEY_BACKSPACE || code == KEY_DELETE) return 8;
    if (code == KEY_ENTER || code == KEY_KPENTER) return 13;
    if (code == KEY_MINUS || code == KEY_KPMINUS) return '-';
    if (code == KEY_A) return 'a';
    if (code == KEY_B) return 'b';
    if (code == KEY_C) return 'c';
    if (code == KEY_D) return 'd';
    if (code == KEY_E) return 'e';
    if (code == KEY_F) return 'f';
    if (code == KEY_G) return 'g';
    if (code == KEY_H) return 'h';
    if (code == KEY_I) return 'i';
    if (code == KEY_J) return 'j';
    if (code == KEY_K) return 'k';
    if (code == KEY_L) return 'l';
    if (code == KEY_M) return 'm';
    if (code == KEY_N) return 'n';
    if (code == KEY_O) return 'o';
    if (code == KEY_P) return 'p';
    if (code == KEY_Q) return 'q';
    if (code == KEY_R) return 'r';
    if (code == KEY_S) return 's';
    if (code == KEY_T) return 't';
    if (code == KEY_U) return 'u';
    if (code == KEY_V) return 'v';
    if (code == KEY_W) return 'w';
    if (code == KEY_X) return 'x';
    if (code == KEY_Y) return 'y';
    if (code == KEY_Z) return 'z';
    return 0;
}

static gboolean on_edit_key(GtkWidget *, GdkEventKey *, gpointer) {
    return FALSE;
}

static void write_size(int w, int h) {
    if (w < 1 || h < 1) return;
    char buf[64];
    snprintf(buf, sizeof buf, "%d %d\n", w, h);
    write_file(path_size, buf);
}

static int read_full(int fd, uint8_t *dst, size_t n) {
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, dst + got, n - got);
        if (r <= 0) return 0;
        got += (size_t)r;
    }
    return 1;
}

/* gen.txt is "count slot". slot is the finished buffer. The kernel is
 * filling the other one, so this read does not race the copy. */
static int read_gen(int *slot) {
    FILE *f = fopen(path_gen, "r");
    if (!f) return -1;
    int g = -1;
    int s = 0;
    int n = fscanf(f, "%d %d", &g, &s);
    fclose(f);
    if (n < 1) return -1;
    if (s != 0 && s != 1) s = 0;
    if (slot) *slot = s;
    return g;
}

static void shm_free_image(void) {
    if (!xdpy || !ximg) return;
    XShmDetach(xdpy, &xshm);
    XSync(xdpy, False);
    if (xshm.shmaddr && xshm.shmaddr != (char *)-1) shmdt(xshm.shmaddr);
    if (xshm.shmid >= 0) shmctl(xshm.shmid, IPC_RMID, 0);
    ximg->data = NULL;
    XDestroyImage(ximg);
    ximg = NULL;
    xshm.shmaddr = NULL;
    xshm.shmid = -1;
    xw = 0;
    xh = 0;
}

static int shm_init(void) {
    GdkDisplay *gd;
    if (!draw_area) return 0;
    gd = gtk_widget_get_display(draw_area);
    if (!GDK_IS_X11_DISPLAY(gd)) return 0;
    xdpy = gdk_x11_display_get_xdisplay(gd);
    if (!xdpy || !XShmQueryExtension(xdpy)) return 0;
    use_shm = 1;
    return 1;
}

/* Kernel buffer goes straight to the drawing-area window. GTK keeps the
 * menu, status line, and resize. Cairo is only the fallback. */
static int shm_ensure(int w, int h) {
    GdkWindow *gw;
    Window xwin;
    XWindowAttributes attr;
    size_t bytes;
    if (!use_shm || !xdpy) return 0;
    if (ximg && xw == w && xh == h && xgc) return 1;
    gw = gtk_widget_get_window(draw_area);
    if (!gw) return 0;
    xwin = gdk_x11_window_get_xid(gw);
    if (!xgc) xgc = XCreateGC(xdpy, xwin, 0, 0);
    if (ximg) shm_free_image();
    if (!XGetWindowAttributes(xdpy, xwin, &attr)) return 0;
    if (attr.depth != 24 && attr.depth != 32) {
        use_shm = 0;
        return 0;
    }
    ximg = XShmCreateImage(xdpy, attr.visual, attr.depth, ZPixmap, NULL, &xshm, w, h);
    if (!ximg) {
        use_shm = 0;
        return 0;
    }
    bytes = (size_t)ximg->bytes_per_line * (size_t)h;
    xshm.shmid = shmget(IPC_PRIVATE, bytes, IPC_CREAT | 0600);
    if (xshm.shmid < 0) {
        ximg->data = NULL;
        XDestroyImage(ximg);
        ximg = NULL;
        use_shm = 0;
        return 0;
    }
    xshm.shmaddr = (char *)shmat(xshm.shmid, 0, 0);
    if (xshm.shmaddr == (char *)-1) {
        shmctl(xshm.shmid, IPC_RMID, 0);
        ximg->data = NULL;
        XDestroyImage(ximg);
        ximg = NULL;
        use_shm = 0;
        return 0;
    }
    ximg->data = xshm.shmaddr;
    xshm.readOnly = False;
    if (!XShmAttach(xdpy, &xshm)) {
        shmdt(xshm.shmaddr);
        shmctl(xshm.shmid, IPC_RMID, 0);
        ximg->data = NULL;
        XDestroyImage(ximg);
        ximg = NULL;
        use_shm = 0;
        return 0;
    }
    XSync(xdpy, False);
    xw = w;
    xh = h;
    return 1;
}

static void shm_put(void) {
    GdkWindow *gw;
    if (!use_shm || !ximg || !xgc || !xdpy) return;
    gw = gtk_widget_get_window(draw_area);
    if (!gw) return;
    XShmPutImage(xdpy, gdk_x11_window_get_xid(gw), xgc, ximg,
                 0, 0, 0, 0, xw, xh, False);
    XFlush(xdpy);
}

static int load_frame(int slot) {
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
    char slot_path[600];
    snprintf(slot_path, sizeof slot_path, "%s/frame%d.raw", dir, slot);
    fd = open(slot_path, O_RDONLY);
    if (fd < 0) return 0;
    if (use_shm && shm_ensure(w, h)) {
        uint8_t *dst = (uint8_t *)ximg->data;
        int dp = ximg->bytes_per_line;
        int ok = 1;
        if (dp == p) {
            if (!read_full(fd, dst, (size_t)p * (size_t)h)) ok = 0;
        } else {
            for (int y = 0; y < h && ok; y++) {
                if (!read_full(fd, dst + (size_t)y * (size_t)dp, (size_t)w * 4)) ok = 0;
            }
        }
        close(fd);
        return ok;
    }
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
        if (!read_full(fd, dst, n)) ok = 0;
    } else {
        for (int y = 0; y < h && ok; y++) {
            if (!read_full(fd, dst + (size_t)y * (size_t)dp, (size_t)w * 4)) ok = 0;
        }
    }
    close(fd);
    if (!ok) return 0;
    cairo_surface_mark_dirty(surf);
    return 1;
}

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer) {
    long long paint0 = mono_us();
    int s, dw, dh;
    if (use_shm && ximg) {
        shm_put();
        hprof_paint(mono_us() - paint0);
        return TRUE;
    }
    cairo_set_source_rgb(cr, 0.02, 0.04, 0.10);
    cairo_paint(cr);
    if (!surf || fw < 1 || fh < 1) {
        hprof_paint(mono_us() - paint0);
        return FALSE;
    }
    /* 1:1 in device pixels. A scale here resampled the raster and walked
     * the ship off the pixel grid whenever the window was not 800×600. */
    s = device_px(w, &dw, &dh);
    cairo_save(cr);
    if (s != 1) cairo_scale(cr, 1.0 / (double)s, 1.0 / (double)s);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    cairo_restore(cr);
    hprof_paint(mono_us() - paint0);
    return FALSE;
}

static void publish_size(void) {
    int w, h;
    if (!draw_area) return;
    device_px(draw_area, &w, &h);
    if (w < 320 || h < 240) return;
    if (w == last_dw && h == last_dh) return;
    last_dw = w;
    last_dh = h;
    write_size(w, h);
}

static gboolean on_configure(GtkWidget *, GdkEventConfigure *, gpointer) {
    publish_size();
    return FALSE;
}

static gboolean on_state(GtkWidget *, GdkEventWindowState *ev, gpointer) {
    int full;
    if (!(ev->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)) return FALSE;
    full = (ev->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    if (menu_bar) {
        if (full) gtk_widget_hide(menu_bar);
        else gtk_widget_show(menu_bar);
    }
    if (status) {
        if (full) gtk_widget_hide(status);
        else gtk_widget_show(status);
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
    else if (code == KEY_N) slot = &k_n;
    else if (code == KEY_X) slot = &k_x;
    else if (edge && (code == KEY_ESC || code == KEY_Q)) {
        if (win && gtk_window_is_active(GTK_WINDOW(win))) {
            if (editor_keys && code == KEY_ESC) {
                push_edge((int)cur_btn, 27);
                return;
            }
            if (editor_keys && code == KEY_Q) {
                push_edge((int)cur_btn, 'q');
                return;
            }
            write_cmd("quit");
            gtk_main_quit();
        }
        return;
    }
    if (editor_keys && down && win && gtk_window_is_active(GTK_WINDOW(win))) {
        int ch = editor_char(code);
        if (ch && (edge || ch == 8)) push_edge((int)cur_btn, ch);
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
        write_file(path_keys, "0 0 0 0 0 0 0 0 0\n");
        return;
    }
    write_keys();
}

static gboolean on_tick(GtkWidget *w, GdkFrameClock *, gpointer) {
    long long t0 = mono_us();
    int slot = 0;
    int g;
    long long gap;
    if (hp_prev) {
        gap = t0 - hp_prev;
        if (gap < 0) gap = 0;
        hp_gap_acc += gap;
        if (gap > hp_gap_max) hp_gap_max = gap;
    }
    if (hp_tick_end) {
        long long idle = t0 - hp_tick_end;
        if (idle < 0) idle = 0;
        hp_idle_acc += idle;
        if (idle > hp_idle_max) hp_idle_max = idle;
        hp_idles++;
    }
    hp_prev = t0;
    hp_ticks++;
    g = read_gen(&slot);
    if (g != last_gen && g > 0) {
        long long a, d;
        if (last_gen >= 0 && g > last_gen + 1) hp_skips += g - last_gen - 1;
        a = mono_us();
        if (load_frame(slot)) {
            d = mono_us() - a;
            if (d < 0) d = 0;
            hp_load_acc += d;
            if (d > hp_load_max) hp_load_max = d;
            hp_loads++;
            last_gen = g;
            if (use_shm) {
                long long p0 = mono_us();
                shm_put();
                hprof_paint(mono_us() - p0);
            }
        }
    }
    publish_size();
    evdev_poll();
    publish_keys();
    {
        long long a = mono_us();
        long long d;
        arcade_audio_poll(path_sfx);
        arcade_audio_poll_vol(path_vol);
        d = mono_us() - a;
        if (d < 0) d = 0;
        hp_audio_acc += d;
        if (d > hp_audio_max) hp_audio_max = d;
    }
    if (!use_shm) gtk_widget_queue_draw(w);
    hprof_flush(mono_us());
    {
        long long busy = mono_us() - t0;
        if (busy < 0) busy = 0;
        hp_busy_acc += busy;
        if (busy > hp_busy_max) hp_busy_max = busy;
        hp_tick_end = mono_us();
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_pump(gpointer) {
    on_tick(draw_area, NULL, NULL);
    return G_SOURCE_CONTINUE;
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
    /* Copy before gtk_init, which permutes argv. No title keeps "Arcade". */
    snprintf(win_title, sizeof win_title, "%s", "Arcade");
    editor_keys = 0;
    if (argc > 2 && argv[2] && argv[2][0]) {
        snprintf(win_title, sizeof win_title, "%s", argv[2]);
        editor_keys = 1;
    }
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
    gtk_window_set_title(GTK_WINDOW(win), win_title);
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 720);
    gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
    g_signal_connect(win, "delete-event", G_CALLBACK(on_delete), NULL);
    g_signal_connect(win, "window-state-event", G_CALLBACK(on_state), NULL);

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

    if (!editor_keys) {
        GtkWidget *menu = gtk_menu_bar_new();
        menu_bar = menu;
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
    }

    draw_area = gtk_drawing_area_new();
    gtk_widget_set_app_paintable(draw_area, TRUE);
    gtk_widget_set_double_buffered(draw_area, FALSE);
    gtk_widget_set_hexpand(draw_area, TRUE);
    gtk_widget_set_vexpand(draw_area, TRUE);
    gtk_widget_set_size_request(draw_area, 320, 240);
    g_signal_connect(draw_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(draw_area, "configure-event", G_CALLBACK(on_configure), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), draw_area, TRUE, TRUE, 0);

    if (editor_keys) {
        snprintf(path_ptr, sizeof path_ptr, "%s/pointer.bin", dir);
        fd_ptr = open(path_ptr, O_RDWR | O_CREAT, 0644);
        if (fd_ptr >= 0 && ftruncate(fd_ptr, 32 + ED_RING * 16) != 0) {
            /* a short pointer file is ignored by the editor */
        }
        gtk_widget_set_can_focus(draw_area, TRUE);
        gtk_widget_add_events(draw_area,
            GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK |
            GDK_KEY_PRESS_MASK);
        g_signal_connect(draw_area, "motion-notify-event", G_CALLBACK(on_motion), NULL);
        g_signal_connect(draw_area, "button-press-event", G_CALLBACK(on_button), NULL);
        g_signal_connect(draw_area, "button-release-event", G_CALLBACK(on_button), NULL);
        g_signal_connect(draw_area, "key-press-event", G_CALLBACK(on_edit_key), NULL);
    }

    if (!editor_keys) {
        status = gtk_label_new("Arrows move · Z/Space fire · Enter start · P pause/settings · Esc quit");
        gtk_widget_set_halign(status, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 2);
    }

    evdev_open_all();
    write_keys();
    arcade_audio_init(".");
    gtk_widget_show_all(win);
    gtk_widget_grab_focus(draw_area);
    shm_init();
    publish_size();
    {
        int slot = 0;
        int g = read_gen(&slot);
        if (g > 0 && load_frame(slot)) {
            last_gen = g;
            if (use_shm) shm_put();
            else gtk_widget_queue_draw(draw_area);
        }
    }
    g_timeout_add(8, on_pump, NULL);
    gtk_main();
    write_cmd("quit");
    arcade_audio_shutdown();
    shm_free_image();
    if (xgc && xdpy) XFreeGC(xdpy, xgc);
    if (surf) cairo_surface_destroy(surf);
    return 0;
}
