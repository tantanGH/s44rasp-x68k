#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "himem.h"
#include "raw_decode.h"

//
//  init raw pcm decoder handle
//
int32_t raw_decode_init(RAW_DECODE_HANDLE* pcm, int32_t sample_rate, int16_t channels, int16_t half_rate, int16_t half_bits) {

  int32_t rc = -1;

  pcm->sample_rate = sample_rate;
  pcm->channels = channels;

  pcm->half_rate = sample_rate >= 32000 ? half_rate : 0;
  pcm->half_bits = half_bits;
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
//  execution with/without rate and bits conversion
//
size_t raw_decode_exec(RAW_DECODE_HANDLE* pcm, void* output_buffer, int16_t* source_buffer, size_t source_buffer_len) {

  size_t source_buffer_ofs = 0;
  size_t output_buffer_ofs = 0;
  
  if (pcm->channels == 2) {

    // stereo

    if (pcm->half_rate == 0) {

      if (pcm->half_bits == 0) {

        // copy only
        memcpy(output_buffer, source_buffer, sizeof(int16_t) * source_buffer_len);
        output_buffer_ofs = sizeof(int16_t) * source_buffer_len;

      } else {

        // signed 16bit to singed 8bit
        int8_t* output_buffer_int8 = (int8_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {
          int8_t lch = source_buffer[ source_buffer_ofs++ ] / 256;
          int8_t rch = source_buffer[ source_buffer_ofs++ ] / 256;
          output_buffer_int8[ output_buffer_ofs++ ] = lch;
          output_buffer_int8[ output_buffer_ofs++ ] = rch;
        }

      }

    } else {

      if (pcm->half_bits == 0) {
 
        // half sampling only
        int16_t* output_buffer_int16 = (int16_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {

          // half sampling
          pcm->resample_counter++;
          if (!(pcm->resample_counter & 0x01)) {
            source_buffer_ofs += pcm->channels;
            continue;
          }

          output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ];
          output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ];
        }
 
      } else {

        // half sampling only
        int16_t* output_buffer_int16 = (int16_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {

          // half sampling
          pcm->resample_counter++;
          if (!(pcm->resample_counter & 0x01)) {
            source_buffer_ofs += pcm->channels;
            continue;
          }

          int8_t lch = source_buffer[ source_buffer_ofs++ ] / 256;
          int8_t rch = source_buffer[ source_buffer_ofs++ ] / 256;
          output_buffer_int16[ output_buffer_ofs++ ] = lch;
          output_buffer_int16[ output_buffer_ofs++ ] = rch;
        }

      }

    }

  } else {

    // mono

    if (pcm->half_rate == 0) {

      if (pcm->half_bits == 0) {

        // copy only
        memcpy(output_buffer, source_buffer, sizeof(int16_t) * source_buffer_len);
        output_buffer_ofs = sizeof(int16_t) * source_buffer_len;

      } else {

        // signed 16bit to singed 8bit
        int8_t* output_buffer_int8 = (int8_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {
          int8_t mch = source_buffer[ source_buffer_ofs++ ] / 256;
          output_buffer_int8[ output_buffer_ofs++ ] = mch;
        }

      }

    } else {

      if (pcm->half_bits == 0) {
 
        // half sampling only
        int16_t* output_buffer_int16 = (int16_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {

          // half sampling
          pcm->resample_counter++;
          if (!(pcm->resample_counter & 0x01)) {
            source_buffer_ofs += pcm->channels;
            continue;
          }

          output_buffer_int16[ output_buffer_ofs++ ] = source_buffer[ source_buffer_ofs++ ];
        }
 
      } else {

        // half sampling only
        int16_t* output_buffer_int16 = (int16_t*)output_buffer;
        while (source_buffer_ofs < source_buffer_len) {

          // half sampling
          pcm->resample_counter++;
          if (!(pcm->resample_counter & 0x01)) {
            source_buffer_ofs += pcm->channels;
            continue;
          }

          int8_t mch = source_buffer[ source_buffer_ofs++ ] / 256;
          output_buffer_int16[ output_buffer_ofs++ ] = mch;
        }

      }

    }

  }

  return output_buffer_ofs;
}
