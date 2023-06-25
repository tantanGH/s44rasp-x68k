#ifndef __H_S44RASP__
#define __H_S44RASP__

#define PROGRAM_VERSION "0.4.0 (2023/06/25)"

#define DEBUG (1)
#define DEBUG2 (1)

#define MAX_PATH_LEN   (256)
#define MAX_PCM_FILES  (256)

#define FORMAT_NONE    (-1)
#define FORMAT_ADPCM   (0)
#define FORMAT_RAW     (1)
#define FORMAT_YM2608  (2)
#define FORMAT_WAV     (3)

typedef struct {
  uint8_t* kmd_file_name;
  uint8_t* remote_pcm_path;
//  int16_t volume;
} PCM_FILE;

#endif