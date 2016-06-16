
#include "bitmap.h"
#include "math.h"

#ifndef BI_RGB
#define BI_RGB                                      (0)
#endif

#pragma pack( push )
#pragma pack( 2 )

typedef struct PTCX_BITMAP_FILE_HEADER 
{
    uint16 type;
    uint32 size;
    uint16 reserved[2];
    uint32 off_bits;

} PTCX_BITMAP_FILE_HEADER;

typedef struct PTCX_BITMAP_INFO_HEADER
{
    uint32 size;
    int32 width;
    int32 height;
    uint16 planes;
    uint16 bit_count;
    uint32 compression;
    uint32 size_image;
    int32 x_pels_per_meter;
    int32 y_pels_per_meter;
    uint32 clr_used;
    uint32 clr_important;

} PTCX_BITMAP_INFO_HEADER;

#pragma pack(pop)

status write_bitmap_image_data(stream * dest, const image &input)
{
    if (BASE_PARAM_CHECK)
    {
        if (!dest || dest->is_full())
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    // The BMP format requires each scanline to be 32 bit aligned, so we insert padding if necessary
    uint32 scanline_padding = greater_multiple(input.query_row_pitch(), 4) - input.query_row_pitch();

    for (uint32 i = 0; i < input.query_height(); i++)
    {
        uint8 *src_ptr = input.query_data() + input.query_block_offset(0, i);

        if (base_failed(dest->write_data(src_ptr, input.query_row_pitch())))
        {
            return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
        }

        for (uint32 j = 0; j < scanline_padding; j++)
        {
            uint8 dummy = 0;

            if (base_failed(dest->write_data(&dummy, 1)))
            {
                return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
            }
        }
    }

    return BASE_SUCCESS;
}

status read_bitmap_image_data(stream *src, image *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (!src || src->is_empty() || !output)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    // The BMP format requires each scanline to be 32 bit aligned, so we insert padding if necessary.
    uint32 scanline_padding = greater_multiple(output->query_row_pitch(), 4) - output->query_row_pitch();

    for (uint32 i = 0; i < output->query_height(); i++)
    {
        uint8 *dest_ptr = output->query_data() + output->query_block_offset(0, i);

        if (base_failed(src->read_data(dest_ptr, output->query_row_pitch())))
        {
            return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
        }

        for (uint32 j = 0; j < scanline_padding; j++)
        {
            uint8 dummy = 0;

            if (base_failed(src->read_data(&dummy, 1)))
            {
                return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
            }
        }
    }

    return BASE_SUCCESS;
}

status load_bitmap(stream *src, image *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (!src || src->is_empty() || !output) 
        {
            return BASE_ERROR_INVALIDARG;
        }
    }

    uint32 bytes_read = 0;

    IGN_IMAGE_FORMAT vif;
    PTCX_BITMAP_INFO_HEADER bih;
    PTCX_BITMAP_FILE_HEADER bmf_header;     

    if (base_failed(src->read_data((uint8 *) &bmf_header, sizeof(PTCX_BITMAP_FILE_HEADER ), &bytes_read)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (bytes_read != sizeof(PTCX_BITMAP_FILE_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(src->read_data((uint8 *) &bih, sizeof(PTCX_BITMAP_INFO_HEADER ), &bytes_read)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (bytes_read != sizeof(PTCX_BITMAP_INFO_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    switch (bih.bit_count)
    {
        case 24: vif = IGN_IMAGE_FORMAT_R8G8B8; break; 

        default:
        {
            base_err("Not implemented: bitmap bit rates other than 24.");

            return BASE_ERROR_INVALID_RESOURCE;

        } break;
    }

    if (base_failed(create_image(vif, bih.width, bih.height, output)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(read_bitmap_image_data(src, output)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    // One final step -- the bitmap format stores data in BGR(A) format, so we must
    // convert in order to store it in memory as RGB(A)

    for (uint32 y = 0; y < (output)->query_height(); y++)
    for (uint32 x = 0; x < (output)->query_width(); x++)
    {
        uint8 * src_data = (output)->query_data() + (output)->query_block_offset(x, y); 
        uint8 * dest_data = src_data;

        uint8 temp = src_data[0];
        dest_data[0] = src_data[2];
        dest_data[2] = temp;
    }

    return BASE_SUCCESS;
}

status save_bitmap(stream *dest, const image &input)
{
    if (BASE_PARAM_CHECK)
    {
        if (!dest || dest->is_full()) 
        {
            return BASE_ERROR_INVALIDARG;
        }

        if (0 == input.query_width() ||
            0 == input.query_height() || 
            0 == input.query_bits_per_pixel())
        {
            return BASE_ERROR_INVALIDARG;
        }
    }

    // This is only a simple test, so we support a single flavor of BMP.
    if (24 != input.query_bits_per_pixel())
    {
        return BASE_ERROR_INVALID_RESOURCE;
    }

    // One pre-step -- the bitmap format stores data in BGR(A) format, so we must
    // convert in order to save it to the stream as RGB(A)

    for (uint32 y = 0; y < input.query_height(); y++)
    for (uint32 x = 0; x < input.query_width(); x++)
    {
        uint8 *src_data = input.query_data() + input.query_block_offset(x, y); 
        uint8 *dest_data = src_data;

        uint8 temp = src_data[0];
        dest_data[0] = src_data[2];
        dest_data[2] = temp;
    }

    uint32 bytes_written = 0;
    PTCX_BITMAP_INFO_HEADER bih;
    PTCX_BITMAP_FILE_HEADER bmf_header;    

    uint32 size_of_file = input.query_slice_pitch() + sizeof(PTCX_BITMAP_FILE_HEADER) + sizeof(PTCX_BITMAP_INFO_HEADER);
     
    bih.size = sizeof(PTCX_BITMAP_INFO_HEADER);    
    bih.width = input.query_width();    
    bih.height = input.query_height();  
    bih.bit_count = 24; 
    bih.planes = 1;    
    bih.compression = BI_RGB;    
    bih.size_image = input.query_slice_pitch();  
    bih.x_pels_per_meter = 0;    
    bih.y_pels_per_meter = 0;    
    bih.clr_used = 0;    
    bih.clr_important = 0;    

    bmf_header.off_bits = sizeof(PTCX_BITMAP_FILE_HEADER) + sizeof(PTCX_BITMAP_INFO_HEADER); 
    bmf_header.size = size_of_file; 
    bmf_header.type = 0x4D42; //BM   
 
    if (base_failed(dest->write_data(&bmf_header, sizeof(PTCX_BITMAP_FILE_HEADER), &bytes_written)) || 
        bytes_written != sizeof(PTCX_BITMAP_FILE_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(dest->write_data(&bih, sizeof(PTCX_BITMAP_INFO_HEADER), &bytes_written)) || 
        bytes_written != sizeof(PTCX_BITMAP_INFO_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(write_bitmap_image_data(dest, input)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}
