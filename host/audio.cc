/* miniaudio engine + sfx.bin ring reader + random BGM.
 * Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
 */
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_GENERATION
#include "miniaudio.h"

#include "audio.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MUSIC_CAP 16

static ma_engine engine;
static int engine_ok;
static int primed;
static uint32_t seen;
static char clip[32][640];

static char music_dir[512];
static char music_files[MUSIC_CAP][512];
static int music_n;
static char stage_files[MUSIC_CAP][512];
static int stage_n;
static char title_file[512];
static char queen_file[512];
static char gyre_file[512];
static char lobby_file[512];
static int music_cur = -1;
static int music_mode;
static ma_sound bgm;
static int bgm_loaded;
static ma_sound_group grp_sfx;
static int grp_ok;
static float vol_music = 0.35f;
static float vol_sfx = 0.80f;
static int last_mv = -1;
static int last_sv = -1;

static const char *kName[8] = {
    "", "shot.wav", "eshot.wav", "boom.wav",
    "hit.wav", "death.wav", "wave.wav", "start.wav"
};

/* Circuit codes 14..23. ray.wav is the gun; enemy-shot.wav fills in
 * until that file is dropped in the same folder. */
static const char *kCircuit[10] = {
    "jump.wav", "land.wav", "hurt.wav", "stomp.wav", "ray.wav",
    "goal.wav", "checkpoint.wav", "powerup.wav", "derez.wav", "crouch.wav"
};

static void load_circuit(const char *use) {
    char path[640];
    int i;
    for (i = 0; i < 10; i++) {
        int code = 14 + i;
        snprintf(path, sizeof path, "%s/assets/circuit/audio/sfx/%s", use, kCircuit[i]);
        if (access(path, R_OK) == 0) {
            snprintf(clip[code], sizeof clip[code], "%s", path);
            continue;
        }
        if (code == 18) {
            snprintf(path, sizeof path, "%s/assets/circuit/audio/sfx/enemy-shot.wav", use);
            if (access(path, R_OK) == 0)
                snprintf(clip[code], sizeof clip[code], "%s", path);
        }
    }
}

