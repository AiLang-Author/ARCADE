/* Host mixer. Kernel emits codes in sfx.bin; SFX clips + random BGM. */
#ifndef ARCADE_AUDIO_H
#define ARCADE_AUDIO_H

int arcade_audio_init(const char *root);
void arcade_audio_play(int code);
void arcade_audio_poll(const char *sfx_bin);
void arcade_audio_set_vol(int music10, int sfx10);
void arcade_audio_poll_vol(const char *vol_path);
void arcade_audio_shutdown(void);

#endif
