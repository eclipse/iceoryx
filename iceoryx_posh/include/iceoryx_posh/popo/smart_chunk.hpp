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

#ifndef IOX_POSH_POPO_SMARTCHUNK_HPP
#define IOX_POSH_POPO_SMARTCHUNK_HPP

#include "iceoryx_hoofs/cxx/type_traits.hpp"
#include "iceoryx_hoofs/cxx/unique_ptr.hpp"
#include "iceoryx_posh/internal/popo/ports/client_server_port_types.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"

namespace iox
{
namespace popo
{
// template <typename T, typename H>
// class SubscriberInterface;


namespace internal
{
/// @brief helper struct for smartchunk
template <typename TransmissionInterface, typename T, typename H>
struct SmartChunkPrivateData
{
    SmartChunkPrivateData(cxx::unique_ptr<T>&& smartchunkUniquePtr, TransmissionInterface& transmitter) noexcept;

    SmartChunkPrivateData(SmartChunkPrivateData&& rhs) noexcept = default;
    SmartChunkPrivateData& operator=(SmartChunkPrivateData&& rhs) noexcept = default;

    SmartChunkPrivateData(const SmartChunkPrivateData&) = delete;
    SmartChunkPrivateData& operator=(const SmartChunkPrivateData&) = delete;

    cxx::unique_ptr<T> smartchunkUniquePtr;
    std::reference_wrapper<TransmissionInterface> transmitterRef;
};

/// @brief specialization of helper struct for smartchunk for const T
template <typename TransmissionInterface, typename T, typename H>
struct SmartChunkPrivateData<TransmissionInterface, const T, H>
{
    SmartChunkPrivateData(cxx::unique_ptr<const T>&& smartchunkUniquePtr) noexcept;

    SmartChunkPrivateData(SmartChunkPrivateData&& rhs) noexcept = default;
    SmartChunkPrivateData& operator=(SmartChunkPrivateData&& rhs) noexcept = default;

    SmartChunkPrivateData(const SmartChunkPrivateData&) = delete;
    SmartChunkPrivateData& operator=(const SmartChunkPrivateData&) = delete;

    cxx::unique_ptr<const T> smartchunkUniquePtr;
};
} // namespace internal

///
/// @brief The SmartChunk class is a mutable abstraction over types which are written to loaned shared memory.
/// These smartchunks are transferable to the iceoryx system.
///
template <typename TransmissionInterface,
          typename T,
          typename H = cxx::add_const_conditionally_t<mepoo::NoUserHeader, T>>
class SmartChunk
{
    static_assert(std::is_const<T>::value == std::is_const<H>::value,
                  "The type `T` and the user-header `H` must be equal in their const qualifier to ensure the same "
                  "access restrictions for the user-header as for the smartchunk data!");

  protected:
    /// @brief Helper type to enable the constructor for the producer, i.e. when T has no const qualifier
    template <typename S, typename TT>
    using ForProducerOnly = std::enable_if_t<std::is_same<S, TT>::value && !std::is_const<TT>::value, S>;

    /// @brief Helper type to enable the constructor for the consumer, i.e. when T has a const qualifier
    template <typename S, typename TT>
    using ForConsumerOnly = std::enable_if_t<std::is_same<S, TT>::value && std::is_const<TT>::value, S>;

    /// @brief Helper type to enable some methods only if a user-header is used
    template <typename R, typename HH>
    using HasUserHeader =
        std::enable_if_t<std::is_same<R, HH>::value && !std::is_same<R, mepoo::NoUserHeader>::value, R>;

  public:
    /// @brief Constructor for a SmartChunk used by the transmitter
    /// @tparam S is a dummy template parameter to enable the constructor only for non-const T
    /// @param smartchunkUniquePtr is a `rvalue` to a `cxx::unique_ptr<T>` with to the data of the encapsulated type T
    /// @param transmitter is a reference to the transmitter to be able to use the `publish`, `send` and `release`
    /// methods
    template <typename S = T, typename = ForProducerOnly<S, T>>
    SmartChunk(cxx::unique_ptr<T>&& smartchunkUniquePtr, TransmissionInterface& transmitter) noexcept;

    /// @brief Constructor for a SmartChunk used by the consumer
    /// @tparam S is a dummy template parameter to enable the constructor only for const T
    /// @param smartchunkUniquePtr is a `rvalue` to a `cxx::unique_ptr<T>` with to the data of the encapsulated type T
    template <typename S = T, typename = ForConsumerOnly<S, T>>
    SmartChunk(cxx::unique_ptr<const T>&& smartchunkUniquePtr) noexcept;

    ~SmartChunk() noexcept = default;

    SmartChunk<TransmissionInterface, T, H>&
    operator=(SmartChunk<TransmissionInterface, T, H>&& rhs) noexcept = default;
    SmartChunk(SmartChunk<TransmissionInterface, T, H>&& rhs) noexcept = default;

    SmartChunk(const SmartChunk<TransmissionInterface, T, H>&) = delete;
    SmartChunk<TransmissionInterface, T, H>& operator=(const SmartChunk<TransmissionInterface, T, H>&) = delete;

    ///
    /// @brief Transparent access to the encapsulated type.
    /// @return a pointer to the encapsulated type.
    ///
    T* operator->() noexcept;

    ///
    /// @brief Transparent read-only access to the encapsulated type.
    /// @return a const pointer to the encapsulated type.
    ///
    const T* operator->() const noexcept;

    ///
    /// @brief Provides a reference to the encapsulated type.
    /// @return A T& to the encapsulated type.
    ///
    T& operator*() noexcept;

    ///
    /// @brief Provides a const reference to the encapsulated type.
    /// @return A const T& to the encapsulated type.
    ///
    const T& operator*() const noexcept;

    ///
    /// @brief Indicates whether the smartchunk is valid, i.e. refers to allocated memory.
    /// @return true if the smartchunk is valid, false otherwise.
    ///
    operator bool() const noexcept;

    ///
    /// @brief Mutable access to the encapsulated type loaned to the smartchunk.
    /// @return a pointer to the encapsulated type.
    ///
    T* get() noexcept;

    ///
    /// @brief Read-only access to the encapsulated type loaned to the smartchunk.
    /// @return a const pointer to the encapsulated type.
    ///
    const T* get() const noexcept;

    /// @brief Helper type to ensure the access to the ChunkHeader has the same const qualifier as the access to the
    /// smartchunk data
    using ConditionalConstChunkHeader_t = cxx::add_const_conditionally_t<mepoo::ChunkHeader, T>;
    ///
    /// @brief Retrieve the ChunkHeader of the underlying memory chunk loaned to the smartchunk.
    /// @return The ChunkHeader of the underlying memory chunk.
    ///
    ConditionalConstChunkHeader_t* getChunkHeader() noexcept;

    ///
    /// @brief Retrieve the ChunkHeader of the underlying memory chunk loaned to the smartchunk.
    /// @return The const ChunkHeader of the underlying memory chunk.
    ///
    const mepoo::ChunkHeader* getChunkHeader() const noexcept;

  private:
    template <typename, typename, typename>
    friend class PublisherImpl;

    template <typename, typename, typename, typename>
    friend class ClientImpl;

    template <typename, typename, typename, typename>
    friend class ServerImpl;

  protected:
    /// @note used by the producer to release the chunk ownership from the `SmartChunk` after publishing the chunk and
    /// therefore preventing the invocation of the custom deleter
    T* release() noexcept;

    internal::SmartChunkPrivateData<TransmissionInterface, T, H> m_members;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/smart_chunk.inl"

#endif // IOX_POSH_POPO_SMARTCHUNK_HPP
