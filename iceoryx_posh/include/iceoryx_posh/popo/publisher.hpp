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

#ifndef IOX_POSH_POPO_TYPED_PUBLISHER_HPP
#define IOX_POSH_POPO_TYPED_PUBLISHER_HPP

#include "iceoryx_posh/popo/base_publisher.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_utils/cxx/type_traits.hpp"

namespace iox
{
namespace popo
{
///
/// @brief The PublisherInterface class defines the publisher interface used by the Sample class to make it generic.
/// This allows any publisher specialization to be stored as a reference by the Sample class.
/// It is also needed to avoid circular dependencies between Sample and Publisher.
///
template <typename T, typename H>
class PublisherInterface
{
  public:
    virtual void publish(Sample<T, H>&& sample) noexcept = 0;

    virtual ~PublisherInterface(){};

  protected:
    PublisherInterface() = default;
};
template <typename T, typename H = mepoo::NoUserHeader, typename BasePublisher_t = BasePublisher<>>
class PublisherImpl : public BasePublisher_t, public PublisherInterface<T, H>
{
    static_assert(!std::is_void<T>::value, "The type `T` must not be void. Use the UntypedPublisher for void types.");
    static_assert(!std::is_void<H>::value, "The user-header `H` must not be void.");

    static_assert(!std::is_const<T>::value, "The type `T` must not be const.");
    static_assert(!std::is_reference<T>::value, "The type `T` must not be a reference.");
    static_assert(!std::is_pointer<T>::value, "The type `T` must not be a pointer.");

    static_assert(!std::is_const<H>::value, "The user-header `H` must not be const.");
    static_assert(!std::is_reference<H>::value, "The user-header `H` must not be a reference.");
    static_assert(!std::is_pointer<H>::value, "The user-header must `H` not be a pointer.");

  public:
    PublisherImpl(const capro::ServiceDescription& service,
                  const PublisherOptions& publisherOptions = PublisherOptions());
    PublisherImpl(const PublisherImpl& other) = delete;
    PublisherImpl& operator=(const PublisherImpl&) = delete;
    PublisherImpl(PublisherImpl&& rhs) = default;
    PublisherImpl& operator=(PublisherImpl&& rhs) = default;
    virtual ~PublisherImpl() = default;

    ///
    /// @brief loan Get a sample from loaned shared memory and consctruct the data with the given arguments.
    /// @param args Arguments used to construct the data.
    /// @return An instance of the sample that resides in shared memory or an error if unable ot allocate memory to
    /// loan.
    /// @details The loaned sample is automatically released when it goes out of scope.
    ///
    template <typename... Args>
    cxx::expected<Sample<T, H>, AllocationError> loan(Args&&... args) noexcept;

    ///
    /// @brief publish Publishes the given sample and then releases its loan.
    /// @param sample The sample to publish.
    ///
    void publish(Sample<T, H>&& sample) noexcept override;

    ///
    /// @brief publishCopyOf Copy the provided value into a loaned shared memory chunk and publish it.
    /// @param val Value to copy.
    /// @return Error if unable to allocate memory to loan.
    ///
    cxx::expected<AllocationError> publishCopyOf(const T& val) noexcept;
    ///
    /// @brief publishResultOf Loan a sample from memory, execute the provided callable to write to it, then publish it.
    /// @param c Callable with the signature void(T*, ArgTypes...) that write's it's result to T*.
    /// @param args The arguments of the callable.
    /// @return Error if unable to allocate memory to loan.
    ///
    template <typename Callable, typename... ArgTypes>
    cxx::expected<AllocationError> publishResultOf(Callable c, ArgTypes... args) noexcept;

  protected:
    using BasePublisher_t::port;

  private:
    Sample<T, H> convertChunkHeaderToSample(mepoo::ChunkHeader* const header) noexcept;

    cxx::expected<Sample<T, H>, AllocationError> loanSample() noexcept;

    using PublisherSampleDeleter = SampleDeleter<typename BasePublisher_t::PortType>;
    PublisherSampleDeleter m_sampleDeleter{port()};
};

template <typename T, typename H = mepoo::NoUserHeader>
using Publisher = PublisherImpl<T, H>;

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/publisher.inl"

#endif // IOX_POSH_POPO_TYPED_PUBLISHER_HPP
