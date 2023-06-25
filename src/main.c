// base
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <jstring.h>

// dos/iocs call
#include <doslib.h>
#include <iocslib.h>

// devices
#include "keyboard.h"
#include "himem.h"
#include "crtc.h"

// uart
#include "uart.h"
#include "rss.h"

// kmd
#include "bmp_decode.h"
#include "kmd.h"

// application
#include "s44rasp.h"

// for play list
static PCM_FILE pcm_files[ MAX_PCM_FILES ];

// original function keys
static uint8_t funckey_original_settings[ 712 ];

// abort vector handler
static volatile int32_t g_abort_flag = 0;
static void abort_application() {
  g_abort_flag = 1;
}

//
//  timer-D interrupt handler
//
static uint32_t g_play_time_msec = 0;
static void __attribute__((interrupt)) __timer_d_interrupt_handler__(void) {
  g_play_time_msec += 10;
}

//
//  show help message
//
static void show_help_message() {
  printf("S44RASP.X - S44/A44 PCM player over UART version " PROGRAM_VERSION " by tantan\n");
  printf("usage: s44rasp [options] <remote-pcm-path> [<remote-pcm-path> ...]\n");
  printf("       s44rasp [options] -k <kmd-file> [<kmd-file> ...]\n");
  printf("       s44rasp [options] -i <indirect-file>\n");
  printf("options:\n");
  printf("       -l<n> ... loop count (default:1)\n");
  printf("       -s    ... shuffle mode\n");
  printf("       -t<n> ... album artwork brightness (default:75)\n");
  printf("       -b<n> ... baud rate (default:38400)\n");
  printf("       -h    ... show help message\n");
}

