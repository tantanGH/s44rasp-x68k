#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "himem.h"
#include "raw_decode.h"

//
//  init raw pcm decoder handle
//
int32_t raw_decode_init(RAW_DECODE_HANDLE* pcm, int32_t sample_rate, int16_t channels, int16_t half_rate, int16_t auto_clip) {

  int32_t rc = -1;

  pcm->sample_rate = sample_rate;
  pcm->channels = channels;

  pcm->half_rate = half_rate;
  pcm->auto_clip = auto_clip;

  pcm->resample_counter = 0;
 
  rc = 0;

exit:
  return rc;
}

//
//  close decoder handle
//
void raw_decode_close(RAW_DECODE_HANDLE* pcm) {

}

//
//  execution with/without rate conversion
//
size_t raw_decode_exec(RAW_DECODE_HANDLE* pcm, int16_t* output_buffer, int16_t* source_buffer, size_t source_buffer_len) {

  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;
  size_t output_buffer_bytes = 0;

  int16_t* output_buffer_int16 = (int16_t*)output_buffer;

  if (pcm->channels == 2) {

    if (pcm->half_rate == 0) {

      //output_buffer_bytes = source_buffer_len * sizeof(int16_t);
      //memcpy((uint8_t*)output_buffer, (uint8_t*)source_buffer, output_buffer_bytes);

      while (source_buffer_ofs < source_buffer_len) {
        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
      }

      output_buffer_bytes = output_buffer_ofs * sizeof(int16_t);

    } else {

      while (source_buffer_ofs < source_buffer_len) {

        pcm->resample_counter++;
        if (!(pcm->resample_counter & 0x01)) {
          source_buffer_ofs += pcm->channels;
          continue;
        }

        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
      }

      output_buffer_bytes = output_buffer_ofs * sizeof(int16_t);

    }

  } else {

    if (pcm->half_rate == 0) {

      //output_buffer_bytes = source_buffer_len * sizeof(int16_t);
      //memcpy(output_buffer, source_buffer, output_buffer_bytes);

      while (source_buffer_ofs < source_buffer_len) {
        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
      }

      output_buffer_bytes = output_buffer_ofs * sizeof(int16_t);

    } else {

      while (source_buffer_ofs < source_buffer_len) {

        pcm->resample_counter++;
        if (!(pcm->resample_counter & 0x01)) {
          source_buffer_ofs += pcm->channels;
          continue;
        }

        output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ] / (pcm->auto_clip + 1);
      }

      output_buffer_bytes = output_buffer_ofs * sizeof(int16_t);

    }

  }

  return output_buffer_bytes;
}
