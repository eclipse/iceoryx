// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2021 by AVIN Systems Private Limited All rights reserved.
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

#ifndef IOX_POSH_POPO_SMARTCHUNK_INL
#define IOX_POSH_POPO_SMARTCHUNK_INL

namespace iox
{
namespace popo
{
namespace internal
{
template <typename TransmissionInterface, typename T, typename H>
inline SmartChunkPrivateData<TransmissionInterface, T, H>::SmartChunkPrivateData(
    cxx::unique_ptr<T>&& smartchunkUniquePtr, TransmissionInterface& transmitter) noexcept
    : smartchunkUniquePtr(std::move(smartchunkUniquePtr))
    , transmitterRef(transmitter)
{
}

template <typename TransmissionInterface, typename T, typename H>
inline SmartChunkPrivateData<TransmissionInterface, const T, H>::SmartChunkPrivateData(
    cxx::unique_ptr<const T>&& smartchunkUniquePtr) noexcept
    : smartchunkUniquePtr(std::move(smartchunkUniquePtr))
{
}

} // namespace internal

template <typename TransmissionInterface, typename T, typename H>
template <typename S, typename>
inline SmartChunk<TransmissionInterface, T, H>::SmartChunk(cxx::unique_ptr<T>&& smartchunkUniquePtr,
                                                           TransmissionInterface& transmitter) noexcept
    : m_members({std::move(smartchunkUniquePtr), transmitter})
{
}

template <typename TransmissionInterface, typename T, typename H>
template <typename S, typename>
inline SmartChunk<TransmissionInterface, T, H>::SmartChunk(cxx::unique_ptr<const T>&& smartchunkUniquePtr) noexcept
    : m_members(std::move(smartchunkUniquePtr))
{
}

template <typename TransmissionInterface, typename T, typename H>
inline T* SmartChunk<TransmissionInterface, T, H>::operator->() noexcept
{
    return get();
}

template <typename TransmissionInterface, typename T, typename H>
inline const T* SmartChunk<TransmissionInterface, T, H>::operator->() const noexcept
{
    return get();
}

template <typename TransmissionInterface, typename T, typename H>
inline T& SmartChunk<TransmissionInterface, T, H>::operator*() noexcept
{
    return *get();
}

template <typename TransmissionInterface, typename T, typename H>
inline const T& SmartChunk<TransmissionInterface, T, H>::operator*() const noexcept
{
    return *get();
}

template <typename TransmissionInterface, typename T, typename H>
inline SmartChunk<TransmissionInterface, T, H>::operator bool() const noexcept
{
    return get() != nullptr;
}

template <typename TransmissionInterface, typename T, typename H>
inline T* SmartChunk<TransmissionInterface, T, H>::get() noexcept
{
    return m_members.smartchunkUniquePtr.get();
}

template <typename TransmissionInterface, typename T, typename H>
inline const T* SmartChunk<TransmissionInterface, T, H>::get() const noexcept
{
    return m_members.smartchunkUniquePtr.get();
}

template <typename TransmissionInterface, typename T, typename H>
inline typename SmartChunk<TransmissionInterface, T, H>::ConditionalConstChunkHeader_t*
SmartChunk<TransmissionInterface, T, H>::getChunkHeader() noexcept
{
    return mepoo::ChunkHeader::fromUserPayload(m_members.smartchunkUniquePtr.get());
}

template <typename TransmissionInterface, typename T, typename H>
inline const mepoo::ChunkHeader* SmartChunk<TransmissionInterface, T, H>::getChunkHeader() const noexcept
{
    return mepoo::ChunkHeader::fromUserPayload(m_members.smartchunkUniquePtr.get());
}

template <typename TransmissionInterface, typename T, typename H>
inline T* SmartChunk<TransmissionInterface, T, H>::release() noexcept
{
    return m_members.smartchunkUniquePtr.release();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_SMARTCHUNK_INL
