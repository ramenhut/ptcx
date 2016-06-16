#include "stream.h"

namespace base {

memory_stream::memory_stream() {}
memory_stream::~memory_stream() {}

uint32 memory_stream::resize_capacity(uint32 new_capacity)
{
    return data.resize_capacity(new_capacity);
}

uint32 memory_stream::query_occupancy() const
{
    return data.query_occupancy();
}

void memory_stream::clear()
{
    data.clear();
}

void memory_stream::empty()
{
    data.empty();
}

bool memory_stream::is_full() const
{
    return data.is_full();
}

bool memory_stream::is_empty() const
{
    return data.is_empty();
}

status memory_stream::read_data(void *output, uint32 size, uint32 *bytes_read)
{
    if (BASE_PARAM_CHECK)
    {
        if (!output || 0 == size)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (is_empty())
    {
        if (bytes_read)
        {
            *bytes_read = 0;
        }

        return BASE_ERROR_INVALID_RESOURCE;
    }

    uint32 internal_to_read = min(size, data.query_occupancy());
    memcpy(output, data.peek(), internal_to_read);
    data.advance_read_position(internal_to_read);

    if (bytes_read)
    {
        *bytes_read = internal_to_read;
    }

    return BASE_SUCCESS;
}

status memory_stream::write_data(void *input, uint32 size, uint32 *bytes_written)
{
    if (BASE_PARAM_CHECK)
    {
        if (!input || 0 == size)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (is_full() || !data.query_capacity())
    {
        if (bytes_written)
        {
            *bytes_written = 0;
        }

        return BASE_ERROR_INVALID_RESOURCE;
    }

    uint32 internal_space_avail = data.query_capacity() - data.query_occupancy();
    uint32 internal_write_index = data.query_write_position();
    uint32 internal_to_write = min(size, internal_space_avail);

    memcpy(data.query_item(internal_write_index), input, internal_to_write);
    data.advance_write_position(internal_to_write);

    if (bytes_written)
    {
        *bytes_written = internal_to_write;
    }

    return BASE_SUCCESS;
}

void *memory_stream::query_write_pointer() const
{
    uint32 write_index = data.query_write_position();
    return data.query_item(write_index);
}

void *memory_stream::query_read_pointer() const
{
    uint32 read_index = data.query_read_position();
    return data.query_item(read_index);
}

void memory_stream::advance_write_pointer(uint32 amount)
{
    data.advance_write_position(amount);
}

void memory_stream::advance_read_pointer(uint32 amount)
{
    data.advance_read_position(amount);
}

} // namespace base
