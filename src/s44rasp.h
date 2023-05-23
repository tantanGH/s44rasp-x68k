#ifndef __H_S44RASP__
#define __H_S44RASP__

#define PROGRAM_VERSION "0.3.0 (2023/05/23)"

#define DEBUG (1)
#define DEBUG2 (1)

#define MAX_PATH_LEN   (256)
#define MAX_PCM_FILES  (256)

#define PILIB_CHANNEL  (1)

#define FORMAT_NONE    (0)
#define FORMAT_ADPCM   (1)
#define FORMAT_RAW     (2)
#define FORMAT_WAV     (3)
#define FORMAT_YM2608  (4)

#define NUM_REVERB_TYPES (8)

#define REVERB_TYPE_NO_REVERB     (0)
#define REVERB_TYPE_ROOM          (1)
#define REVERB_TYPE_STUDIO_SMALL  (2)
#define REVERB_TYPE_STUDIO_MEDIUM (3)
#define REVERB_TYPE_STUDIO_LARGE  (4)
#define REVERB_TYPE_HALL          (5)
#define REVERB_TYPE_SPACE_ECHO    (6)
#define REVERB_TYPE_HALF_ECHO     (7)

//                                 012345678901234
#define REVERB_STR_NO_REVERB      "0.NO REVERB    "
#define REVERB_STR_ROOM           "1.ROOM         "
#define REVERB_STR_STUDIO_SMALL   "2.STUDIO SMALL "
#define REVERB_STR_STUDIO_MEDIUM  "3.STUDIO MEDIUM"
#define REVERB_STR_STUDIO_LARGE   "4.STUDIO LARGE "
#define REVERB_STR_HALL           "5.HALL         "
#define REVERB_STR_SPACE_ECHO     "6.SPACE ECHO   "
#define REVERB_STR_HALF_ECHO      "7.HALF ECHO    "

typedef struct {
  uint8_t* file_name;
  int16_t volume;
} PCM_FILE;

typedef struct {
  void* buffer;
  size_t buffer_bytes;
} PCM_BUFFER;

#endif