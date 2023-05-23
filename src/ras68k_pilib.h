#ifndef __H_RAS68K_PILIB__
#define __H_RAS68K_PILIB__

#include <stdint.h>
#include <stddef.h>

int32_t ras68k_pilib_keepchk(void);
int32_t ras68k_pilib_init_opm(void);
int32_t ras68k_pilib_init_psg(void);
int32_t ras68k_pilib_init_midi(void);
int32_t ras68k_pilib_set_filter_mode(int16_t filter_enabled);
int32_t ras68k_pilib_set_reverb_type(int16_t reverb_type);
int32_t ras68k_pilib_upload_pcm_data(void* buf, size_t buf_len);
int32_t ras68k_pilib_upload_pcm_data2(void* buf, size_t buf_len);
int32_t ras68k_pilib_send_register_data(uint8_t reg, uint8_t data);
int32_t ras68k_pilib_send_midi_data(uint8_t data);
int32_t ras68k_pilib_stop_pcm_all(void);
int32_t ras68k_pilib_stop_pcm(void);
int32_t ras68k_pilib_play_adpcm(uint32_t mode, void* buf, size_t buf_len);
int32_t ras68k_pilib_play_pcm8(uint16_t channel, uint32_t mode, void* buf, size_t buf_len);
int32_t ras68k_pilib_play_pcm8pp(uint16_t channel, uint32_t mode, uint32_t freq, void* buf, size_t buf_len);
int32_t ras68k_pilib_set_pcm8_mode(uint16_t channel, uint32_t mode);
int32_t ras68k_pilib_set_pcm8pp_mode(uint16_t channel, uint32_t mode, uint32_t freq);

#endif