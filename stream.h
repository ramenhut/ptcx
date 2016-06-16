
/*
// Copyright (c) 1998-2010 Joe Bertolami. All Right Reserved.
//
// stream.h
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

#ifndef __STREAM_H__	
#define __STREAM_H__	

#include "base.h"
#include "ring_buffer.h"

namespace base {

class stream
{

public:

    virtual void empty() = 0;
    virtual bool is_full() const = 0;
    virtual bool is_empty() const = 0;

    virtual uint32 query_occupancy() const = 0;

    virtual status read_data(void *output, uint32 size, uint32 *bytes_read = 0) = 0;
    virtual status write_data(void *input, uint32 size, uint32 *bytes_written = 0) = 0;
};

class memory_stream : public stream
{
    BASE_DISABLE_COPY_AND_ASSIGN(memory_stream);

protected:

    ring_buffer<uint8> data;

public:

    memory_stream();
    virtual ~memory_stream();

    virtual uint32 resize_capacity(uint32 new_capacity);
    virtual uint32 query_occupancy() const;

    virtual void clear();
    virtual void empty();

    virtual bool is_full() const;
    virtual bool is_empty() const;

    virtual void *query_write_pointer() const;
    virtual void *query_read_pointer() const;

    virtual void advance_write_pointer(uint32 amount);
    virtual void advance_read_pointer(uint32 amount);

    virtual status read_data(void *output, uint32 size, uint32 *bytes_read = 0);
    virtual status write_data(void *input, uint32 size, uint32 *bytes_written = 0);
};

} // namespace base

#endif // __STREAM_H__





