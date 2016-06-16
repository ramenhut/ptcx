
#include "ptcx_internal.h"

uint32 configure_quality_quant_step_bits(uint8 quality)
{
    switch (quality)
    {
        case 0:
        case 1: return 2;
        case 2: return 4;
        case 3: return 4;
        case 4: return 4;
        default: return 4;
    }

    return 2;
}

uint32 configure_quality_block_width(uint8 quality)
{
    switch (quality)
    {
        case 0:
        case 1: return PTCX_MAX_BLOCK_SIZE;
        case 2: return PTCX_MAX_BLOCK_SIZE >> 1;
        case 3: return PTCX_MAX_BLOCK_SIZE >> 2;
        case 4: return PTCX_MAX_BLOCK_SIZE >> 3;
        default: return PTCX_MAX_BLOCK_SIZE >> 2;
    }

    return 4;
}

uint32 configure_quality_block_height(uint8 quality)
{
    return configure_quality_block_width(quality);
}

uint32 sum_square_differences(uint8 *list_a, uint8 *list_b, uint32 count)
{
    uint32 total = 0;

    for (uint32 i = 0; i < count; i++)
    {
        int32 value_a = list_a[i];
        int32 value_b = list_b[i];
        int32 delta = value_b - value_a;

        total += delta * delta;
    }

    return total;
}

// Computes the quantization step value and (optionally) returns the quantized result.
uint8 quantize_pixel(uint8 quant_step_bits, const PTCX_PIXEL_RANGE &range, uint8 *source_pixel, uint32 *error = NULL)
{
    uint8 step_value = 0;

    // Here we're simply calculating the length of the vector that runs from our 
    // pixel boundary maximum coordinate to that of our minimum coordinate (range). We then
    // calculate the vector from our minimum coordinate to our current pixel (pixel) and 
    // project this vector onto (range) in order to determine its relative length percentage.

    int16 min_value[3] = {range.min_value[0], range.min_value[1], range.min_value[2]};
    int16 max_value[3] = {range.max_value[0], range.max_value[1], range.max_value[2]};
    int16 range_delta[3] = {max_value[0] - min_value[0], max_value[1] - min_value[1], max_value[2] - min_value[2]};
    int32 range_dot = range_delta[0] * range_delta[0] + range_delta[1] * range_delta[1] + range_delta[2] * range_delta[2];
    int16 range_length = sqrt(range_dot);

    uint8 step_count = (1 << quant_step_bits) - 1;
    int16 unit_length = (step_count ? (range_length / step_count) : 0);

    // We calculate the ratio of our (pixel) vector to that of our (range) vector and then 
    // map it to the set of quantization values, which are based on the number of quantization 
    // steps we're allowed to use (minus one to account for the inclusion of 1.0f).

    int16 pixel_values[3] = {source_pixel[0], source_pixel[1], source_pixel[2]};
    int16 pixel_delta[3] = {pixel_values[0] - min_value[0], pixel_values[1] - min_value[1], pixel_values[2] - min_value[2]};
    int32 pixel_dot = pixel_delta[0] * pixel_delta[0] + pixel_delta[1] * pixel_delta[1] + pixel_delta[2] * pixel_delta[2];
    int16 pixel_length = sqrt(pixel_dot);

    step_value = (unit_length ? (pixel_length / unit_length) : 0);
    
    if (error)
    {
        uint8 reconstruction[3] =
        {
            min_value[0] + range_delta[0] / step_count * step_value,
            min_value[1] + range_delta[1] / step_count * step_value,
            min_value[2] + range_delta[2] / step_count * step_value,
        };

        (*error) = sum_square_differences(source_pixel, reconstruction, 3);
    }

    return step_value;
}