static int le32u(const uint8_t *p) {
    return (int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static int try_root(const char *root) {
    char probe[640];
    snprintf(probe, sizeof probe, "%s/assets/sfx/shot.wav", root);
    return access(probe, R_OK) == 0;
}

static int has_ci(const char *h, const char *n) {
    size_t nh = strlen(h), nn = strlen(n);
    if (nn == 0 || nn > nh) return 0;
    for (size_t i = 0; i + nn <= nh; i++) {
        if (strncasecmp(h + i, n, nn) == 0) return 1;
    }
    return 0;
}

static void scan_music(const char *root) {
    music_n = 0;
    stage_n = 0;
    title_file[0] = 0;
    queen_file[0] = 0;
    gyre_file[0] = 0;
    lobby_file[0] = 0;
    snprintf(music_dir, sizeof music_dir, "%s/assets/music", root);
    DIR *d = opendir(music_dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        const char *n = e->d_name;
        size_t L = strlen(n);
        if (L < 5) continue;
        if (strcasecmp(n + L - 4, ".mp3") != 0) continue;
        if (music_n >= MUSIC_CAP) break;
        snprintf(music_files[music_n], sizeof music_files[0], "%s", n);
        music_n++;
        if (has_ci(n, "melow") || has_ci(n, "mellow") || has_ci(n, "lobby")) {
            snprintf(lobby_file, sizeof lobby_file, "%s", n);
        } else if (has_ci(n, "turn us around")) {
            snprintf(title_file, sizeof title_file, "%s", n);
        } else if (has_ci(n, "queen")) {
            snprintf(queen_file, sizeof queen_file, "%s", n);
        } else if (has_ci(n, "gyre") || has_ci(n, "iron cluster")) {
            snprintf(gyre_file, sizeof gyre_file, "%s", n);
        } else if (stage_n < MUSIC_CAP) {
            snprintf(stage_files[stage_n], sizeof stage_files[0], "%s", n);
            stage_n++;
        }
    }
    closedir(d);
}

static void bgm_stop(void) {
    if (!bgm_loaded) return;
    ma_sound_stop(&bgm);
    ma_sound_uninit(&bgm);
    bgm_loaded = 0;
    music_mode = 0;
}

static void bgm_play_file(const char *name, int loop, int mode) {
    if (!engine_ok || !name || !name[0]) return;
    bgm_stop();
    char path[640];
    snprintf(path, sizeof path, "%s/%s", music_dir, name);
    ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;
    if (ma_sound_init_from_file(&engine, path, flags, NULL, NULL, &bgm) != MA_SUCCESS) {
        fprintf(stderr, "arcade audio: bgm failed %s\n", path);
        return;
    }
    ma_sound_set_volume(&bgm, vol_music);
    ma_sound_set_looping(&bgm, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(&bgm);
    bgm_loaded = 1;
    music_mode = mode;
    fprintf(stderr, "arcade audio: bgm %s\n", name);
}

static void bgm_next(void) {
    if (!engine_ok || stage_n < 1) return;
    int pick = 0;
    if (stage_n == 1) {
        pick = 0;
    } else {
        pick = rand() % stage_n;
        if (pick == music_cur) pick = (pick + 1) % stage_n;
    }
    music_cur = pick;
    bgm_play_file(stage_files[pick], 0, 1);
}

static void bgm_title(void) {
    if (bgm_loaded && music_mode == 2) return;
    if (title_file[0]) bgm_play_file(title_file, 1, 2);
    else bgm_next();
}

/* Mode 5. The select menu loops the lobby track. */
static void bgm_lobby(void) {
    if (bgm_loaded && music_mode == 5) return;
    if (lobby_file[0]) bgm_play_file(lobby_file, 1, 5);
    else if (title_file[0]) bgm_play_file(title_file, 1, 2);
    else bgm_next();
}

static void bgm_queen(void) {
    if (bgm_loaded && music_mode == 3) return;
    if (queen_file[0]) bgm_play_file(queen_file, 1, 3);
    else bgm_next();
}

/* Mode 4. Missing file stays silent so the NOWAY HOME title cannot leak in. */
static void bgm_gyre(void) {
    if (!gyre_file[0]) {
        bgm_stop();
        return;
    }
    if (bgm_loaded && music_mode == 4) return;
    bgm_play_file(gyre_file, 1, 4);
}

int arcade_audio_init(const char *root) {
    const char *use = ".";
    if (root && try_root(root)) use = root;
    else if (try_root(".")) use = ".";
    else if (try_root("..")) use = "..";
    for (int i = 1; i < 8; i++) {
        snprintf(clip[i], sizeof clip[i], "%s/assets/sfx/%s", use, kName[i]);
    }
    load_circuit(use);
    scan_music(use);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    ma_engine_config cfg = ma_engine_config_init();
    if (ma_engine_init(&cfg, &engine) != MA_SUCCESS) {
        fprintf(stderr, "arcade audio: engine init failed (no playback device)\n");
        engine_ok = 0;
        return 0;
    }
    ma_engine_set_volume(&engine, 1.0f);
    grp_ok = 0;
    if (ma_sound_group_init(&engine, 0, NULL, &grp_sfx) == MA_SUCCESS) {
        grp_ok = 1;
        ma_sound_group_set_volume(&grp_sfx, vol_sfx);
    }
    engine_ok = 1;
    primed = 0;
    seen = 0;
    fprintf(stderr, "arcade audio: sfx from %s/assets/sfx  music=%d stage=%d title=%s queen=%s gyre=%s lobby=%s\n",
            use, music_n, stage_n, title_file[0] ? title_file : "-",
            queen_file[0] ? queen_file : "-", gyre_file[0] ? gyre_file : "-",
            lobby_file[0] ? lobby_file : "-");
    return 1;
}

void arcade_audio_play(int code) {
    if (!engine_ok) return;
    if (code == 8) { bgm_next(); return; }
    if (code == 9) { bgm_stop(); return; }
    if (code == 10) { bgm_title(); return; }
    if (code == 11) { bgm_queen(); return; }
    if (code == 12) { bgm_gyre(); return; }
    if (code == 13) { bgm_lobby(); return; }
    if (code >= 14 && code < 32) {
        if (clip[code][0] == 0) return;
        ma_sound_group *g = grp_ok ? &grp_sfx : NULL;
        ma_engine_play_sound(&engine, clip[code], g);
        return;
    }
    if (code < 1 || code > 7) return;
    if (clip[code][0] == 0) return;
    ma_sound_group *g = grp_ok ? &grp_sfx : NULL;
    ma_engine_play_sound(&engine, clip[code], g);
}

static int last_hold;
static int last_full = -1;

static void apply_vol(int music10, int sfx10, int full) {
    if (music10 < 0) music10 = 0;
    if (music10 > 10) music10 = 10;
    if (sfx10 < 0) sfx10 = 0;
    if (sfx10 > 10) sfx10 = 10;
    if (full) {
        vol_music = music10 / 10.0f;
        vol_sfx = sfx10 / 10.0f;
    } else {
        vol_music = (music10 / 10.0f) * 0.50f;
        vol_sfx = (sfx10 / 10.0f) * 0.80f;
    }
    if (bgm_loaded) ma_sound_set_volume(&bgm, vol_music);
    if (grp_ok) ma_sound_group_set_volume(&grp_sfx, vol_sfx);
}

void arcade_audio_set_vol(int music10, int sfx10) {
    apply_vol(music10, sfx10, 0);
}

void arcade_audio_poll_vol(const char *vol_path) {
    if (!vol_path) return;
    FILE *f = fopen(vol_path, "r");
    if (!f) return;
    int m = 7, s = 8, hold = 0;
    int n = fscanf(f, "%d %d %d", &m, &s, &hold);
    fclose(f);
    if (n < 2) return;
    /* Circuit writes a third number. 1 holds the track without unloading it. */
    /* A third number is Circuit's scale, including when it shares the cabinet directory. */
    int full = (n >= 3) || (strstr(vol_path, "/circuit/") != NULL);
    if (m != last_mv || s != last_sv || full != last_full) {
        last_mv = m;
        last_sv = s;
        last_full = full ? 1 : 0;
        apply_vol(m, s, full);
    }
    if (n < 3) hold = 0;
    if (hold != last_hold) {
        last_hold = hold ? 1 : 0;
        if (bgm_loaded) {
            if (last_hold) ma_sound_stop(&bgm);
            else ma_sound_start(&bgm);
        }
    }
}

/* Editor writes "<gen> <path>" into <shm>/preview.txt. One gen plays once. */
static ma_sound preview_snd;
static int preview_live;
static int preview_gen;

static void preview_stop(void) {
    if (!preview_live) return;
    ma_sound_stop(&preview_snd);
    ma_sound_uninit(&preview_snd);
    preview_live = 0;
}

static void poll_preview(const char *sfx_bin) {
    if (!sfx_bin) return;
    size_t n = strlen(sfx_bin);
    if (n < 8 || n + 16 >= 600) return;
    if (strcmp(sfx_bin + n - 7, "sfx.bin") != 0) return;
    char path[600];
    memcpy(path, sfx_bin, n - 7);
    memcpy(path + (n - 7), "preview.txt", 12);
    int fd = open(path, O_RDWR);
    if (fd < 0) return;
    char buf[640];
    ssize_t got = read(fd, buf, sizeof buf - 1);
    if (got <= 0) {
        close(fd);
        return;
    }
    buf[got] = 0;
    while (got > 0 && (buf[got - 1] == '\n' || buf[got - 1] == '\r' || buf[got - 1] == ' '))
        buf[--got] = 0;
    char *s = buf;
    while (*s == ' ' || *s == '\t') s++;
    int gen = 0;
    while (*s >= '0' && *s <= '9') {
        gen = gen * 10 + (*s - '0');
        s++;
    }
    while (*s == ' ' || *s == '\t') s++;
    int cleared = -1;
    if (lseek(fd, 0, SEEK_SET) >= 0)
        cleared = ftruncate(fd, 0);
    close(fd);
    if (cleared != 0 && gen == preview_gen) return;
    if (gen == 0 || gen == preview_gen) return;
    preview_gen = gen;
    if (!engine_ok) return;
    preview_stop();
    if (s[0] == 0 || access(s, R_OK) != 0) return;
    fprintf(stderr, "preview %d %s\n", gen, s);
    ma_sound_group *g = grp_ok ? &grp_sfx : NULL;
    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
    if (ma_sound_init_from_file(&engine, s, flags, g, NULL, &preview_snd) != MA_SUCCESS) return;
    ma_sound_set_looping(&preview_snd, MA_FALSE);
    if (ma_sound_start(&preview_snd) != MA_SUCCESS) {
        ma_sound_uninit(&preview_snd);
        return;
    }
    preview_live = 1;
}

/* Circuit publishes code + path lines after it reads the level. */
static time_t map_m;
static off_t map_sz;
static int map_seen;

static char level_music[640];
static time_t mus_m;
static off_t mus_sz;
static int mus_seen;

static void bgm_play_path(const char *path) {
    if (!engine_ok || !path || !path[0]) return;
    bgm_stop();
    ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;
    if (ma_sound_init_from_file(&engine, path, flags, NULL, NULL, &bgm) != MA_SUCCESS) {
        fprintf(stderr, "arcade audio: bgm failed %s\n", path);
        level_music[0] = 0;
        return;
    }
    ma_sound_set_volume(&bgm, vol_music);
    ma_sound_set_looping(&bgm, MA_TRUE);
    bgm_loaded = 1;
    if (!last_hold) ma_sound_start(&bgm);
    music_mode = 6;
    snprintf(level_music, sizeof level_music, "%s", path);
    fprintf(stderr, "arcade audio: bgm %s\n", path);
}

static int sibling_path(const char *sfx_bin, const char *name, char *out, size_t cap) {
    if (!sfx_bin || !name || !out || cap < 8) return 0;
    const char *slash = strrchr(sfx_bin, '/');
    size_t dirn = slash ? (size_t)(slash - sfx_bin) : 0;
    size_t namelen = strlen(name);
    if (dirn + 1 + namelen + 1 > cap) return 0;
    if (dirn) memcpy(out, sfx_bin, dirn);
    out[dirn] = '/';
    memcpy(out + dirn + 1, name, namelen + 1);
    return 1;
}

/* Circuit writes one path, or a blank line for silence, beside sfx.bin. */
static void poll_music(const char *sfx_bin) {
    char pathbuf[600];
    if (!sibling_path(sfx_bin, "music.txt", pathbuf, sizeof pathbuf)) return;
    const char *p = pathbuf;
    struct stat st;
    if (stat(p, &st) != 0) return;
    if (mus_seen && st.st_mtime == mus_m && st.st_size == mus_sz) return;
    mus_seen = 1;
    mus_m = st.st_mtime;
    mus_sz = st.st_size;
    FILE *f = fopen(p, "r");
    if (!f) return;
    char line[700];
    if (!fgets(line, sizeof line, f)) {
        fclose(f);
        if (music_mode == 6) bgm_stop();
        level_music[0] = 0;
        return;
    }
    fclose(f);
    size_t len = strlen(line);
    while (len && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' '))
        line[--len] = 0;
    if (len == 0 || access(line, R_OK) != 0) {
        if (music_mode == 6) bgm_stop();
        level_music[0] = 0;
        return;
    }
    if (music_mode == 6 && strcmp(level_music, line) == 0 && bgm_loaded) return;
    bgm_play_path(line);
}

static void poll_sfx_map(const char *sfx_bin) {
    char pathbuf[600];
    if (!sibling_path(sfx_bin, "sfx.map", pathbuf, sizeof pathbuf)) return;
    const char *p = pathbuf;
    struct stat st;
    if (stat(p, &st) != 0) return;
    if (map_seen && st.st_mtime == map_m && st.st_size == map_sz) return;
    map_seen = 1;
    map_m = st.st_mtime;
    map_sz = st.st_size;
    FILE *f = fopen(p, "r");
    if (!f) return;
    char line[700];
    while (fgets(line, sizeof line, f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (*s < '0' || *s > '9') continue;
        int code = 0;
        while (*s >= '0' && *s <= '9') {
            code = code * 10 + (*s - '0');
            s++;
        }
        while (*s == ' ' || *s == '\t') s++;
        size_t len = strlen(s);
        while (len && (s[len - 1] == '\n' || s[len - 1] == '\r')) s[--len] = 0;
        if (code >= 14 && code < 32 && len > 0 && len < 640 && access(s, R_OK) == 0)
            snprintf(clip[code], sizeof clip[code], "%s", s);
    }
    fclose(f);
}

/* The menu writes its theme before the host opens. Keep that one music
 * code. Shot effects in the same backlog stay quiet. A level track that
 * poll_music just started is left alone. */
static int backlog_bgm(const uint8_t *buf, uint32_t w) {
    uint32_t nlook = w;
    int bgm = 0;
    if (nlook > 64) nlook = 64;
    if (nlook == 0) return 0;
    for (uint32_t i = w - nlook; i != w; i++) {
        int c = (int)buf[8 + (i % 64)];
        if (c >= 8 && c <= 13) bgm = c;
    }
    return bgm;
}

void arcade_audio_poll(const char *sfx_bin) {
    if (!sfx_bin) return;
    poll_preview(sfx_bin);
    poll_sfx_map(sfx_bin);
    poll_music(sfx_bin);
    int fd = open(sfx_bin, O_RDONLY);
    if (fd < 0) return;
    uint8_t buf[72];
    ssize_t n = read(fd, buf, 72);
    close(fd);
    if (n < 72) return;
    uint32_t w = (uint32_t)le32u(buf);
    if (!primed) {
        int bgm = 0;
        seen = w;
        primed = 1;
        if (!(bgm_loaded && music_mode == 6)) bgm = backlog_bgm(buf, w);
        if (bgm) arcade_audio_play(bgm);
        return;
    }
    uint32_t nnew = w - seen;
    if (nnew == 0) {
        if (bgm_loaded && ma_sound_at_end(&bgm) && music_mode == 1) bgm_next();
        return;
    }
    if (nnew > 64) nnew = 64;
    for (uint32_t i = w - nnew; i != w; i++) {
        arcade_audio_play((int)buf[8 + (i % 64)]);
    }
    seen = w;
    if (bgm_loaded && ma_sound_at_end(&bgm) && music_mode == 1) bgm_next();
}

void arcade_audio_shutdown(void) {
    preview_stop();
    bgm_stop();
    if (grp_ok) {
        ma_sound_group_uninit(&grp_sfx);
        grp_ok = 0;
    }
    if (engine_ok) {
        ma_engine_uninit(&engine);
        engine_ok = 0;
    }
}
