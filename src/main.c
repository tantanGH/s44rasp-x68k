#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <doslib.h>
#include <iocslib.h>

// devices
#include "keyboard.h"
#include "himem.h"

// drivers
#include "ras68k_pilib.h"

// decoders
#include "adpcm_decode.h"
#include "raw_decode.h"
#include "wav_decode.h"

// application
#include "s44rasp.h"

// pcm files
static PCM_FILE pcm_files[ MAX_PCM_FILES ];

// original function keys
static uint8_t funckey_original_settings[ 712 ];

// original function key mode
static int32_t funckey_mode = -1;

// abort vector handler
static void abort_application() {

  // reset pilib
  ras68k_pilib_init_opm();

  // cursor on
  C_CURON();

  // funckey mode
  if (funckey_mode >= 0) {
    C_FNKMOD(funckey_mode);
  }
  
  // resume function key settings
  FNCKEYST(0, funckey_original_settings);

  // flush key buffer
  while (B_KEYSNS() != 0) {
    B_KEYINP();
  }

  B_PRINT("aborted.\n");

  exit(1);
}

//
//  show help message
//
static void show_help_message() {
  printf("usage: s44p_ras [options] <input-file[.pcm|.sXX|.mXX|.wav]> [input-file..]\n");
  printf("options:\n");
  printf("       -v[n] ... volume (1-15, default:7)\n");
  printf("       -r[n] ... reverb type (0-7, default:2)\n");
  printf("       -q[n] ... quality (0:full, 1:half-rate, default:0)\n");
  printf("       -c[n] ... auto clipping (0:no-clip, 1:clip, default:1)\n");
  printf("\n");
  printf("       -i <indirect-file> ... indirect file\n");
  printf("       -l[n] ... loop count\n");
  printf("       -s    ... shuffle play\n");
  printf("\n");
  printf("       -b[n] ... buffer size (2-32, default:4)\n");
  printf("       -h    ... show help message\n");
}

