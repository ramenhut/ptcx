
/*
// Copyright (c) 2009-2014 Joe Bertolami. All Right Reserved.
//
// math.h
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

#ifndef __BASE_MATH_H__
#define __BASE_MATH_H__

#include "base.h"

#define	BASE_USE_FAST_32BIT_LOG2 (1)

#define BASE_MAX_INT64           (0x7FFFFFFFFFFFFFFF)
#define BASE_MAX_INT32           (0x7FFFFFFF)
#define BASE_MAX_INT16           (0x7FFF)
#define BASE_MAX_INT8            (0x7F)

#define BASE_MAX_UINT64          (0xFFFFFFFFFFFFFFFF)
#define BASE_MAX_UINT32          (0xFFFFFFFF)
#define BASE_MAX_UINT16          (0xFFFF)
#define BASE_MAX_UINT8           (0xFF)

#define BASE_MIN_INT64           (-BASE_MAX_INT64 - 1)
#define BASE_MIN_INT32           (-BASE_MAX_INT32 - 1)
#define BASE_MIN_INT16           (-BASE_MAX_INT16 - 1)
#define BASE_MIN_INT8            (-BASE_MAX_INT8 - 1)

#define BASE_PI                  (3.14159262f)
#define BASE_INFINITY            (1.0e15f)
#define BASE_EPSILON             (1.0e-5f)
#define BASE_LOG2                (0.3010299956639f) 

#define base_min2( a, b )        ((a) < (b) ? (a) : (b))
#define base_max2( a, b )        ((a) > (b) ? (a) : (b))
#define base_min3( a, b, c )     ((c) < (a) ? ((c) < (b) ? (c) : (b)) : (a) < (b) ? (a) : (b))
#define base_max3( a, b, c )     ((c) > (a) ? ((c) > (b) ? (c) : (b)) : (a) > (b) ? (a) : (b))
#define base_required_bits(n)    (log2((n)) + 1)
#define base_round_out( n, a )	 ((n) < 0 ? (n) - (a) : (n) + (a))

namespace base {

const uint8 log2_byte_lut[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
};

inline uint8 log2(uint8 value) 
{
    return log2_byte_lut[value];
}

inline uint8 log2(uint16 value) 
{
    if (value <= 0xFF) 
    {
        return log2((uint8) value);
    }

    return 8 + log2((uint8) (value >> 8));
}

inline uint8 log2(uint32 value) 
{
#ifdef BASE_USE_FAST_32BIT_LOG2

    if (value <= 0xFFFF) 
    {
        return log2((uint16) value);
    }

    return 16 + log2((uint16) (value >> 16));

#else
    /* This is useful for generating a log lut. */

    if (0 == value) 
    {
        return 0;
    }

    /*
    // This will provide accurate values for pow-2 aligned
    // inputs. For all else, the return will be truncated to the
    // nearest integer.
    */

    uint32 result = 0;

    while (value >>= 1)
    {
        result++;
    }

    return result;

#endif
}

inline int8 sign(int8 value)
{
    int8 is_non_zero = !!value;
    int8 missing_sign_bit = !(value & 0x80);

    // Branchless sign that returns zero for zero values.
    return (missing_sign_bit - !missing_sign_bit) * is_non_zero;
}

inline int16 sign(int16 value)
{
    int16 is_non_zero = !!value;
    int16 missing_sign_bit = !(value & 0x8000);
    return (missing_sign_bit - !missing_sign_bit) * is_non_zero;
}

inline int32 sign(int32 value)
{
    int32 is_non_zero = !!value;
    int32 missing_sign_bit = !(value & 0x80000000);
    return (missing_sign_bit - !missing_sign_bit) * is_non_zero;
}

/*
inline float log2(float value)
{
   int *exp_ptr = reinterpret_cast<int *>(&value);
   int x = *exp_ptr;
   int log_2 = ((x >> 23) & 255) - 128;
   x &= ~(255 << 23);
   x += 127 << 23;
   *exp_ptr = x;

   value = ((-1.0f / 3.0f) * value + 2.0f) * value - 2.0f / 3.0f;
   return (value + log_2) * 0.69314718f;
}
*/

inline float log2(float value)
{
    int *int_value = reinterpret_cast<int *>(&value);
    float log_2 = (float)(((*int_value >> 23) & 255) - 128);              
    *int_value &= ~(255 << 23);
    *int_value += 127 << 23;
    log_2 += (-0.34484843f * value + 2.02466578f) * value - 0.67487759f;

    return (log_2);
}

inline int8 abs(int8 value) 
{
    if (value == BASE_MIN_INT8) 
        return BASE_MAX_INT8;

    return (value < 0 ? -value : value);
}

inline int16 abs(int16 value) 
{
    if (value == BASE_MIN_INT16) 
        return BASE_MAX_INT16;

    return (value < 0 ? -value : value);
}

inline int32 abs(int32 value) 
{
    if (value == BASE_MIN_INT32) 
        return BASE_MAX_INT32;

    return (value < 0 ? -value : value);
}

inline int16 clip_range(int16 value, int16 min, int16 max) 
{
    return (value < min ? min : (value > max ? max : value));
}

inline int16 saturate(int32 input )
{
    return clip_range(input, 0, 255);
}

inline bool is_pow2(uint32 value)
{
    return (0 == (value & (value - 1)));
}

inline int32 rounded_div(int32 numer, int32 denom)
{
    if ((numer & 0x80000000) ^ (denom & 0x80000000))
    {
        return (numer - denom / 2) / denom;
    }

    return (numer + denom / 2) / denom;
}

inline int32 rounded_div_pow2(int32 numer, uint32 pos_denom)
{
#if BASE_DEBUG
    if (!is_pow2(pos_denom))
    {
        base_post_error(BASE_ERROR_INVALIDARG);
    }
#endif

    if (numer & 0x80000000)
    {
        return (numer - (pos_denom >> 1)) >> (log2(pos_denom));
    }

    return (numer + (pos_denom >> 1)) >> (log2(pos_denom));
}

inline uint32 greater_multiple(uint32 value, uint32 multiple) 
{
    uint32 mod = value % multiple;

    if (0 != mod) 
    {
        value += multiple - mod;
    }

    return value;
}

inline uint32 align(uint32 value, uint32 alignment) 
{
    return greater_multiple(value, alignment);
}   

inline uint32 align16(uint32 value)
{
    return (value & 0xF ? value + ~(value & 0xF) + 1 : value);
}

inline uint32 align8(uint32 value)
{
    return (value & 0x7 ? value + ~(value & 0x7 ) + 1 : value);
}

inline uint32 align2( uint32 value )
{
    if (is_pow2(value))
    {
        return value;
    }
    
    int32 power = 0;
    
    while (value) 
    {
        value >>= 1;
        power++;
    }
    
    return 1 << power;
}

inline float inv_sqrt(float32 f)
{
    // Newton-Raphson approximation with a curiously awesome initial guess
    float32 half = 0.5f * f;
    
    int32 i = *reinterpret_cast<int32 *>(&f);
    
    i = 0x5f3759df - (i >> 1);
    f = *reinterpret_cast<float32 *>(&i);
    f = f * (1.5f - half * f * f);
    // f = f * (1.5f - half * f * f);   // if we want extra precision we do an extra degree
    return f;
}

inline float sqrtf(float f)
{    
    return 1.0f / inv_sqrt(f);
}

inline uint32 sqrt(uint32 f)
{  
    return (1.0f / inv_sqrt(f) + 0.5f);
}

} // namespace base

#endif // __BASE_MATH_H__