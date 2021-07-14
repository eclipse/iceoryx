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

#ifndef IOX_POSH_POPO_RESPONSE_HPP
#define IOX_POSH_POPO_RESPONSE_HPP

#include "iceoryx_posh/popo/smart_chunk.hpp"

namespace iox
{
namespace popo
{
template <typename T, typename H>
class RpcInterface;

///
/// @brief The Response class is a mutable abstraction over types which are written to loaned shared memory.
/// These responses are publishable to the iceoryx system.
///
template <typename T, typename H = cxx::add_const_conditionally_t<mepoo::NoUserHeader, T>>
class Response : public SmartChunk<RpcInterface<T, H>, T, H>
{
    static_assert(std::is_const<T>::value == std::is_const<H>::value,
                  "The type `T` and the user-header `H` must be equal in their const qualifier to ensure the same "
                  "access restrictions for the user-header as for the response data!");

    using Base_t =  SmartChunk<RpcInterface<T, H>, T, H>;
    /// @brief Helper type to enable the constructor for the producer, i.e. when T has a non const qualifier
    template <typename S, typename TT>
    using ForProducerOnly = typename Base_t::template ForProducerOnly<S, TT>;

    /// @brief Helper type to enable the constructor for the consumer, i.e. when T has a const qualifier
    template <typename S, typename TT>
    using ForConsumerOnly = typename Base_t::template ForConsumerOnly<S, TT>;

    /// @brief Helper type to enable some methods only if a user-header is used
    template <typename R, typename HH>
    using HasUserHeader = typename Base_t::template HasUserHeader<R, HH>;

  public:
    /// @brief Constructor for a Response used by the Producer
    /// @tparam S is a dummy template parameter to enable the constructor only for non-const T
    /// @param responseUniquePtr is a `rvalue` to a `cxx::unique_ptr<T>` with to the data of the encapsulated type T
    /// @param producer is a reference to the producer to be able to use the `publish` and `release` methods
    template <typename S = T, typename = ForProducerOnly<S, T>>
    Response(cxx::unique_ptr<T>&& responseUniquePtr, RpcInterface<T, H>& Producer) noexcept;

    /// @brief Constructor for a Response used by the Subscriber
    /// @tparam S is a dummy template parameter to enable the constructor only for const T
    /// @param responseUniquePtr is a `rvalue` to a `cxx::unique_ptr<T>` with to the data of the encapsulated type T
    template <typename S = T, typename = ForConsumerOnly<S, T>>
    Response(cxx::unique_ptr<const T>&& responseUniquePtr) noexcept;

    ~Response() noexcept = default;

    Response<T, H>& operator=(Response<T, H>&& rhs) noexcept = default;
    Response(Response<T, H>&& rhs) noexcept = default;

    Response(const Response<T, H>&) = delete;
    Response<T, H>& operator=(const Response<T, H>&) = delete;

    ///
    /// @brief Retrieve the user-header of the underlying memory chunk loaned to the response.
    /// @return The user-header of the underlying memory chunk.
    ///
    template <typename R = H, typename = HasUserHeader<R, H>>
    R& getResponseHeader() noexcept;

    ///
    /// @brief Retrieve the user-header of the underlying memory chunk loaned to the response.
    /// @return The user-header of the underlying memory chunk.
    ///
    template <typename R = H, typename = HasUserHeader<R, H>>
    const R& getResponseHeader() const noexcept;

    ///
    /// @brief send the response via the producer from which it was loaned and automatically
    /// release ownership to it.
    /// @details Only available for non-const type T.
    ///
    template <typename S = T, typename = ForProducerOnly<S, T>>
    void send() noexcept;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/response.inl"

#endif // IOX_POSH_POPO_RESPONSE_HPP
