// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_UTILS_CXX_VARIANT_QUEUE_INL
#define IOX_UTILS_CXX_VARIANT_QUEUE_INL

#include "iceoryx_utils/error_handling/error_handling.hpp"

namespace iox
{
namespace cxx
{
template <typename ValueType, uint64_t Capacity>
inline VariantQueue<ValueType, Capacity>::VariantQueue(const VariantQueueTypes type) noexcept
    : m_type(type)
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        m_fifo.template emplace<concurrent::FiFo<ValueType, Capacity>>();
        break;
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        m_fifo.template emplace<concurrent::SoFi<ValueType, Capacity>>();
        break;
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
        IOX_FALLTHROUGH;
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        m_fifo.template emplace<concurrent::ResizeableLockFreeQueue<ValueType, Capacity>>();
        break;
    }
    }
}

template <typename ValueType, uint64_t Capacity>
optional<ValueType> VariantQueue<ValueType, Capacity>::push(const ValueType& value) noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        auto hadSpace =
            m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_SingleProducerSingleConsumer)>()
                ->push(value);

        return (hadSpace) ? cxx::nullopt : cxx::make_optional<ValueType>(value);
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        ValueType overriddenValue;
        auto hadSpace =
            m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
                ->push(value, overriddenValue);

        return (hadSpace) ? cxx::nullopt : cxx::make_optional<ValueType>(overriddenValue);
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    {
        auto hadSpace =
            m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
                ->tryPush(value);

        return (hadSpace) ? cxx::nullopt : cxx::make_optional<ValueType>(value);
    }
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->push(value);
    }
    }

    return cxx::nullopt;
}

template <typename ValueType, uint64_t Capacity>
inline optional<ValueType> VariantQueue<ValueType, Capacity>::pop() noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_SingleProducerSingleConsumer)>()
            ->pop();
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        ValueType returnType;
        auto hasReturnType =
            m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
                ->pop(returnType);

        return (hasReturnType) ? make_optional<ValueType>(returnType) : cxx::nullopt;
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->pop();
    }
    }

    return cxx::nullopt;
}

template <typename ValueType, uint64_t Capacity>
inline bool VariantQueue<ValueType, Capacity>::empty() const noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_SingleProducerSingleConsumer)>()
            ->empty();
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
            ->empty();
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->empty();
    }
    }

    return true;
}

template <typename ValueType, uint64_t Capacity>
inline uint64_t VariantQueue<ValueType, Capacity>::size() noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_SingleProducerSingleConsumer)>()
            ->size();
        break;
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
            ->size();
        break;
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->size();
        break;
    }
    }

    return 0U;
}


template <typename ValueType, uint64_t Capacity>
inline bool VariantQueue<ValueType, Capacity>::setCapacity(const uint64_t newCapacity) noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        /// @todo must be implemented for FiFo
        assert(false);
        return false;
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
            ->setCapacity(newCapacity);
        return true;
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        // we may discard elements in the queue if the size is reduced and the fifo contains too many elements
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->setCapacity(newCapacity);
    }
    }
    return false;
}

template <typename ValueType, uint64_t Capacity>
inline uint64_t VariantQueue<ValueType, Capacity>::capacity() const noexcept
{
    switch (m_type)
    {
    case VariantQueueTypes::FiFo_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_SingleProducerSingleConsumer)>()
            ->capacity();
        break;
    }
    case VariantQueueTypes::SoFi_SingleProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
            ->capacity();
        break;
    }
    case VariantQueueTypes::FiFo_MultiProducerSingleConsumer:
    case VariantQueueTypes::SoFi_MultiProducerSingleConsumer:
    {
        return m_fifo
            .template get_at_index<static_cast<uint64_t>(VariantQueueTypes::FiFo_MultiProducerSingleConsumer)>()
            ->capacity();
        break;
    }
    }

    return 0U;
}

template <typename ValueType, uint64_t Capacity>
inline typename VariantQueue<ValueType, Capacity>::fifo_t&
VariantQueue<ValueType, Capacity>::getUnderlyingFiFo() noexcept
{
    return m_fifo;
}

} // namespace cxx
} // namespace iox

#endif // IOX_UTILS_CXX_VARIANT_QUEUE_INL
