
/*
// Copyright (c) 2002-2011 Joe Bertolami. All Right Reserved.
//
// ptcx.h
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
// Description:
//
//   Primitive Texture Compression (PTCX) is a simple compressed texture format designed 
//   by Joe Bertolami in 2003 for the Vision Engine. This format provides the following
//   benefits:
//
//      o: Moderate texture compression ratio (typically between 4:1 and 16:1)
//      o: Flexible encoding settings that can be adjusted based on the content
//      o: Simple architecture that can be adjusted to support different pixel formats
//      o: Relatively fast decode performance
//
// Notes:
//
//   (1) This format is typically used to compress low frequency data (e.g. textures for grass 
//		 and trees -- but not faces or text). In testing we've found that PTCX can outperform
//       PNG, in terms of compression efficiency, for these kinds of textures. 
//
//   (2) Unlike DXT and S3TC, there is no GPU hardware support for PTCX. This severely
//       limits its practical utility. This release is meant for educational purposes.
//
//  Additional Information:
//
//   For more information, visit http://www.bertolami.com.
*/

#ifndef __PTCX_H__
#define __PTCX_H__

#include "base.h"
#include "image.h"
#include "stream.h"

using namespace base;
using namespace imagine;

/* 
// PTCX Decode
//
//   Decompresses data from a data source and places it in a freshly allocated
//   output image.
//
// Returns:
//
//   BASE_SUCCESS upon success, otherwise a specific error value will be returned. 
*/

status load_ptcx(stream *input, image *output);

/*
// PTCX Encode
//
//   Compresses an image according to the specified quality, and writes the 
//   result to the output buffer.
//
// Returns:
//
//   BASE_SUCCESS upon success, otherwise a specific error value will be returned.
//
// Notes:
//
//   o: Quality ranges from 1-4, with 4 being the highest quality (least compression)
//   o: The input image must be RGB8 and macroblock (BASE_PTCX_MAX_BLOCK_SIZE) pixel aligned.
*/

status save_ptcx(const image &input, uint8 quality, stream *output);

#endif // __PTCX_H__
