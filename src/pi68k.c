#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <doslib.h>
#include <iocslib.h>
#include "himem.h"
#include "ras68k.h"

int32_t main(int32_t argc, uint8_t* argv[]) {

  int rc = -1;

  FILE* fp = NULL;
  uint8_t* pcm_buf = NULL;

  if (argc < 2) {
    printf("usage: pi68k <file.s44|pcm>");
    goto exit;
  }

  uint8_t* file_name = argv[1];
  uint8_t* file_ext = file_name + strlen(file_name) - 4;

  if (!ras68k_pilib_keepchk()) {
    printf("error: PILIB.X is not running.\n");
    goto exit;
  }
  printf("PILIB.X is running.\n");

  if (ras68k_pilib_init_opm() != 0) {
    printf("error: ras68k OPM mode init error.\n");
    goto exit;
  }
  printf("init ras68k in OPM mode.\n");

  if (ras68k_pilib_set_pcm_filter(1) != 0) {
    printf("error: ras68k PCM filter set error.\n");
    goto exit;
  }
  printf("Enabled ras68k PCM filter.\n");

  if (ras68k_pilib_set_reverb_type(5) != 0) {
    printf("error: ras68k reverb type.\n");
    goto exit;
  }
  printf("Enabled ras68k reverb type.\n");

  if (stricmp(file_ext, ".pcm") != 0 && stricmp(file_ext, ".s44") != 0) {
    printf("error: unknown format data.\n");
    goto exit;
  }

  fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("error: file open error.\n");
    goto exit;
  }

  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  pcm_buf = himem_malloc(file_size, 0);
  if (pcm_buf == NULL) {
    printf("error: memory allocation error.\n");
    goto exit;
  }

  printf("Loading PCM data...\n");
  size_t read_len = 0;
  do {
    int32_t len = fread(pcm_buf + read_len, 1, file_size - read_len, fp);
    if (len <= 0) break;
    read_len += len;
  } while (read_len < file_size);

  fclose(fp);
  fp = NULL;

  printf("Uploading PCM data to ras68k...\n");
  if (ras68k_pilib_upload_pcm_data(pcm_buf, file_size) != 0) {
    printf("error: ras68k PCM data upload error.\n");
    goto exit;
  }

  uint32_t pcm_channel = 7;
  uint32_t pcm_volume = 0x08;
  uint32_t pcm_freq = stricmp(file_ext, ".pcm") == 0 ? 0x04 : 0x1d;
  uint32_t pcm_pan = 0x03;
  uint32_t pcm_mode = ( pcm_volume << 16 ) | ( pcm_freq << 8 ) | pcm_pan;

  printf("pcm play started.\n");
  if (ras68k_pilib_play_pcm(pcm_channel, pcm_mode, pcm_buf, file_size) != 0) {
    printf("error: pcm play error.\n");
    goto exit;
  }

  rc = 0;

exit:

  getchar();

  ras68k_pilib_stop_pcm_all();

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  if (pcm_buf != NULL) {
    himem_free(pcm_buf, 0);
    pcm_buf = NULL;
  }

  return rc;
}