//
//  main
//
int32_t main(int32_t argc, uint8_t* argv[]) {

  // default return code
  int rc = -1;

  // baud rate
  int32_t baud_rate = 38400;

  // timeout
  int32_t timeout = 60;

  // control parameters
  int16_t pic_brightness = 75;
  int16_t loop_count = 1;
  int16_t shuffle_play = 0;

  // play list
  uint8_t* indirect_file_buffer = NULL;
  int16_t num_pcm_files = 0;

  // kmd artwork displayed?
  int16_t kmd_artwork = 0;

  // uart instance
  UART uart = { 0 };

  // rss instance
  RSS rss = { 0 };

  // preserve original function key settings
  FNCKEYGT(0, funckey_original_settings);
  int32_t funckey_mode = C_FNKMOD(-1);

  // set abort vectors
  uint32_t abort_vector1 = INTVCS(0xFFF1, (int8_t*)abort_application);
  uint32_t abort_vector2 = INTVCS(0xFFF2, (int8_t*)abort_application);  

  // for supervisor
  int32_t ssp = 0;

  // parse command lines
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 't') {
        pic_brightness = atoi(argv[i]+2);
        if (pic_brightness < 0 || pic_brightness > 100 || strlen(argv[i]) < 3) {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'b') {
        baud_rate = atoi(argv[i]+2);
        if (baud_rate != 9600 && baud_rate != 19200 && baud_rate != 38400) {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'l') {
        loop_count = atoi(argv[i]+2);
      } else if (argv[i][1] == 's') {
        shuffle_play = 1;
      } else if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else if (argv[i][1] == 'k' && i+1 < argc) {
        if (num_pcm_files > 0) {
          show_help_message();
          goto exit;
        }
        for (int16_t j = i + 1; j < argc; j++) {
          pcm_files[ num_pcm_files ].kmd_file_name = argv[j];
          pcm_files[ num_pcm_files ].remote_pcm_path = NULL;
          num_pcm_files++;
          i++;
        }
      } else if (argv[i][1] == 'i' && i+1 < argc) {

        if (num_pcm_files > 0) {
          show_help_message();
          goto exit;
        }

        uint8_t line[ MAX_PATH_LEN * 2 + 1 ];
        int16_t count = 0;
        FILE* fp = fopen(argv[i+1], "r");
        if (fp != NULL) {
          while (fgets(line, MAX_PATH_LEN * 2 , fp) != NULL) {
            for (int16_t j = 0; j < MAX_PATH_LEN * 2; j++) {
              if (line[j] <= ' ') {
                line[j] = '\0';
              }
            }
            if (strlen(line) < 5) continue;
            count++;
          }
          fclose(fp);
          fp = NULL;
        } else {
          printf("error: indirect file open error.\n");
          goto exit;
        }

        if (count > MAX_PCM_FILES) {
          printf("error: too many pcm files in the indirect file.\n");
          goto exit;
        }
        indirect_file_buffer = himem_malloc(MAX_PATH_LEN * 2 * count, 0);
        if (indirect_file_buffer == NULL) {
          printf("error: memory allocation error.\n");
          goto exit;
        }
        memset(indirect_file_buffer, 0, MAX_PATH_LEN * 2 * count);

        fp = fopen(argv[i+1], "r");
        if (fp != NULL) {
          while (fgets(line, MAX_PATH_LEN * 2, fp) != NULL) {
            for (int16_t j = 0; j < MAX_PATH_LEN * 2; j++) {
              if (line[j] <= ' ') {
                line[j] = '\0';
              }
            }
            if (strlen(line) < 5) continue;
            uint8_t* buf = indirect_file_buffer + MAX_PATH_LEN * 2 * num_pcm_files;
            strcpy(buf, line);
            uint8_t* c = strchr(buf, ',');
            if (c != NULL) {
              *c = '\0';
              pcm_files[ num_pcm_files ].kmd_file_name = buf;
              pcm_files[ num_pcm_files ].remote_pcm_path = c + 1;
            } else {
              pcm_files[ num_pcm_files ].kmd_file_name = buf;
              pcm_files[ num_pcm_files ].remote_pcm_path = NULL;
            }
            num_pcm_files++;
          }
          fclose(fp);
          fp = NULL;
          printf("Loaded %d files from the indirect file.\n", num_pcm_files);
        }

        i++;

      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      pcm_files[ num_pcm_files ].kmd_file_name = NULL;
      pcm_files[ num_pcm_files ].remote_pcm_path = argv[i];
      num_pcm_files++;
    }
  }

  if (num_pcm_files == 0) {
    show_help_message();
    goto exit;
  }

  // open uart  
  if (uart_open(&uart, baud_rate, timeout) != 0) {
    goto exit;
  }

  // open rss
  if (rss_open(&rss) != 0) {
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

  // cursor off
  C_CUROFF();

  // function key off
  C_FNKMOD(3);

  // graphic clear and on
  CRTMOD(16);

  // main loop
  int16_t playback_index = 0;
  while (playback_index < num_pcm_files) {

    G_CLR_ON();

    KMD_HANDLE kmd = { 0 };

    uint8_t* kmd_file_name = pcm_files[ playback_index ].kmd_file_name;
    uint8_t* remote_pcm_path = pcm_files[ playback_index ].remote_pcm_path;

    int16_t use_kmd = kmd_file_name != NULL ? 1 : 0;

    if (use_kmd) {
      FILE* fp_kmd = fopen(kmd_file_name, "r");
      if (fp_kmd == NULL) {
        printf("error: kmd file open error. (%s)\n", kmd_file_name);
        goto exit;
      }
      kmd_init(&kmd, fp_kmd, 1, 0);
      fclose(fp_kmd);

      // overwrite remote path
      if (kmd.tag_remote_path[0] != '\0') {
        if (remote_pcm_path == NULL || remote_pcm_path[0] == '\0') {
          remote_pcm_path = kmd.tag_remote_path;
        }
      }
    }

    if (remote_pcm_path == NULL) {
      printf("error: remote pcm path is not specified.\n");
      goto exit;
    }

    // input pcm file name and extension
    uint8_t* pcm_file_exp = remote_pcm_path + strlen(remote_pcm_path) - 4;

    // input format check
    int16_t input_format = FORMAT_NONE;
    int32_t pcm_freq = -1;
    int16_t pcm_channels = -1;
    if (stricmp(".s32", pcm_file_exp) == 0) {
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
    } else if (stricmp(".a32", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 32000;
      pcm_channels = 2;
    } else if (stricmp(".a44", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 44100;
      pcm_channels = 2;
    } else if (stricmp(".a48", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 48000;
      pcm_channels = 2;
    } else if (stricmp(".n32", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 32000;
      pcm_channels = 1;
    } else if (stricmp(".n44", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 44100;
      pcm_channels = 1;
    } else if (stricmp(".n48", pcm_file_exp) == 0) {
      input_format = FORMAT_YM2608;
      pcm_freq = 48000;
      pcm_channels = 1;
    } else {
      printf("error: unknown format file (%s).\n", remote_pcm_path);
      goto exit;
    }

    // KMD artwork
    if (use_kmd && kmd.tag_artwork[0] != '\0') {

      if (ssp == 0) {
        ssp = B_SUPER(0);
      }

      // 768(512)x512x65536
      crtc_set_extra_mode(0);

      C_CLS_AL();

      // fill text screen for mask
      TPALET2(4, 0x0001);
      TPALET2(5, TPALET2(1,-1));
      TPALET2(6, TPALET2(2,-1));
      TPALET2(7, TPALET2(3,-1));
      struct TXFILLPTR txfil = { 2, 0, 0, 768, 512, 0xffff };
      TXFILL(&txfil);

      printf("\rloading KMD album artwork...");
      static uint8_t artwork_file_name[ MAX_PATH_LEN ];
      artwork_file_name[0] = '\0';
      if (jstrchr(kmd.tag_artwork, '\\') != NULL || jstrchr(kmd.tag_artwork, '/') != NULL || jstrchr(kmd.tag_artwork, ':') != NULL) {
        strcpy(artwork_file_name, kmd.tag_artwork);
      } else {
        strcpy(artwork_file_name, kmd_file_name);
        uint8_t* c = jstrrchr(artwork_file_name, '\\');
        if (c == NULL) c = jstrrchr(artwork_file_name, '/');
        if (c == NULL) c = jstrrchr(artwork_file_name, ':');
        if (c != NULL) {
          strcpy(c + 1, kmd.tag_artwork);
        } else {
          strcpy(artwork_file_name, kmd.tag_artwork);
        }
      }

      FILE* fp_art = fopen(artwork_file_name, "rb");
      if (fp_art != NULL) {
        fseek(fp_art, 0, SEEK_END);
        size_t pic_data_len = ftell(fp_art);
        fseek(fp_art, 0, SEEK_SET);
        uint8_t* pic_data = (uint8_t*)himem_malloc(pic_data_len, 0);
        if (pic_data != NULL) {
          size_t read_len = 0;
          do {
            size_t len = fread(pic_data + read_len, sizeof(uint8_t), pic_data_len - read_len, fp_art);
            if (len == 0) break;
            read_len += len;
          } while (read_len < pic_data_len);
          if (read_len >= pic_data_len) {
            if (pic_data[0] == 0x42 && pic_data[1] == 0x4d) {
              BMP_DECODE_HANDLE bmp_decode;
              bmp_decode_init(&bmp_decode, pic_brightness, 0);
              if (bmp_decode_exec(&bmp_decode, pic_data, pic_data_len) == 0) {
                SCROLL(0, 512-128, 0);
                SCROLL(1, 512-128, 0);
                SCROLL(2, 512-128, 0);
                SCROLL(3, 512-128, 0);
                struct TXFILLPTR txfil = { 2, 128, 0, 512, 512, 0x0000 };
                TXFILL(&txfil);
                kmd_artwork = 1;
              }
              bmp_decode_close(&bmp_decode);
            }
          }
          himem_free(pic_data, 0);
        }
        fclose(fp_art);
      }

      printf("\r\x1b[0K");
    }

    // check remote pcm existence and get size
    uint32_t remote_pcm_size = 0;
    int32_t rss_result = rss_head_pcm(&rss, remote_pcm_path, &uart, &remote_pcm_size);

    // check communication result
    if (rss_result == UART_QUIT || rss_result == UART_EXIT) {
      printf("\rerror: canceled.\n");
      break;
    } else if (rss_result == UART_TIMEOUT) { 
      printf("\rerror: timeout.\n");
      break;
    } else if (rss_result == 404) { 
      printf("\rerror: not found.\n");
      goto skip;
    } else if (rss_result != 200) {
      printf("\rerror: communication error.\n");
      break;
    }

    printf("\n");

    printf("File name     : %s\n", remote_pcm_path);
    printf("Data format   : %s\n", 
      input_format == FORMAT_YM2608 ? "4/16bit YM2608 ADPCM" : "16bit raw (big endian)");

    // describe playback drivers
    printf("PCM driver    : %s\n", "UART RSSN");
    printf("PCM frequency : %d [Hz]\n", pcm_freq);
    printf("PCM channels  : %s\n", pcm_channels == 1 ? "mono" : "stereo");

    float pcm_1sec_size = pcm_freq * (input_format == FORMAT_YM2608 ? 0.5 : 2.0);
    uint32_t total_play_msec = (float)remote_pcm_size * 1000.0 / pcm_channels / pcm_1sec_size;
    printf("PCM length    : %4.2f [sec]\n", total_play_msec / 1000.0);

    if (use_kmd) {
      if (kmd.tag_title[0]  != '\0') printf("KMD title     : %s\n", kmd.tag_title);
      if (kmd.tag_artist[0] != '\0') printf("KMD artist    : %s\n", kmd.tag_artist);
      if (kmd.tag_album[0]  != '\0') printf("KMD album     : %s\n", kmd.tag_album);
    }

    printf("\n");

    // play remote PCM
    rss_result = rss_play_pcm(&rss, remote_pcm_path, &uart);

    printf("\rnow playing ... push [ESC]/[Q] key to quit.\x1b[0K");
    if (use_kmd) {
      printf("\n\n");
      kmd_preserve_cursor_position(&kmd);
    }

    // check communication result
    if (rss_result == UART_QUIT || rss_result == UART_EXIT) {
      printf("\rerror: canceled.\n");
      break;
    } else if (rss_result == UART_TIMEOUT) { 
      printf("\rerror: timeout.\n");
      break;
    } else if (rss_result == 404) { 
      printf("\rerror: not found.\n");
      goto skip;
    } else if (rss_result != 200) {
      printf("\rerror: communication error.\n");
      break;
    }

    g_play_time_msec = 0;
    if (TIMERDST((uint8_t*)(__timer_d_interrupt_handler__), 7, 200) != 0) {
      printf("\rerror: timer-D is being used by other applications.\n");
      goto exit;
    }

    g_abort_flag = 0;
    
    while (g_play_time_msec < total_play_msec) {

      // check global abort flag
      if (g_abort_flag) {
        rc = 1;
        break;
      }

      // check esc key to exit
      if (B_KEYSNS() != 0) {
        int16_t scan_code = B_KEYINP() >> 8;
        if (scan_code == KEY_SCAN_CODE_ESC || scan_code == KEY_SCAN_CODE_Q) {
          B_PRINT("\n\n");
          B_PRINT("\rstopped.\x1b[0K");
          rc = 1;
          break;
        } else if (scan_code == KEY_SCAN_CODE_RIGHT) {
          B_PRINT("\n\n");
          B_PRINT("\rskipped.\x1b[0K\n");
          rc = 2;
          break;
        } else if (playback_index > 0 && scan_code == KEY_SCAN_CODE_LEFT) {
          B_PRINT("\n\n");
          B_PRINT("\rbacked.\x1b[0K\n");
          rc = 3;
          break;
        } else if (scan_code == KEY_SCAN_CODE_UP) {
          B_PRINT("\n\n");
          B_PRINT("\rrestarted.\x1b[0K\n");
          rc = 4;
          break;
        }
      }

      // kmd display
      if (use_kmd) {
        kmd_deactivate_events(&kmd, g_play_time_msec);
        kmd_activate_current_event(&kmd, g_play_time_msec);
      }

    }

    // close kmd
    if (use_kmd) {
      kmd_close(&kmd);
    }

    // stop playing
    rss_stop_pcm(&rss, &uart);

    // stop timer-D handling
    TIMERDST(0,0,0);

    if (rc == 1) {
      break;
    } else if (rc == 2) {
      playback_index ++;
      continue;
    } else if (rc == 3) {
      if (playback_index > 0) {
        playback_index --;
      }
      continue;
    } else if (rc == 4) {
      continue;
    }

skip:
    playback_index ++;
  }

exit:

  // reset scroll position
  if (kmd_artwork) {
    SCROLL(0, 0, 0);
    SCROLL(1, 0, 0);
    SCROLL(2, 0, 0);
    SCROLL(3, 0, 0);

    struct TXFILLPTR txfil = { 2, 0, 0, 768, 512, 0x0000 };
    TXFILL(&txfil);

    TPALET2(4,-2);
    TPALET2(5,-2);
    TPALET2(6,-2);
    TPALET2(7,-2);

  }

  // reclaim buffer
  if (indirect_file_buffer != NULL) {
    himem_free(indirect_file_buffer, 0);
    indirect_file_buffer = NULL;
  }

  // close rss
  rss_close(&rss);

  // close uart
  uart_close(&uart);

  // flush key buffer
  while (B_KEYSNS() != 0) {
    B_KEYINP();
  }

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