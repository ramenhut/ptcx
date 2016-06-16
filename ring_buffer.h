
/*
// Copyright (c) 1998-2010 Joe Bertolami. All Right Reserved.
//
// list.h
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

#ifndef __RING_BUFFER_H__	
#define __RING_BUFFER_H__	

#include "base.h"
#include <vector>

#define BASE_MAX_ARRAY_SIZE             (4*BASE_MB)

// A simple round robin ring buffer designed for single threaded or 
// single producer + single consumer scenarios.

namespace base {

BASE_TEMPLATE_T class ring_buffer
{
    BASE_DISABLE_COPY_AND_ASSIGN(ring_buffer);

protected:
    
    std::vector<T> data;
    uint32 read_index;
    uint32 write_index;

public:
    
    ring_buffer();
    virtual ~ring_buffer();
    
    virtual uint32 query_occupancy() const;
    virtual uint32 query_capacity() const;
    virtual uint32 resize_capacity(uint32 new_capacity);

    virtual status write(const T &element);
    virtual status read(T *element);

    virtual T *peek();                                  // access the latest data without a dequeue
    virtual status skip();                              // dequeue the next element in the buffer
    
    virtual void clear();                               // deallocates the buffer
    virtual void empty();                               // evicts out the occupants

    virtual bool is_full() const;
    virtual bool is_empty() const;

    // Direct access to the data (unsafe)    
    virtual T *query_item(uint32 index) const;

    virtual uint32 query_write_position() const;
    virtual uint32 query_read_position() const;

    // Pointer adjustments will fail if there is insufficient space.
    virtual status advance_write_position(uint32 amount);
    virtual status advance_read_position(uint32 amount);
};

BASE_TEMPLATE_T ring_buffer<T>::ring_buffer()
{
    read_index = 0;
    write_index = 0;
}

BASE_TEMPLATE_T ring_buffer<T>::~ring_buffer() {}

BASE_TEMPLATE_T uint32 ring_buffer<T>::query_capacity() const
{
    return (data.size());
}

BASE_TEMPLATE_T uint32 ring_buffer<T>::query_occupancy() const
{
    // This is not thread safe if there are multiple producers 
    // or multiple consumers that use it.
    return (write_index - read_index);
}

BASE_TEMPLATE_T uint32 ring_buffer<T>::resize_capacity(uint32 new_capacity)
{
    if (BASE_PARAM_CHECK)
    {
        if (new_capacity > BASE_MAX_ARRAY_SIZE)
        {
            base_post_error(BASE_ERROR_CAPACITY_LIMIT);
            return 0;
        }
    }
    
    read_index = 0;
    write_index = 0;
    data.resize(new_capacity);
     
    if (data.size() != new_capacity)
    {
        base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    return new_capacity;
}

BASE_TEMPLATE_T bool ring_buffer<T>::is_full() const
{ 
    return (write_index - read_index) >= data.size(); 
}

BASE_TEMPLATE_T bool ring_buffer<T>::is_empty() const
{ 
    return (write_index - read_index) == 0;
}

BASE_TEMPLATE_T status ring_buffer<T>::write(const T &element)
{
    // We're the only producer - if there's space to write, there
    // will only ever be more space to write due to consumer activity.
    
    if (!is_full())
    {
        data[write_index % data.size()] = element;
        write_index++;
        
        return BASE_SUCCESS;
    }
    
    return BASE_ERROR_CAPACITY_LIMIT;
}

BASE_TEMPLATE_T status ring_buffer<T>::read(T *element)
{
    if (BASE_PARAM_CHECK)
    {
        if (!element)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }
    
    // We're the only consumer - if there's anything to read, there
    // will only ever be more to read due to producer activity
    
    if (!is_empty())
    {
        (*element) = data[read_index % data.size()];
        read_index++;
        
        return BASE_SUCCESS;
    }
    
    return BASE_ERROR_CAPACITY_LIMIT;
}

BASE_TEMPLATE_T void ring_buffer<T>::clear()
{
    // This should never be called unless you intend to deallocate 
    // the buffer and subsequently resize it.

    empty();
    data.clear();
}

BASE_TEMPLATE_T void ring_buffer<T>::empty()
{
    read_index = 0;
    write_index = 0;
}

BASE_TEMPLATE_T T *ring_buffer<T>::query_item(uint32 index) const
{
    return (T *) &data[index];
}

BASE_TEMPLATE_T T *ring_buffer<T>::peek()
{
    if (!is_empty())
    {
        return &data[read_index % data.size()];
    }
    
    return NULL;
}

BASE_TEMPLATE_T status ring_buffer<T>::skip()
{
    if (!is_empty())
    {
        read_index++; // Advance our read pointer over the very next item.
        
        return BASE_SUCCESS;
    }

    return BASE_ERROR_CAPACITY_LIMIT;    
}

BASE_TEMPLATE_T uint32 ring_buffer<T>::query_write_position() const 
{ 
    return write_index % data.size(); 
}

BASE_TEMPLATE_T uint32 ring_buffer<T>::query_read_position() const
{ 
    return read_index % data.size(); 
}

BASE_TEMPLATE_T status ring_buffer<T>::advance_write_position(uint32 amount) 
{ 
    if (BASE_PARAM_CHECK)
    {
        if (0 == amount)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (query_capacity() - query_occupancy() < amount)
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    write_index += amount;

    return BASE_SUCCESS;
}

BASE_TEMPLATE_T status ring_buffer<T>::advance_read_position(uint32 amount) 
{ 
    if (BASE_PARAM_CHECK)
    {
        if (0 == amount)
        {
            return base_post_error(BASE_ERROR_INVALIDARG);
        }
    }

    if (write_index - read_index < amount)
    {
        return base_post_error(BASE_ERROR_OUTOFMEMORY);
    }

    read_index += amount; 

    return BASE_SUCCESS; 
}

} // namespace base

#endif // __RING_BUFFER_H__



