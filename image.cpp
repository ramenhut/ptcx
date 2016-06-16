
#include "image.h"

namespace imagine {

uint8 channel_count_from_format(IGN_IMAGE_FORMAT format)
{
    switch (format) 
    {
        case IGN_IMAGE_FORMAT_NONE: return 0;
        case IGN_IMAGE_FORMAT_R8G8B8: return 3;
    }

    base_post_error(BASE_ERROR_INVALIDARG);

    return 0;
}

uint8 pixel_rate_from_format(IGN_IMAGE_FORMAT format)
{
    switch (format) 
    {
        case IGN_IMAGE_FORMAT_NONE: return 0;
        case IGN_IMAGE_FORMAT_R8G8B8: return 24;
    }

    base_post_error(BASE_ERROR_INVALIDARG);

    return 0;
}

image::image()
{
    image_format = IGN_IMAGE_FORMAT_NONE;
    placement_allocation = false;
    width_in_pixels = 0;
    height_in_pixels = 0;
    bits_per_pixel = 0;
    channel_count = 0;
    data_buffer = 0;
}

image::~image()
{
    // We retain tight control of allocation and deallocation because the underlying 
    // image format is still in flux. Be sure to use create_image and destroy_image.

    deallocate();
}

uint32 image::query_row_pitch() const
{
    return (width_in_pixels * bits_per_pixel) >> 3;
}

uint32 image::query_slice_pitch() const
{
    return query_row_pitch() * height_in_pixels;
}

uint32 image::query_block_offset(uint32 i, uint32 j) const
{
    return (query_row_pitch() * j) + ((i * bits_per_pixel) >> 3);
}

status image::allocate(uint32 size)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == size)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    deallocate();

    data_buffer = new uint8[size];

    if (!data_buffer)
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    memset(data_buffer, 0, size);

    placement_allocation = false;

    return BASE_SUCCESS;
}

status image::set_placement(void *data)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == data)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    deallocate();

    data_buffer = static_cast<uint8 *>(data);

    placement_allocation = true;
    
    return BASE_SUCCESS;
}

void image::deallocate()
{
    if (!placement_allocation)
    {
        delete [] data_buffer;
    }

    data_buffer = 0;
}

status image::set_dimension(uint32 width, uint32 height)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == width || 0 == height)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    // Check for an uninitialized image.
    if (0 == bits_per_pixel || IGN_IMAGE_FORMAT_NONE == image_format)
    {
        // You must call set_image_format prior to calling this function, so that
        // we know how to allocate the image.

        return base_post_error(BASE_ERROR_INVALID_RESOURCE);
    }

    width_in_pixels = width;
    height_in_pixels = height;

    return BASE_SUCCESS;
}

status image::set_image_format(IGN_IMAGE_FORMAT format)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == channel_count_from_format(format))
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    uint32 rate_total = pixel_rate_from_format(format);
    uint8 channel_total = channel_count_from_format(format);

    if (0 != (rate_total % 8))
    {
        // The format is invalid -- it does not contain a byte aligned pixel rate.
        return base_post_error(BASE_ERROR_INVALIDARG);
    }

    image_format = format;
    bits_per_pixel = rate_total;
    channel_count = channel_total;

    return BASE_SUCCESS;
}

uint32 image::query_width() const
{
    return width_in_pixels;
}

uint32 image::query_height() const
{
    return height_in_pixels;
}

uint8 *image::query_data() const
{
    return data_buffer;
}

uint8 image::query_bits_per_pixel() const
{
    return bits_per_pixel;
}

IGN_IMAGE_FORMAT image::query_image_format() const
{
    return image_format;
}

uint8 image::query_channel_count() const
{
    return channel_count;
}

status create_image(IGN_IMAGE_FORMAT format, uint32 width, uint32 height, image *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == width || 0 == height)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }

        if (!output)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (base_failed(output->set_image_format(format)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(output->set_dimension(width, height)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    // All images are required to use byte aligned pixel rates, so there is 
    // no need to align the allocation size.

    if (base_failed(output->allocate((width * height * output->query_bits_per_pixel()) >> 3)))
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    return BASE_SUCCESS;
}

status create_image(IGN_IMAGE_FORMAT format, void *image_data, uint32 width, uint32 height, image *output)
{
    if (BASE_PARAM_CHECK)
    {
        if (0 == width || 0 == height)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }

        if (!image_data || !output)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (base_failed(output->set_image_format(format)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(output->set_dimension(width, height)))
    {
        return base_post_error(BASE_ERROR_EXECUTION_FAILURE);
    }

    if (base_failed(output->set_placement(image_data)))
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    return BASE_SUCCESS;
}

status destroy_image(image *input)
{
    if (BASE_PARAM_CHECK) 
    {
        if (!input)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    input->deallocate();

    return BASE_SUCCESS;
}

} // namespace imagine