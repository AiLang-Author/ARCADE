/* miniaudio engine + sfx.bin ring reader + random BGM.
 * Copyright © 2026 Sean Collins, 2 Paws Machine and Engineering. SCSL v1.0.
 */
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_MP3
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
#include <time.h>
#include <unistd.h>

#define MUSIC_CAP 16

static ma_engine engine;
static int engine_ok;
static int primed;
static uint32_t seen;
static char clip[8][640];

static char music_dir[512];
static char music_files[MUSIC_CAP][512];
static int music_n;
static char stage_files[MUSIC_CAP][512];
static int stage_n;
static char title_file[512];
static char queen_file[512];
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
    snprintf(music_dir, sizeof music_dir, "%s/assets/music", root);
    DIR *d = opendir(music_dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        const char *n = e->d_name;
        size_t L = strlen(n);
        if (L < 5) continue;
        if (strcasecmp(n + L - 4, ".wav") != 0) continue;
        if (music_n >= MUSIC_CAP) break;
        snprintf(music_files[music_n], sizeof music_files[0], "%s", n);
        music_n++;
        if (has_ci(n, "turn us around")) {
            snprintf(title_file, sizeof title_file, "%s", n);
        } else if (has_ci(n, "queen")) {
            snprintf(queen_file, sizeof queen_file, "%s", n);
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
    if (title_file[0]) bgm_play_file(title_file, 1, 2);
    else bgm_next();
}

static void bgm_queen(void) {
    if (queen_file[0]) bgm_play_file(queen_file, 1, 3);
    else bgm_next();
}

int arcade_audio_init(const char *root) {
    const char *use = ".";
    if (root && try_root(root)) use = root;
    else if (try_root(".")) use = ".";
    else if (try_root("..")) use = "..";
    for (int i = 1; i < 8; i++) {
        snprintf(clip[i], sizeof clip[i], "%s/assets/sfx/%s", use, kName[i]);
    }
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
    fprintf(stderr, "arcade audio: sfx from %s/assets/sfx  music=%d stage=%d title=%s queen=%s\n",
            use, music_n, stage_n, title_file[0] ? title_file : "-", queen_file[0] ? queen_file : "-");
    bgm_title();
    return 1;
}

void arcade_audio_play(int code) {
    if (!engine_ok) return;
    if (code == 8) { bgm_next(); return; }
    if (code == 9) { bgm_stop(); return; }
    if (code == 10) { bgm_title(); return; }
    if (code == 11) { bgm_queen(); return; }
    if (code < 1 || code > 7) return;
    if (clip[code][0] == 0) return;
    ma_sound_group *g = grp_ok ? &grp_sfx : NULL;
    ma_engine_play_sound(&engine, clip[code], g);
}

void arcade_audio_set_vol(int music10, int sfx10) {
    if (music10 < 0) music10 = 0;
    if (music10 > 10) music10 = 10;
    if (sfx10 < 0) sfx10 = 0;
    if (sfx10 > 10) sfx10 = 10;
    vol_music = (music10 / 10.0f) * 0.50f;
    vol_sfx = (sfx10 / 10.0f) * 0.80f;
    if (bgm_loaded) ma_sound_set_volume(&bgm, vol_music);
    if (grp_ok) ma_sound_group_set_volume(&grp_sfx, vol_sfx);
}

void arcade_audio_poll_vol(const char *vol_path) {
    if (!vol_path) return;
    FILE *f = fopen(vol_path, "r");
    if (!f) return;
    int m = 7, s = 8;
    int n = fscanf(f, "%d %d", &m, &s);
    fclose(f);
    if (n != 2) return;
    if (m == last_mv && s == last_sv) return;
    last_mv = m;
    last_sv = s;
    arcade_audio_set_vol(m, s);
}

void arcade_audio_poll(const char *sfx_bin) {
    if (!sfx_bin) return;
    int fd = open(sfx_bin, O_RDONLY);
    if (fd < 0) return;
    uint8_t buf[72];
    ssize_t n = read(fd, buf, 72);
    close(fd);
    if (n < 72) return;
    uint32_t w = (uint32_t)le32u(buf);
    if (!primed) {
        seen = w;
        primed = 1;
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
