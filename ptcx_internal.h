
/*
// Copyright (c) 2002-2014 Joe Bertolami. All Right Reserved.
//
// ptcx_internal.h
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Additional Information:
//
//   For more information, visit http://www.bertolami.com.
*/

#ifndef __PTCX_INTERNAL_H__
#define __PTCX_INTERNAL_H__

#include "ptcx.h"
#include "math.h"

#define PTCX_MAJOR_VERSION                       (2)
#define PTCX_MAGIC_VALUE                         (0x50544358)   // "PTCX"
#define PTCX_MAX_BLOCK_SIZE                      (16)
#define PTCX_MAX_QUANT_CONTROL_BITS              (16)
#define PTCX_DEFAULT_IMAGE_DEPTH                 (1)
#define PTCX_MAX_QUANT_STEP_BITS                 (4)
#define PTCX_QUALITY_DELTA                       (64.0f)
#define PTCX_MAX_MB_TABLE_SIZE                   (PTCX_MAX_BLOCK_SIZE * PTCX_MAX_BLOCK_SIZE)
#define PTCX_MAX_BLOCK_DATA_SIZE                 ((PTCX_MAX_MB_TABLE_SIZE * PTCX_MAX_QUANT_STEP_BITS + \
                                                 (PTCX_MAX_QUANT_CONTROL_BITS << 1)) >> 3)

#if (0 == (PTCX_MAX_BLOCK_SIZE >> 3))
  #error "Maximum block size is too small"
#endif

#define PTCX_SHOW_BLOCK_MAP                      (0)     // enable this to display the block map
#define PTCX_SHOW_RANGE_MAP                      (0)     // enable this to display the range estimation map

#pragma pack( push )
#pragma pack( 1 )

typedef struct PTCX_FILE_HEADER 
{
    uint32 magic;
    uint16 version;
    uint16 header_size;
    uint16 image_width;
    uint16 image_height;
    uint16 image_depth;
    uint16 block_width;
    uint16 block_height;
    uint8 quant_step_bits;                      // the number of lerp steps in between the quantization base colors 
    uint8 quant_control_bits;                   // control bit count -- this defines the precision of the quantization base colors
    uint32 source_format;                       // source format -- dictates reconstituted format

} PTCX_FILE_HEADER;

typedef struct PTCX_PIXEL_RANGE
{
    uint8 min_value[3];
    uint8 max_value[3];

} VN_PTCX_PIXEL_RANGE;

#pragma pack(pop)

status range_estimate_min_max(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y);
status range_estimate_linear_distance(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y);
status range_estimate_regression(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y);

#endif // __PTCX_INTERNAL_H__
