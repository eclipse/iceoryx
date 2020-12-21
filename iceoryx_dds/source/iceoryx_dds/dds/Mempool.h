// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*!
 * @file Mempool.h
 */

#ifndef _MEMPOOL_MEMPOOL_H_
#define _MEMPOOL_MEMPOOL_H_

#include <fastcdr/Cdr.h>


namespace Mempool
{

/*!
    * @brief This struct represents the structure Chunk.
    * @ingroup MEMPOOL
    */
struct Chunk
{
    uint32_t size;
    uint8_t* payload;

    Chunk()
        : size(0)
        , payload(nullptr)
    {
    }

    Chunk(uint32_t _size, uint8_t* _payload)
        : size(_size)
        , payload(_payload)
    {
    }

    static size_t getCdrSerializedSize(const Mempool::Chunk& data)
    {
        // (void)data;
        size_t alignment = 0;
        alignment += 4 + eprosima::fastcdr::Cdr::alignment(alignment, 4);

        if (data.size > 0)
        {
            alignment += (data.size * 1) + eprosima::fastcdr::Cdr::alignment(alignment, 1);
        }
        alignment += 4 + eprosima::fastcdr::Cdr::alignment(alignment, 4);
        return alignment;
    }

    static size_t getMaxCdrSerializedSize()
    {
        size_t alignment = 0;
        alignment += 4 + eprosima::fastcdr::Cdr::alignment(alignment, 4);
        alignment += (100 * 1) + eprosima::fastcdr::Cdr::alignment(alignment, 1);
        return alignment;
    }
};

} // namespace Mempool

#endif // _MEMPOOL_MEMPOOL_H_