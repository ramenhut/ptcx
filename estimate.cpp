
#include "ptcx_internal.h"

status range_estimate_min_max(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y)
{
    if (BASE_PARAM_CHECK )
    {
        if (!range)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    range->min_value[0] = 255;
    range->min_value[1] = 255;
    range->min_value[2] = 255;

    range->max_value[0] = 0;
    range->max_value[1] = 0;
    range->max_value[2] = 0;

    for (uint32 subj = 0; subj < header.block_height; subj++)
    for (uint32 subi = 0; subi < header.block_width; subi++)
    {
        uint8 *src_pixel = input.query_data() + input.query_block_offset(x + subi, y + subj);

        if (src_pixel[0] < range->min_value[0]) range->min_value[0] = src_pixel[0];
        if (src_pixel[1] < range->min_value[1]) range->min_value[1] = src_pixel[1];
        if (src_pixel[2] < range->min_value[2]) range->min_value[2] = src_pixel[2];

        if (src_pixel[0] > range->max_value[0]) range->max_value[0] = src_pixel[0];
        if (src_pixel[1] > range->max_value[1]) range->max_value[1] = src_pixel[1];
        if (src_pixel[2] > range->max_value[2]) range->max_value[2] = src_pixel[2];
    }     

    return BASE_SUCCESS;
}

status range_estimate_linear_distance(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y)
{
    if (BASE_PARAM_CHECK)
    {
        if (!range)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    uint32 aii = 0, ajj = 0;
    uint32 ai = 0, aj = 0;
    uint32 max_length = 0;
       
    for (uint32 subjj = 0; subjj < header.block_height; subjj++)
    for (uint32 subii = 0; subii < header.block_width; subii++)
    for (uint32 subj = 0;  subj < header.block_height; subj++)
    for (uint32 subi = 0;  subi < header.block_width; subi++)
    {
        if (subjj == subj && subii == subi) continue;

        uint8 *src_pixel_a = input.query_data() + input.query_block_offset(x + subi, y + subj);
        uint8 *src_pixel_b = input.query_data() + input.query_block_offset(x + subii, y + subjj);

        int32 delta[3] = 
        { 
            static_cast<int32>(src_pixel_a[0]) - src_pixel_b[0], 
            static_cast<int32>(src_pixel_a[1]) - src_pixel_b[1],
            static_cast<int32>(src_pixel_a[2]) - src_pixel_b[2]
        };

        uint32 length = sqrt(static_cast<uint32>(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]));

        if (length > max_length)
        {
            max_length = length;
            aii = subii;
            ajj = subjj;
            ai = subi;
            aj = subj;
        }
    }     

    // Compute the final pixel range.
    uint8 * min_pixel = input.query_data() + input.query_block_offset(x + aii, y + ajj);
    uint8 * max_pixel = input.query_data() + input.query_block_offset(x + ai, y + aj);

    uint32 min_pixel_values[3] = { min_pixel[0], min_pixel[1], min_pixel[2] };
    uint32 max_pixel_values[3] = { max_pixel[0], max_pixel[1], max_pixel[2] };

    uint32 min_pixel_length = sqrt((min_pixel_values[0] * min_pixel_values[0]) + 
                                   (min_pixel_values[1] * min_pixel_values[1]) + 
                                   (min_pixel_values[2] * min_pixel_values[2]));

    uint32 max_pixel_length = sqrt((max_pixel_values[0] * max_pixel_values[0]) + 
                                   (max_pixel_values[1] * max_pixel_values[1]) +
                                   (max_pixel_values[2] * max_pixel_values[2]));

    if (max_pixel_length > min_pixel_length)
    {
        memcpy(range->min_value, min_pixel, 3);
        memcpy(range->max_value, max_pixel, 3);
    }
    else
    {
        memcpy(range->min_value, max_pixel, 3);
        memcpy(range->max_value, min_pixel, 3);
    }

    return BASE_SUCCESS;
}

status compute_linear_squares_2(uint8 *pixel_list, uint32 pixel_count, uint8 *out_start, uint8 *out_end)
{
    if (BASE_PARAM_CHECK)
    {
        if (!pixel_list || !out_start || !out_end)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

	float xiyi = 0;
	float xi2 = 0;
	float yi = 0;
	float xi = 0;
    float i = pixel_count;
	
	for (uint32 j = 0; j < pixel_count; j++)
	{
		xiyi += pixel_list[j * 2 + 0] * pixel_list[j * 2 + 1];
		xi2  += pixel_list[j * 2 + 0] * pixel_list[j * 2 + 0];
		yi   += pixel_list[j * 2 + 1];
		xi   += pixel_list[j * 2 + 0];
	}

    // Solve our linear system
    float bd = (i / xi - xi / xi2);
    float md = (2.0f * xi2);

    if (0.0f == bd || 0.0f == md)
    {
        return BASE_ERROR_EXECUTION_FAILURE;
    }

    float b = (xiyi / xi2 - yi / xi) / bd;
	float m = (2.0f * xiyi + 2.0f * b * xi) / md;

    // Next we determine an origin and vector for the line. We search for the coordinate 
    // with the minimum x, and then plug this into our line equation to determine the 
    // closest origin. We also search for the coordinate with the maximum x, and use this 
    // to derive the line segment end point.

    float min_x_value = 9999999.0f;
    float max_x_value = 0;

     for (uint32 j = 0; j < pixel_count; j++)
     {
         if (pixel_list[j * 2 + 0] <= min_x_value) min_x_value = pixel_list[j * 2 + 0];
         if (pixel_list[j * 2 + 0] >= max_x_value) max_x_value = pixel_list[j * 2 + 0]; 
     }

     out_start[0] = min_x_value;
     out_start[1] = m * min_x_value + b;
     out_end[0] = max_x_value;
     out_end[1] = m * max_x_value + b;

	return BASE_SUCCESS;
}

status compute_linear_squares_3(uint8 *pixel_set, uint32 pixel_count, uint8 *out_start, uint8 *out_end)
{
   if (BASE_PARAM_CHECK)
    {
        if (!pixel_set || !out_start || !out_end)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

   uint8  a_list[PTCX_MAX_BLOCK_SIZE * PTCX_MAX_BLOCK_SIZE * 2];
   uint8  b_list[PTCX_MAX_BLOCK_SIZE * PTCX_MAX_BLOCK_SIZE * 2];

   for (uint32 i = 0; i < pixel_count; i++)
   {
       a_list[i * 2 + 0] = pixel_set[i * 3 + 0];
       a_list[i * 2 + 1] = pixel_set[i * 3 + 1];
       b_list[i * 2 + 0] = pixel_set[i * 3 + 1];
       b_list[i * 2 + 1] = pixel_set[i * 3 + 2];
   }

   uint8 a_list_origin[2] = {0};
   uint8 a_list_vector[2] = {0};
   uint8 b_list_origin[2] = {0};
   uint8 b_list_vector[2] = {0};

   if (base_failed(compute_linear_squares_2(a_list, pixel_count, a_list_origin, a_list_vector)) ||
       base_failed(compute_linear_squares_2(b_list, pixel_count, b_list_origin, b_list_vector)))
   {
       // Cannot perform a regression on this data.
       return BASE_ERROR_INVALID_RESOURCE;
   }

   out_start[0] = a_list_origin[0];
   out_start[1] = a_list_origin[1];
   out_start[2] = b_list_origin[1];
   
   out_end[0] = a_list_vector[0];
   out_end[1] = a_list_vector[1];
   out_end[2] = b_list_vector[1];

   return BASE_SUCCESS;
}

status range_estimate_regression(const PTCX_FILE_HEADER &header, PTCX_PIXEL_RANGE *range, const image &input, uint32 x, uint32 y)
{
    if (BASE_PARAM_CHECK)
    {
        if (!range)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    uint8 pixel_set[PTCX_MAX_BLOCK_SIZE * PTCX_MAX_BLOCK_SIZE * 3];
    uint32 index = 0;

    range->min_value[0] = 255;
    range->min_value[1] = 255;
    range->min_value[2] = 255;

    range->max_value[0] = 0;
    range->max_value[1] = 0;
    range->max_value[2] = 0;

    for (uint32 subj = 0; subj < header.block_height; subj++)
    for (uint32 subi = 0; subi < header.block_width; subi++)
    {
        uint8 *src_pixel = input.query_data() + input.query_block_offset(x + subi, y + subj);
        uint32 index = subj * header.block_width + subi;

        pixel_set[index++] = src_pixel[0];
        pixel_set[index++] = src_pixel[1];
        pixel_set[index++] = src_pixel[2];
    }     

    if (base_failed(compute_linear_squares_3(pixel_set, header.block_width * header.block_height, range->min_value, range->max_value)))
    {
        return BASE_ERROR_EXECUTION_FAILURE;
    }

    return BASE_SUCCESS;
}

