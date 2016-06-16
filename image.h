
/*
// Copyright (c) 2002-2014 Joe Bertolami. All Right Reserved.
//
// image.h
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

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "base.h"

namespace imagine {

using namespace base;

enum IGN_IMAGE_FORMAT 
{
    IGN_IMAGE_FORMAT_NONE = 0,
    IGN_IMAGE_FORMAT_R8G8B8,            // RGB, 8 bits per channel
};

class image
{
    friend status create_image(IGN_IMAGE_FORMAT format, uint32 width, uint32 height, image *output);
    friend status create_image(IGN_IMAGE_FORMAT format, void *image_data, uint32 width, uint32 height, image *output);
    friend status destroy_image(image *input);

private:

    IGN_IMAGE_FORMAT image_format;
    bool placement_allocation;

    uint32 width_in_pixels;
    uint32 height_in_pixels;
    uint32 bits_per_pixel;
    uint8 channel_count;
    uint8 *data_buffer;

private:

    /* 
    // Allocation management 
    //
    // The following methods should not be used for placement allocated images. For 
    // non-placement images, deallocate must be called prior to destruction.
    */

    status allocate(uint32 size);

    void deallocate();

    /*
    // An image is considered uninitialized if its format field is set to zero
    // (no format). Images must have an empty data buffer in this scenario.
    */

    status set_image_format(IGN_IMAGE_FORMAT format);

    /*
    // set_dimension will automatically manage the memory of the object. This is the 
    // primary interface that should be used for reserving memory for the image. Note
    // that the image must contain a valid format prior to calling SetDimension.
    */

    status set_dimension(uint32 width, uint32 height);

    /*
    // placement_allocation identifies whether the image owns its backing storage or
    // whether the memory was provided by the caller.
    */

    status set_placement(void *data);

public:

    image();		
    virtual ~image();	

    /*
    // Image dimensions are always specified in pixels. Note that compressed image
    // formats may not store their image data as a contiguous set of pixels.
    */

    uint32 query_width() const;                     
    uint32 query_height() const;                    

    uint8 *query_data() const;              
    uint8 query_bits_per_pixel() const;    
    uint8 query_channel_count() const;

    IGN_IMAGE_FORMAT query_image_format() const;

    /*
    // Row Pitch
    //
    // row pitch is the byte delta between two adjacent rows of pixels in the image.
    // This function takes alignment into consideration and may provide a value that
    // is greater than the byte width of the visible image. 
    */

    uint32 query_row_pitch() const;
    
    /*
    // Slice Pitch
    //
    // SlicePitch is the byte size of the entire image. This size may extend beyond the
    // edge of the last row and column of the image, due to alignment and tiling 
    // requirements on certain platforms.
    */

    uint32 query_slice_pitch() const;

    /*
    // Block Offset
    //
    // Block offset returns the byte offset from the start of the image to pixel (i,j).
    // Formats are required to use byte aligned pixel rates, so this function will always
    // point to the start of a pixel block.
    */

    uint32 query_block_offset(uint32 i, uint32 j) const;
};

/* 
// image management 
//
// create_image must be called prior to using an image object, and destroy_image should always 
// be called on images prior to their destruction. We use this factory-like interface in order 
// to hide the details of the underlying image implementation.
*/

status create_image(IGN_IMAGE_FORMAT format, uint32 width, uint32 height, image *output);
status create_image(IGN_IMAGE_FORMAT format, void *image_data, uint32 width, uint32 height, image *output);
status destroy_image(image *input);

} // namespace imagine

#endif // __IMAGE_H__