void write_quantization_table(const PTCX_FILE_HEADER &header, const PTCX_PIXEL_RANGE &range, const image &input, uint32 x, uint32 y, ring_buffer<uint8> *output)
{
    uint32 quant_step_count = 1 << header.quant_step_bits;
    uint32 quant_step_mask = quant_step_count - 1;
    uint8 quant_look_aside = 0;

    for (uint32 subj = 0; subj < header.block_height; subj++)
    for (uint32 subi = 0; subi < header.block_width; subi++)
    {
        uint32 linear_sub_index = subi + subj * header.block_width;
        uint8 *src_pixel = input.query_data() + input.query_block_offset(x + subi, y + subj);
        uint32 clamped_index = quantize_pixel(header.quant_step_bits, range, src_pixel);

        // Add this new quantized value into our list. Note that we always add to the most 
        // significant bits in order to ensure proper ordering for a future dequantization 
        // operation (which will expect the bits in ascending order).

        quant_look_aside >>= header.quant_step_bits;
        quant_look_aside |= (clamped_index << ( 8 - header.quant_step_bits));

        // If we have a full byte of data in our look aside buffer, write it out. We determine
        // when to write it out by checking the current number of new bits we've written to our
        // buffer, and writing whenever it reaches 8 bits.

        if (linear_sub_index && (8 - header.quant_step_bits) == ((header.quant_step_bits * linear_sub_index) % 8))
        {
            if (base_failed(output->write(quant_look_aside)))
            {
                base_post_error(BASE_ERROR_EXECUTION_FAILURE);
            }
        }
    }
}

