
#include "ptcx_internal.h"

void read_control_values(stream *input, PTCX_PIXEL_RANGE *range, const PTCX_FILE_HEADER &header)
{
    if (BASE_PARAM_CHECK)
    {
        if (!input || input->is_empty())
        {
            base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    switch (header.quant_control_bits)
    {
        case 16:
        {
            uint16 min_value = 0;
            uint16 max_value = 0;

            input->read_data(&min_value, 2);
            input->read_data(&max_value, 2);

            range->min_value[0] = ((min_value) & 0x1F) * 8;
            range->min_value[1] = ((min_value >> 5) & 0x3F) * 4;
            range->min_value[2] = ((min_value >> 11) & 0x1F) * 8;

            range->max_value[0] = ((max_value) & 0x1F) * 8;
            range->max_value[1] = ((max_value >> 5) & 0x3F) * 4;
            range->max_value[2] = ((max_value >> 11) & 0x1F) * 8;

        } break;

        default: base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    };
}

status read_macroblock_table(stream *input, const PTCX_FILE_HEADER &header, std::vector<uint8> *output)
{
    uint32 table_byte_size = (header.image_width / header.block_width) *
                             (header.image_height / header.block_height);

    table_byte_size = (table_byte_size >> 2);

    output->resize(table_byte_size);

    if (table_byte_size != output->size())
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(input->read_data(&((*output)[0]), table_byte_size)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}

uint8 query_macroblock_shift(uint8 *input, uint32 x, uint32 width_in_blocks, uint32 y)
{
    // The byte we must access is uiBlockIndex / 4, and the bits within that byte are defined by
    // ( bits >> ( 2 * ( uiBlockIndex % 4 ) ) ) & 0x3.

    uint32 block_index = y * width_in_blocks + x;
    uint32 byte_index = block_index >> 2;
    uint32 bit_data = 0;

    bit_data = input[byte_index];

    return (bit_data >> ((block_index % 4) << 1)) & 0x3;
}

status read_macroblock(stream *input, const PTCX_FILE_HEADER &header, uint32 start_x, uint32 start_y, image *output)
{
    uint32 quant_step_count = 1 << header.quant_step_bits;
    uint32 quant_step_mask = quant_step_count - 1;
    uint8 quant_look_aside = 0;

    VN_PTCX_PIXEL_RANGE range = {{255, 255, 255}, {0, 0, 0}};

    // Read our control values from the stream, using the number of bits defined
    // by our pandax file header structure. 

    read_control_values(input, &range, header);

    int16 min_value[3] = {range.min_value[0], range.min_value[1], range.min_value[2]};
    int16 max_value[3] = {range.max_value[0], range.max_value[1], range.max_value[2]};
    int16 range_delta[3] = {max_value[0] - min_value[0], max_value[1] - min_value[1], max_value[2] - min_value[2]};

    // Read our quantization table out to the image, using the number of bits (and
    // thus steps) as defined by our ptcx file header structure.

    for (uint32 subj = 0; subj < header.block_height; subj++)
    for (uint32 subi = 0; subi < header.block_width; subi++)
    {
        uint32 linear_sub_index = subi + subj * header.block_width;
        uint8 *dest_pixel = output->query_data() + output->query_block_offset(start_x + subi, start_y + subj);

        // If we have an empty byte of data in our look aside buffer, read one in.
        if (0 == ((header.quant_step_bits * linear_sub_index) % 8))
        {
            if (base_failed(input->read_data(&quant_look_aside, 1)))
            {
                return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
            }
        }

        // Now we must simply pull a new quantization table value from our list. Note that
        // we always remove from the least significant bits in order to ensure proper ordering 
        // with respect to the quantization operation.

        uint32 step_value = (quant_look_aside & quant_step_mask);
        quant_look_aside >>= header.quant_step_bits;

        dest_pixel[0] = min_value[0] + range_delta[0] / quant_step_mask * step_value;
        dest_pixel[1] = min_value[1] + range_delta[1] / quant_step_mask * step_value;
        dest_pixel[2] = min_value[2] + range_delta[2] / quant_step_mask * step_value;
    }

    return BASE_SUCCESS;
}

status inverse_quantize(stream *input, const PTCX_FILE_HEADER &header, image *output)
{    
    std::vector<uint8> macroblock_table;
    PTCX_FILE_HEADER temp_header = header;  
    
    // At the start of our file, just after our header, we have a quantization map that indicates a 
    // per-macro-block block shift. Each entry in our map is two bits, and we have one set of these
    // bits for each macro block (defined as the block width * height in our header). 

    if (base_failed(read_macroblock_table(input, header, &macroblock_table)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    // Dequantize the data and place in our output buffer
    for (uint32 j = 0; j < output->query_height(); j += header.block_height)
    for (uint32 i = 0; i < output->query_width(); i += header.block_width)
    {
        // Query our micro-block size and proceed to decompress each micro-block
        // within our larger macro-block. We grab our two bits and divide the 
        // supplied macroblock dimensions by that amount (down to a minimum of two).

        uint8 macro_scale_bits = \
            query_macroblock_shift(&macroblock_table[0], i / header.block_width,
                                   header.image_width / header.block_width, j / header.block_height );

        temp_header.block_width = header.block_width >> macro_scale_bits;
        temp_header.block_height = header.block_height >> macro_scale_bits;

        if (temp_header.block_width < 2) temp_header.block_width = 2;
        if (temp_header.block_height < 2) temp_header.block_height = 2;

        for (uint32 micro_j = 0; micro_j < (header.block_height / temp_header.block_height); micro_j++)
        for (uint32 micro_i = 0; micro_i < (header.block_width / temp_header.block_width); micro_i++)
        {
            uint32 adjusted_i = i + micro_i * temp_header.block_width;
            uint32 adjusted_j = j + micro_j * temp_header.block_height;

            if (base_failed(read_macroblock(input, temp_header, adjusted_i, adjusted_j, output)))
            {
                return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
            }

#if PTCX_SHOW_BLOCK_MAP

            // Write out the block shift for the current microblock.
            for (uint32 subj = 0; subj < temp_header.block_height; subj++)
            for (uint32 subi = 0; subi < temp_header.block_width; subi++)
            {
                uint8 *dest_pixel = output->query_data() + output->query_block_offset(adjusted_i + subi, adjusted_j + subj);
                dest_pixel[0] = (128 + macro_scale_bits * 32);
                dest_pixel[1] = (64 + macro_scale_bits * 32);
                dest_pixel[2] = (64 + macro_scale_bits * 32); 
            }
#endif
        }        
    }

    return BASE_SUCCESS;
}

status load_ptcx(stream *input, image *output)
{
    uint32 bytes_read = 0;
    PTCX_FILE_HEADER pxh = {0};

    if (BASE_PARAM_CHECK)
    {
        if (!input || !output || input->is_empty()) 
        {
            return BASE_ERROR_INVALIDARG;
        }
    }

    if (base_failed(input->read_data(&pxh, sizeof(PTCX_FILE_HEADER), &bytes_read)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (bytes_read != sizeof(PTCX_FILE_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }
    
    // Verify the integrity of our file
    if (PTCX_MAGIC_VALUE != pxh.magic || sizeof(PTCX_FILE_HEADER) != pxh.header_size)
    {
        return base_post_error(BASE_ERROR_INVALID_RESOURCE);
    }

    if (2 != pxh.version)
    {
        return base_post_error( BASE_ERROR_INVALID_RESOURCE );
    }

    // Create our image as an RGB8 source.
    if (base_failed(create_image(IGN_IMAGE_FORMAT_R8G8B8, pxh.image_width, pxh.image_height, output)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    // Dequantize our image blob based on the header data.
    if (base_failed(inverse_quantize(input, pxh, output)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}
