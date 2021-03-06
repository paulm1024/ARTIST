/*
 * Copyright 2016 Lukas Dresel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
//
// Created by Lukas on 8/23/2015.
//


#ifdef __cplusplus
extern "C" {
#endif


#include "leb128.h"

#include <stdbool.h>

#include <utility/macros.h>
#include <utility/bit_utils.h>

// Reads an unsigned LEB128 value, updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
uint32_t DecodeUnsignedLeb128(const uint8_t **data) {
    const uint8_t *ptr = *data;
    int result = *(ptr++);
    if (UNLIKELY(result > 0x7f)) {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    return (uint32_t) result;
}

// Reads an unsigned LEB128 + 1 value. updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
// It is possible for this function to return -1.
int32_t DecodeUnsignedLeb128P1(const uint8_t **data) {
    return DecodeUnsignedLeb128(data) - 1;
}

// Reads a signed LEB128 value, updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
int32_t DecodeSignedLeb128(const uint8_t **data) {
    const uint8_t *ptr = *data;
    int32_t result = *(ptr++);
    if (result <= 0x7f) {
        result = (result << 25) >> 25;
    } else {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur <= 0x7f) {
            result = (result << 18) >> 18;
        } else {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur <= 0x7f) {
                result = (result << 11) >> 11;
            } else {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur <= 0x7f) {
                    result = (result << 4) >> 4;
                } else {
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    return result;
}

// Returns the number of bytes needed to encode the value in unsigned LEB128.
uint32_t UnsignedLeb128Size(uint32_t data) {
    // bits_to_encode = (data != 0) ? 32 - CLZ(x) : 1  // 32 - CLZ(data | 1)
    // bytes = ceil(bits_to_encode / 7.0);             // (6 + bits_to_encode) / 7
    uint32_t x = 6 + 32 - COUNT_32BIT_LEADING_ZEROS(data | 1);
    // Division by 7 is done by (x * 37) >> 8 where 37 = ceil(256 / 7).
    // This works for 0 <= x < 256 / (7 * 37 - 256), i.e. 0 <= x <= 85.
    return (x * 37) >> 8;
}

// Returns the number of bytes needed to encode the value in unsigned LEB128.
uint32_t SignedLeb128Size(int32_t data) {
    // Like UnsignedLeb128Size(), but we need one bit beyond the highest bit that differs from sign.
    data = data ^ (data >> 31);
    uint32_t x =
            1 /* we need to encode the sign bit */ + 6 + 32 - COUNT_32BIT_LEADING_ZEROS(data | 1);
    return (x * 37) >> 8;
}

uint8_t *EncodeUnsignedLeb128(uint8_t *dest, uint32_t value) {
    uint8_t out = (uint8_t) (value & 0x7f);
    value >>= 7;
    while (value != 0) {
        *dest++ = (uint8_t) (out | 0x80);
        out = (uint8_t) (value & 0x7f);
        value >>= 7;
    }
    *dest++ = out;
    return dest;
}

uint8_t *EncodeSignedLeb128(uint8_t *dest, int32_t value) {
    uint32_t extra_bits = (uint32_t) (value ^ (value >> 31)) >> 6;
    uint8_t out = (uint8_t) (value & 0x7f);
    while (extra_bits != 0u) {
        *dest++ = (uint8_t) (out | 0x80);
        value >>= 7;
        out = (uint8_t) (value & 0x7f);
        extra_bits >>= 7;
    }
    *dest++ = out;
    return dest;
}


#ifdef __cplusplus
}
#endif