void write_control_values(const PTCX_PIXEL_RANGE &range, const PTCX_FILE_HEADER &header, ring_buffer<uint8> *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (!output || output->is_full())
        {
            base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    switch (header.quant_control_bits)
    {
        case 16:
        {
            uint16 red5 = (range.min_value[2] / 8) & 0x1F;
            uint16 green6 = (range.min_value[1] / 4) & 0x3F;
            uint16 blue5 = (range.min_value[0] / 8) & 0x1F;

            uint16 min_combined = (red5 << 11) | (green6 << 5) | (blue5);

            red5 = (range.max_value[2] / 8) & 0x1F;
            green6 = (range.max_value[1] / 4) & 0x3F;
            blue5 = (range.max_value[0] / 8) & 0x1F;

            uint16 max_combined = (red5 << 11) | (green6 << 5) | (blue5);

            output->write(min_combined & 0xFF);
            output->write(min_combined >> 8);
            output->write(max_combined & 0xFF);
            output->write(max_combined >> 8);

        } break;

        default: base_post_error(BASE_ERROR_EXECUTION_FAILURE);;
    };
}

uint32 estimate_quantization_error(const PTCX_FILE_HEADER &header, const PTCX_PIXEL_RANGE &range, const image &input, uint32 pixel_x, uint32 pixel_y)
{
    uint32 error = 0;
    uint8 quant_look_aside = 0;
    uint32 quant_step_count = 1 << header.quant_step_bits;
    uint32 quant_step_mask = quant_step_count - 1;

    for (uint32 subj = 0; subj < header.block_height; subj++)
    for (uint32 subi = 0; subi < header.block_width;  subi++ )
    {
        uint32 temp_error;

        // quantize the source value, dequantize it, and then compare against the source (add squared error to sum)
        uint8 *source_pixel = input.query_data() + input.query_block_offset(pixel_x + subi, pixel_y + subj);        
        quantize_pixel(header.quant_step_bits, range, source_pixel, &temp_error);

        error += temp_error; 
    }

    return error;
}

void quantize_microblock(const image &input, const PTCX_FILE_HEADER &header, uint32 pixel_x, uint32 pixel_y, ring_buffer<uint8> *output, uint32 *error)
{
    uint32 best_quant_func = 0;
    uint32 lowest_quant_error = BASE_MAX_UINT32;   

    PTCX_PIXEL_RANGE range[3] = 
    {
        {{255, 255, 255}, {0, 0, 0}}, 
        {{255, 255, 255}, {0, 0, 0}},
        {{255, 255, 255}, {0, 0, 0}}
    };

    // We support three different methods for generating the control values. Selection
    // of these values, in conjunction with the particular characteristics of the source
    // data, has a large impact on the quality of the compression -- so we perform all three 
    // paths and then select the best one.

    for (uint8 quant = 0; quant < 3; quant++)
    {
        switch (quant)
        {
            case 0: range_estimate_min_max(header, &range[quant], input, pixel_x, pixel_y); break;
            
            // For most images these estimators will increase processing costs with little added benefit.
            // case 1: range_estimate_regression(header, &range[quant], input, pixel_x, pixel_y); break;
            // case 2: range_estimate_linear_distance(header, &range[quant], input, pixel_x, pixel_y); break;

            default: continue;
        };

        // Calculate the expected error to determine the best range method.
        uint32 quant_error = estimate_quantization_error(header, range[quant], input, pixel_x, pixel_y);

        if (quant_error <= lowest_quant_error)
        {
            lowest_quant_error = quant_error;
            best_quant_func = quant;
        }
    }

#if PTCX_SHOW_RANGE_MAP
    range[best_quant_func].min_value[0] =  64 + best_quant_func * 32;
    range[best_quant_func].min_value[1] = 128 + best_quant_func * 32;
    range[best_quant_func].min_value[2] =  64 + best_quant_func * 32;
    range[best_quant_func].max_value[0] =  64 + best_quant_func * 32;
    range[best_quant_func].max_value[1] = 128 + best_quant_func * 32;
    range[best_quant_func].max_value[2] =  64 + best_quant_func * 32;
#endif

    // Using the best quant func write out our control values as well as
    // our full quantization table.

    (*error) += lowest_quant_error;
    write_control_values(range[best_quant_func], header, output);
    write_quantization_table(header, range[best_quant_func], input, pixel_x, pixel_y, output);
}

void write_macroblock_table_entry(uint8 *mb_table, uint32 x, uint32 width_in_blocks, uint32 y, uint8 value)
{
    // The byte we must access is block_index / 4, and the bits within that byte 
    // are defined by (bits >> (2 * (block_index % 4))) & 0x3.

    uint32 block_index = y * width_in_blocks + x;
    uint32 byte_index = block_index >> 2;
    uint8 bit_shift = (block_index % 4) << 1;
    uint8 *bit_data = 0;
    uint8 bit_mask = 0;

    // Fetch our byte from the buffer.
    bit_data = &mb_table[byte_index];
    bit_mask = 0x3 << bit_shift;
    (*bit_data) &= (value << bit_shift) | (~bit_mask);
}

status quantize_macroblock(const image &input, const PTCX_FILE_HEADER &header, uint32 pixel_x, uint32 pixel_y, ring_buffer<uint8> *trial_buffers, uint8 *mb_table, stream *out_stream)
{
    PTCX_FILE_HEADER trial_header = header;
    
    uint8 final_macroblock_level = 2;
    uint32 block_pixel_count = header.block_width * header.block_height;
    uint32 trial_macroblock_error[3] = {0};

    // We check which microblock size yields the best compression 
    // ratio for the provided quality.

    for (uint32 block_shift = 0; block_shift < 3; block_shift++)
    {
        trial_buffers[block_shift].empty();

        trial_header.block_width = header.block_width >> block_shift;
        trial_header.block_height = header.block_height >> block_shift;

        if (trial_header.block_width < 2) trial_header.block_width = 2;
        if (trial_header.block_height < 2) trial_header.block_height = 2;

        // Traverse each pixel, appending control bits onto our trial table. Note that
        // the control bits specified in the header will be a power of 2 between 2 and 8.

        for (uint32 micro_j = 0; micro_j < (header.block_height / trial_header.block_height); micro_j++)
        for (uint32 micro_i = 0; micro_i < (header.block_width / trial_header.block_width); micro_i++)
        {
            uint32 sub_x = pixel_x + micro_i * trial_header.block_width;
            uint32 sub_y = pixel_y + micro_j * trial_header.block_height;

            quantize_microblock(input, trial_header, sub_x, sub_y, &trial_buffers[block_shift], 
                                &trial_macroblock_error[block_shift]);
        }

        // Convert our measured sum of squared error into mean squared quantization error.
        trial_macroblock_error[block_shift] = trial_macroblock_error[block_shift] / block_pixel_count;
    }

    // Select the best compression option whose error rate is below our threshold, and write its 
    // results to the output.

    if (trial_macroblock_error[0] <= PTCX_QUALITY_DELTA) 
    {
        final_macroblock_level = 0;
    }
    else if (trial_macroblock_error[1] <= PTCX_QUALITY_DELTA) 
    {
        final_macroblock_level = 1;
    }

    write_macroblock_table_entry(mb_table, pixel_x / header.block_width, header.image_width / header.block_width, 
                                 pixel_y / header.block_height, final_macroblock_level);

    out_stream->write_data(trial_buffers[final_macroblock_level].peek(), 
                           trial_buffers[final_macroblock_level].query_occupancy());

    return BASE_SUCCESS;
}

status quantize_worker(const image &input, const PTCX_FILE_HEADER &header, uint8 *mb_table, stream *out_stream)
{    
    ring_buffer<uint8> trial_buffers[3];

    for (uint8 i = 0; i < 3; i++)
    {
        if (PTCX_MAX_BLOCK_DATA_SIZE != trial_buffers[i].resize_capacity(PTCX_MAX_BLOCK_DATA_SIZE))
        {
            return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
        }
    }

    for (uint32 j = 0; j < input.query_height(); j += header.block_height)
    for (uint32 i = 0; i < input.query_width(); i += header.block_width)
    {
        if (base_failed(quantize_macroblock(input, header, i, j, trial_buffers, mb_table, out_stream)))
        {
            return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
        }
    }

    return BASE_SUCCESS;
}

status prepare_temporary_buffers(const PTCX_FILE_HEADER &header, memory_stream *temp_stream, std::vector<uint8> *macroblock_table)
{
    uint32 max_image_size = (header.image_width / 2) * (header.image_height * 2) * (6 + 2);
    uint32 macroblock_table_size = (header.image_width / header.block_width) * (header.image_height / header.block_height);
    macroblock_table_size = (macroblock_table_size << 1) >> 3;

    if (!macroblock_table_size)
    {
        // We do not support images with fewer than 4 blocks.
        return base_post_error(BASE_ERROR_INVALID_RESOURCE);
    }

    macroblock_table->resize(macroblock_table_size);

    if (macroblock_table_size != macroblock_table->size())
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (max_image_size != temp_stream->resize_capacity(max_image_size))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}

status quantize_image(const image &input, const PTCX_FILE_HEADER &header, stream *out_stream)
{
    if (BASE_PARAM_CHECK)
    {
        if (!out_stream)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }

        if (!is_pow2(header.quant_step_bits))
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    memory_stream image_stream;
    std::vector<uint8> macroblock_table;

    if (base_failed(prepare_temporary_buffers(header, &image_stream, &macroblock_table)))
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    if (base_failed(quantize_worker(input, header, &(macroblock_table[0]), &image_stream)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    // Relay our quantization table and image buffer out to the final output stream with correct order.
    if (base_failed(out_stream->write_data(&macroblock_table[0], macroblock_table.size())))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(out_stream->write_data(image_stream.query_read_pointer(), image_stream.query_occupancy())))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}

void configure_header_quality(PTCX_FILE_HEADER *header, uint8 quality)
{
    header->block_width = configure_quality_block_width(quality);
    header->block_height = configure_quality_block_height(quality);
    header->quant_step_bits = configure_quality_quant_step_bits(quality);
}

void configure_header(const image &input, PTCX_FILE_HEADER *out_header, uint8 quality)
{
    out_header->magic = PTCX_MAGIC_VALUE;
    out_header->version = PTCX_MAJOR_VERSION;                    
    out_header->header_size = sizeof(PTCX_FILE_HEADER);
    out_header->image_width = input.query_width();
    out_header->image_height = input.query_height();
    out_header->image_depth = PTCX_DEFAULT_IMAGE_DEPTH;
    out_header->block_width = PTCX_MAX_BLOCK_SIZE / 4;
    out_header->block_height = PTCX_MAX_BLOCK_SIZE / 4;
    out_header->quant_step_bits = PTCX_MAX_QUANT_STEP_BITS;
    out_header->quant_control_bits = PTCX_MAX_QUANT_CONTROL_BITS;
    out_header->source_format = input.query_image_format();

    configure_header_quality(out_header, quality);
}

status save_ptcx(const image &input, uint8 quality, stream *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (!output || output->is_full()) 
        {
            return BASE_ERROR_INVALIDARG;
        }

        if (IGN_IMAGE_FORMAT_R8G8B8 != input.query_image_format())
        {
            return BASE_ERROR_INVALIDARG;
        }
    }

    uint32 bytes_written = 0;

    PTCX_FILE_HEADER pxh = {0};

    // Our input image must be 16 pixel aligned, for now.
    if (input.query_width() % 16 || input.query_height() % 16)
    {
        return BASE_ERROR_INVALID_RESOURCE;
    }
    
    configure_header(input, &pxh, quality);

    if (base_failed(output->write_data(&pxh, sizeof(PTCX_FILE_HEADER), &bytes_written)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (bytes_written != sizeof(PTCX_FILE_HEADER))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(quantize_image(input, pxh, output)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    return BASE_SUCCESS;
}