//
//  main
//
int32_t main(int32_t argc, uint8_t* argv[]) {

  // default return code
  int rc = -1;

  // preserve original function key settings
  FNCKEYGT(0, funckey_original_settings);

  // credit
  printf("S44P_RAS.X - ADPCM/PCM/WAV player for ras68k-ext version " PROGRAM_VERSION " by tantan\n");

  // control parameters
  int16_t playback_volume = 7;
  int16_t half_rate = 0;
//  int16_t half_bits = 0;
  int16_t auto_clip = 1;
  int16_t reverb_type = REVERB_TYPE_STUDIO_SMALL;
  int16_t num_pcm_files = 0;
  int16_t loop_count = 1;
  int16_t shuffle_play = 0;
  int16_t num_chains = 4;
  int16_t pic_brightness = 75;
  uint8_t* indirect_file_names = NULL;

  // pcm buffers
  PCM_BUFFER* pcm_buffers = NULL;

  // parse command lines
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'v') {
        playback_volume = atoi(argv[i]+2);
        if (playback_volume < 1 || playback_volume > 15 || strlen(argv[i]) < 3) {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'l') {
        loop_count = atoi(argv[i]+2);
      } else if (argv[i][1] == 'q') {
        int16_t q = atoi(argv[i]+2);
        if (q == 0) {
          half_rate = 0;
        } else if (q == 1) {
          half_rate = 1;
        } else {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'c') {
        int16_t c = atoi(argv[i]+2);
        if (c == 0) {
          auto_clip = 0;
        } else if (c == 1) {
          auto_clip = 1;
        } else {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'r') {
        reverb_type = atoi(argv[i]+2);
        if (reverb_type < 0 || reverb_type > 7 || strlen(argv[i]) < 3) {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 's') {
        shuffle_play = 1;
      } else if (argv[i][1] == 'b') {
        num_chains = atoi(argv[i]+2);
        if (num_chains < 2 || num_chains > 32) {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'i' && i+1 < argc) {

        // indirect file
        int16_t count = 0;
        FILE* fp = fopen(argv[i+1], "r");
        if (fp != NULL) {

          // phase1: count lines
          static uint8_t line[ MAX_PATH_LEN + 1 ];
          while (fgets(line, MAX_PATH_LEN, fp) != NULL) {
            for (int16_t i = 0; i < MAX_PATH_LEN; i++) {
              if (line[i] <= ' ') {
                line[i] = '\0';
              }
            }
            if (strlen(line) < 5) continue;
            count++;
          }
          fclose(fp);
          fp = NULL;

          if (count > MAX_PCM_FILES) {
            printf("error: too many pcm files in the indirect file.\n");
            goto exit;
          }

          // phase2: read lines
          indirect_file_names = himem_malloc( MAX_PATH_LEN * count, 0 );
          fp = fopen(argv[i+1], "r");
          while (fgets(line, MAX_PATH_LEN, fp) != NULL) {
            for (int16_t i = 0; i < MAX_PATH_LEN; i++) {
              if (line[i] <= ' ') {
                line[i] = '\0';
              }
            }
            if (strlen(line) < 5) continue;
            int16_t volume = playback_volume;
            for (int16_t i = 0; i < MAX_PATH_LEN; i++) {
              if (line[i] == ',') {
                if (i+2 < MAX_PATH_LEN && line[i+1] == 'v') {
                  int16_t v = atoi(line+i+2);
                  if (v >= 1 && v <= 12) volume = v;
                }
                line[i] = '\0';
                break;
              }
            }
            pcm_files[ num_pcm_files ].file_name = indirect_file_names + MAX_PATH_LEN * num_pcm_files;
            strcpy(pcm_files[ num_pcm_files ].file_name, line);
            pcm_files[ num_pcm_files ].volume = volume;
            num_pcm_files++;
          }

          fclose(fp);
          fp = NULL;

        }
        i++;

      } else if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (strlen(argv[i]) >= 5) {
        pcm_files[ num_pcm_files ].file_name = argv[i];
        pcm_files[ num_pcm_files ].volume = playback_volume;
        num_pcm_files++;
      }
    }
  }

  // check PILIB.X
  if (!ras68k_pilib_keepchk()) {
    printf("error: PILIB.X is not running.\n");
    goto exit;
  }

  // ras68k-ext init OPM mode
  if (ras68k_pilib_init_opm() != 0) {
    printf("error: ras68k-ext OPM mode initialization error.\n");
    goto exit;
  }

  // ras68k-ext stop PCM
  if (ras68k_pilib_stop_pcm_all() != 0) {
    printf("error: ras68k-ext PCM stop error.\n");
    goto exit;
  }

  // ras68k-ext filter on
  if (ras68k_pilib_set_filter_mode(1) != 0) {
    printf("error: ras68k-ext set filter mode error.\n");
    goto exit;
  }

  // ras68k-ext set reverb type
  if (ras68k_pilib_set_reverb_type(reverb_type) != 0) {
    printf("error: ras68k reverb type.\n");
    goto exit;
  }

  // check number of PCM files
  if (num_pcm_files == 0) {
    show_help_message();
    goto exit;
  }

  // customize function keys
  uint8_t funckey_settings[ 712 ];
  memset(funckey_settings, 0, 712);
  funckey_settings[ 20 * 32 + 6 * 0 ] = '\x05';   // ROLLUP
  funckey_settings[ 20 * 32 + 6 * 1 ] = '\x15';   // ROLLDOWN
  funckey_settings[ 20 * 32 + 6 * 3 ] = '\x07';   // DEL
  funckey_settings[ 20 * 32 + 6 * 4 ] = '\x01';   // UP
  funckey_settings[ 20 * 32 + 6 * 5 ] = '\x13';   // LEFT
  funckey_settings[ 20 * 32 + 6 * 6 ] = '\x04';   // RIGHT
  funckey_settings[ 20 * 32 + 6 * 7 ] = '\x06';   // DOWN
  FNCKEYST(0, funckey_settings);

  if (shuffle_play) {
    // randomize
    srand(ONTIME());
    for (int16_t i = 0; i < num_pcm_files * 10; i++) {
      int16_t a = rand() % num_pcm_files;
      int16_t b = rand() % num_pcm_files;
      uint8_t* c = pcm_files[ a ].file_name;
      int16_t v = pcm_files[ a ].volume;
      pcm_files[ a ].file_name = pcm_files[ b ].file_name;
      pcm_files[ a ].volume = pcm_files[ b ].volume;
      pcm_files[ b ].file_name = c;
      pcm_files[ b ].volume = v;
    }
  }
  
#ifdef DEBUG2
  printf("ras68k-ext and pilib is ok.\n");
#endif

  // cursor off
  C_CUROFF();

  // set abort vectors
  uint32_t abort_vector1 = INTVCS(0xFFF1, (int8_t*)abort_application);
  uint32_t abort_vector2 = INTVCS(0xFFF2, (int8_t*)abort_application);  

  // playback controls
  uint8_t* pcm_file_name = NULL;
  int16_t playback_index = 0;
  int16_t first_play = 1;

loop:

  pcm_file_name   = pcm_files[ playback_index ].file_name;
  playback_volume = pcm_files[ playback_index ].volume;

#ifdef DEBUG2
  printf("file=%s, volume=%d\n", pcm_file_name, playback_volume);
#endif

  // input pcm file name and extension
  uint8_t* pcm_file_exp = pcm_file_name + strlen(pcm_file_name) - 4;

  // input format check
  int16_t input_format = FORMAT_NONE;
  int32_t pcm_freq = -1;
  int16_t pcm_channels = -1;
  if (stricmp(".pcm", pcm_file_exp) == 0) {
    input_format = FORMAT_ADPCM;
    pcm_freq = 15625;
    pcm_channels = 1;
  } else if (stricmp(".s32", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 32000;
    pcm_channels = 2;
  } else if (stricmp(".s44", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 44100;
    pcm_channels = 2;
  } else if (stricmp(".s48", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 48000;
    pcm_channels = 2;
  } else if (stricmp(".m32", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 32000;
    pcm_channels = 1;
  } else if (stricmp(".m44", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 44100;
    pcm_channels = 1;
  } else if (stricmp(".m48", pcm_file_exp) == 0) {
    input_format = FORMAT_RAW;
    pcm_freq = 48000;
    pcm_channels = 1;
  } else if (stricmp(".wav", pcm_file_exp) == 0) {
    input_format = FORMAT_WAV;
    pcm_freq = -1;              // not yet determined
    pcm_channels = -1;          // not yet determined
  } else {
    printf("error: unknown format file (%s).\n", pcm_file_name);
    goto exit;
  }

#ifdef DEBUG2
  printf("format=%d,freq=%d,channels=%d\n", input_format, pcm_freq, pcm_channels);
#endif

  // file read buffers
  void* fread_buffer = NULL;
  FILE* fp = NULL;

try:

  // decoders
  ADPCM_DECODE_HANDLE adpcm_decoder = { 0 };
  RAW_DECODE_HANDLE raw_decoder = { 0 };
  WAV_DECODE_HANDLE wav_decoder = { 0 };

  // init adpcm decoder if needed
  if (input_format == FORMAT_ADPCM) {
    if (adpcm_decode_init(&adpcm_decoder, pcm_freq, auto_clip) != 0) {
      printf("error: ADPCM decoder initialization error.\n");
      goto catch;
    }
  }

  // init raw pcm decoder if needed
  if (input_format == FORMAT_RAW) {
    if (raw_decode_init(&raw_decoder, pcm_freq, pcm_channels, half_rate, auto_clip) != 0) {
      printf("error: PCM decoder initialization error.\n");
      goto catch;
    }
  }

  // init wav decoder if needed
  if (input_format == FORMAT_WAV) {
    if (wav_decode_init(&wav_decoder, half_rate, auto_clip) != 0) {
      printf("error: WAV decoder initialization error.\n");
      goto catch;
    }
  }

#ifdef DEBUG2
  printf("decoder init ok.\n");
#endif

  // file size check
  struct FILBUF filbuf;
  if (FILES(&filbuf, pcm_file_name, 0x23) < 0) {
    printf("error: cannot open input file (%s).\n", pcm_file_name);
    goto catch;
  }
  uint32_t pcm_data_size = filbuf.filelen;
  int16_t pcm_file_group = abs( filbuf.name[0] * 3 + filbuf.name[1] * 7 + filbuf.name[2] ) % 3;

  // open input file
  fp = fopen(pcm_file_name, "rb");
  if (fp == NULL) {
    printf("error: cannot open input file (%s).\n", pcm_file_name);
    goto catch;
  }

  // read header part of WAV file
  size_t skip_offset = 0;
  if (input_format == FORMAT_WAV) {
    int32_t ofs = wav_decode_parse_header(&wav_decoder, fp);
    if (ofs < 0) {
      //printf("error: wav header parse error.\n");
      goto catch;
    }
    pcm_freq = wav_decoder.sample_rate;
    pcm_channels = wav_decoder.channels;
    skip_offset = ofs;
  }
  pcm_data_size -= skip_offset;

  // describe PCM attributes
  if (first_play) {

    printf("\n");

    printf("File name     : %s\n", pcm_file_name);
    printf("Data size     : %d [bytes]\n", pcm_data_size);
    printf("Data format   : %s\n", 
      input_format == FORMAT_WAV ? "WAV" :
      input_format == FORMAT_RAW ? "16bit signed raw PCM (big)" :
      "ADPCM(MSM6258V)");

    // describe playback drivers
    printf("PCM driver    : %s\n", "PILIB");
  
    if (input_format == FORMAT_ADPCM) {
      float pcm_1sec_size = pcm_freq * 0.5;
      printf("PCM frequency : %d [Hz]\n", pcm_freq);
      printf("PCM channels  : %s\n", "mono");
      printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_1sec_size);
//      if (use_kmd) {
//        if (kmd.tag_title[0]  != '\0') printf("KMD title     : %s\n", kmd.tag_title);
//        if (kmd.tag_artist[0] != '\0') printf("KMD artist    : %s\n", kmd.tag_artist);
//        if (kmd.tag_album[0]  != '\0') printf("KMD album     : %s\n", kmd.tag_album);
//      }
    }

    if (input_format == FORMAT_RAW) {
      float pcm_1sec_size = pcm_freq * 2;
      printf("PCM frequency : %d [Hz]\n", half_rate ? pcm_freq / 2 : pcm_freq);
      printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
      printf("PCM length    : %4.2f [sec]\n", (float)pcm_data_size / pcm_channels / pcm_1sec_size);
//      if (use_kmd) {
//        if (kmd.tag_title[0]  != '\0') printf("KMD title     : %s\n", kmd.tag_title);
//        if (kmd.tag_artist[0] != '\0') printf("KMD artist    : %s\n", kmd.tag_artist);
//        if (kmd.tag_album[0]  != '\0') printf("KMD album     : %s\n", kmd.tag_album);
//      }
    }

    if (input_format == FORMAT_WAV) {
      printf("PCM frequency : %d [Hz]\n", half_rate ? pcm_freq / 2 : pcm_freq);
      printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");
      printf("PCM length    : %4.2f [sec]\n", (float)wav_decoder.duration / pcm_freq);
//      if (use_kmd) {
//        if (kmd.tag_title[0]  != '\0') printf("KMD title     : %s\n", kmd.tag_title);
//        if (kmd.tag_artist[0] != '\0') printf("KMD artist    : %s\n", kmd.tag_artist);
//        if (kmd.tag_album[0]  != '\0') printf("KMD album     : %s\n", kmd.tag_album);
//      }
    }

    printf("Reverb type   : %s\n", 
            reverb_type == REVERB_TYPE_NO_REVERB     ? REVERB_STR_NO_REVERB     :
            reverb_type == REVERB_TYPE_ROOM          ? REVERB_STR_ROOM          :
            reverb_type == REVERB_TYPE_STUDIO_SMALL  ? REVERB_STR_STUDIO_SMALL  :
            reverb_type == REVERB_TYPE_STUDIO_MEDIUM ? REVERB_STR_STUDIO_MEDIUM :
            reverb_type == REVERB_TYPE_STUDIO_LARGE  ? REVERB_STR_STUDIO_LARGE  :
            reverb_type == REVERB_TYPE_HALL          ? REVERB_STR_HALL          :
            reverb_type == REVERB_TYPE_SPACE_ECHO    ? REVERB_STR_SPACE_ECHO    :
            reverb_type == REVERB_TYPE_HALF_ECHO     ? REVERB_STR_HALF_ECHO     : "???");

    printf("\n");

    first_play = 0;
  }

  // allocate file read buffer
  size_t fread_buffer_bytes = input_format == FORMAT_ADPCM ? 
          pcm_freq * pcm_channels * ( 2 + 2 * ( pcm_file_group % 2 )) * 4 / 8 :   // 4bit, 2sec or 4sec
          pcm_freq * pcm_channels * ( 3 + pcm_file_group ) * 16 / 8 / 2;          // 16bit, 1.5sec ~ 2.5sec
  fread_buffer = himem_malloc(fread_buffer_bytes, 0);
  if (fread_buffer == NULL) {
    printf("\rerror: file read buffer memory allocation error.\n");
    goto catch;
  }

  // pcm buffers
  pcm_buffers = (PCM_BUFFER*)himem_malloc(sizeof(PCM_BUFFER) * num_chains, 0);
  if (pcm_buffers == NULL) {
    printf("error: out of memory.\n");
    goto catch;
  }
  memset(pcm_buffers, 0, sizeof(PCM_BUFFER) * num_chains);
  for (int16_t i = 0; i < num_chains; i++) {
    size_t pcm_buffer_bytes = input_format == FORMAT_ADPCM ? 
                                fread_buffer_bytes * 4 :                // 4bit to 16bit 
                                fread_buffer_bytes / ( half_rate + 1 );
    pcm_buffers[i].buffer = himem_malloc(pcm_buffer_bytes, 0);
    if (pcm_buffers[i].buffer == NULL) {
      printf("error: out of memory.\n");
      goto catch;
    }
    pcm_buffers[i].buffer_bytes = 0;
  }

#ifdef DEBUG2
  printf("PCM buffer allocated.\n");
#endif

  // initial buffering
  int16_t end_flag = 0;
  for (int16_t i = 0; i < num_chains; i++) {

    if (end_flag) break;

    // check esc key to exit
    if (B_KEYSNS() != 0) {
      int16_t scan_code = B_KEYINP() >> 8;
      if (scan_code == KEY_SCAN_CODE_ESC || scan_code == KEY_SCAN_CODE_Q) {
        printf("\rcanceled.\x1b[0K");
        rc = 1;
        goto catch;
      }
    }

    printf("\rnow buffering (%d/%d) ...", i+1, num_chains); 
 
    if (input_format == FORMAT_ADPCM) {

      // ADPCM(MSM6258V)
      uint8_t* fread_buffer_uint8 = (uint8_t*)fread_buffer;
      size_t fread_buffer_len = fread_buffer_bytes;
      size_t fread_len = 0;
      do {
        size_t len = fread(fread_buffer_uint8 + fread_len, sizeof(uint8_t), fread_buffer_len - fread_len, fp);
        if (len == 0) break;
        fread_len += len;
      } while (fread_len < fread_buffer_len);
      if (fread_len < fread_buffer_len) {
        end_flag = 1;
      }
      pcm_buffers[i].buffer_bytes = adpcm_decode_exec(&adpcm_decoder, pcm_buffers[i].buffer, fread_buffer_uint8, fread_len);
  
#ifdef DEBUG
      printf("i=%d, fread_len=%d, buffer=%x, buffer_bytes=%d\n", i, fread_len, pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes);
#endif

    } else if (input_format == FORMAT_RAW) {

      // raw signed 16bit PCM
      int16_t* fread_buffer_int16 = (int16_t*)fread_buffer;
      size_t fread_buffer_len = fread_buffer_bytes / sizeof(int16_t);
      size_t fread_len = 0;
      do {
        size_t len = fread(fread_buffer_int16 + fread_len, sizeof(int16_t), fread_buffer_len - fread_len, fp);
        if (len == 0) break;
        fread_len += len;
      } while (fread_len < fread_buffer_len);
      if (fread_len < fread_buffer_len) {
        end_flag = 1;
      }
      pcm_buffers[i].buffer_bytes = raw_decode_exec(&raw_decoder, pcm_buffers[i].buffer, fread_buffer_int16, fread_len);

#ifdef DEBUG
      printf("i=%d, fread_len=%d, buffer=%x, buffer_bytes=%d\n", i, fread_len, pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes);
#endif

    } else if (input_format == FORMAT_WAV) {

      // WAV
      int16_t* fread_buffer_int16 = (int16_t*)fread_buffer;
      size_t fread_buffer_len = fread_buffer_bytes / sizeof(int16_t);
      size_t fread_len = 0;
      do {
        size_t len = fread(fread_buffer_int16 + fread_len, sizeof(int16_t), fread_buffer_len - fread_len, fp);
        if (len == 0) break;
        fread_len += len;
      } while (fread_len < fread_buffer_len);
      if (fread_len < fread_buffer_len) {
        end_flag = 1;
      }
      pcm_buffers[i].buffer_bytes = wav_decode_exec(&wav_decoder, pcm_buffers[i].buffer, fread_buffer_int16, fread_len);

#ifdef DEBUG
      printf("i=%d, fread_len=%d, buffer=%x, buffer_bytes=%d\n", i, fread_len, pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes);
#endif

    }

    // upload to ras68k-ext
#ifdef DEBUG2
    printf("uploading to ras68k-ext... %x, %d\n", pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes);
#endif
    if (pcm_buffers[i].buffer_bytes > 0) {
      if (ras68k_pilib_upload_pcm_data(pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes) != 0) {
        printf("error: ras68k PCM data upload error.\n");
        goto exit;
      } else {
        printf("uploaded.\n");
      }
    }
#ifdef DEBUG3
    uint32_t hash_sum = 0;
    for (size_t j = 0; j < pcm_buffers[i].buffer_bytes; j++) {
      hash_sum += ((uint8_t*)pcm_buffers[i].buffer)[j];
    }
    printf("hash=%08X, end_flag=%d\n", hash_sum, end_flag);
#endif
  }

  uint16_t pilib_freq = 0x04;
  
  if (half_rate) {

    if (pcm_channels == 1) {
      pilib_freq =  pcm_freq == 15625 ? 0x08 :
                    pcm_freq == 32000 ? 0x09 :
                    pcm_freq == 44100 ? 0x0a :
                    pcm_freq == 48000 ? 0x0b : 0x04;
    } else {
      pilib_freq =  pcm_freq == 32000 ? 0x19 :
                    pcm_freq == 44100 ? 0x1a :
                    pcm_freq == 48000 ? 0x1b : 0x04;
    }

  } else {

    if (pcm_channels == 1) {
      pilib_freq =  pcm_freq == 15625 ? 0x08 : 
                    pcm_freq == 32000 ? 0x0c :
                    pcm_freq == 44100 ? 0x0d :
                    pcm_freq == 48000 ? 0x0e : 0x04;
    } else {
      pilib_freq =  pcm_freq == 32000 ? 0x1c :
                    pcm_freq == 44100 ? 0x1d :
                    pcm_freq == 48000 ? 0x1e : 0x04;
    }

  }
  
  uint16_t pilib_pan = 0x03;
  uint32_t pilib_mode = ( playback_volume << 16 ) | ( pilib_freq << 8 ) | pilib_pan;

#ifdef DEBUG
  printf("volume=%x, freq=%x, pan=%x, mode=%x\n", playback_volume, pilib_freq, pilib_pan, pilib_mode);
#endif

  B_PRINT("\rnow playing ... push [ESC]/[Q] key to quit. [SPACE] to pause. [RIGHT] to skip.\x1b[0K");
  int16_t paused = 0;
  uint32_t pause_time;

  for (int16_t i = 0; i < num_chains; i++) {
#ifdef DEBUG
    printf("i=%d, buffer=%x, buffer_bytes=%d\n", i, pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes);
#endif
    if (pcm_buffers[i].buffer_bytes > 0) {
      if (ras68k_pilib_play_pcm8pp(PILIB_CHANNEL, pilib_mode, half_rate ? pcm_freq * 256 / 2 : pcm_freq * 256, pcm_buffers[i].buffer, pcm_buffers[i].buffer_bytes) != 0) {
        printf("error: pcm8pp play error.\n");
        goto catch;
      }
    }

    getchar();
  }

catch:

  // ras68k-ext stop PCM
  if (ras68k_pilib_stop_pcm_all() != 0) {
    printf("error: ras68k-ext PCM stop error.\n");
    goto exit;
  }

  // close adpcm decoder
  if (input_format == FORMAT_ADPCM) {
    adpcm_decode_close(&adpcm_decoder);
  }

  // close raw decoder
  if (input_format == FORMAT_RAW) {
    raw_decode_close(&raw_decoder);
  }

  // close wav decoder
  if (input_format == FORMAT_WAV) {
    wav_decode_close(&wav_decoder);
  }

#ifdef DEBUG
  printf("closed decoders.\n");
#endif

  // close pcm buffer
  if (pcm_buffers != NULL) {
    for (int16_t i = 0; i < num_chains; i++) {
      if (pcm_buffers[i].buffer != NULL) {
        himem_free(pcm_buffers[i].buffer, 0);
        pcm_buffers[i].buffer = NULL;
      }
    }
    himem_free(pcm_buffers, 0);
    pcm_buffers = NULL;
  }

#ifdef DEBUG
  printf("reclaimed buffer memories.\n");
#endif

exit:

  // cursor on
  C_CURON();

  // function key mode
  if (funckey_mode >= 0) {
    C_FNKMOD(funckey_mode);
  }

  // resume function key settings
  FNCKEYST(0, funckey_original_settings);

  // resume abort vectors
  INTVCS(0xFFF1, (int8_t*)abort_vector1);
  INTVCS(0xFFF2, (int8_t*)abort_vector2);  

  return rc;
}