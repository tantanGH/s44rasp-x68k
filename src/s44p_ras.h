#ifndef __H_S44P_RAS__
#define __H_S44P_RAS__

#define PROGRAM_VERSION "0.1.0 (2023/05/19)"

#define MAX_PATH_LEN (256)
#define MAX_PCM_FILES (256)

#define PCM_BUFFER_BYTES (0xff00)
#define PILIB_CHANNEL (6)

#define FORMAT_NONE    (0)
#define FORMAT_ADPCM   (1)
#define FORMAT_RAW     (2)
#define FORMAT_WAV     (3)
#define FORMAT_YM2608  (4)

#define DEBUG (1)

typedef struct {
  uint8_t* file_name;
  int16_t volume;
} PCM_FILE;

typedef struct {
  void* buffer;
  size_t buffer_bytes;
} PCM_BUFFER;

#endif