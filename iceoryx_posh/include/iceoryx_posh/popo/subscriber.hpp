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

#ifndef IOX_POSH_POPO_TYPED_SUBSCRIBER_HPP
#define IOX_POSH_POPO_TYPED_SUBSCRIBER_HPP

#include "iceoryx_posh/internal/popo/sample_deleter.hpp"
#include "iceoryx_posh/popo/base_subscriber.hpp"

namespace iox
{
namespace popo
{
template <typename T, typename H = iox::mepoo::NoUserHeader, typename BaseSubscriber_t = BaseSubscriber<>>
class SubscriberImpl : public BaseSubscriber_t
{
    using SelfType = SubscriberImpl<T, BaseSubscriber_t>;
    static_assert(!std::is_void<T>::value, "Type must not be void. Use the UntypedSubscriber for void types.");

  public:
    SubscriberImpl(const capro::ServiceDescription& service,
                   const SubscriberOptions& subscriberOptions = SubscriberOptions());
    SubscriberImpl(const SubscriberImpl& other) = delete;
    SubscriberImpl& operator=(const SubscriberImpl&) = delete;
    SubscriberImpl(SubscriberImpl&& rhs) = delete;
    SubscriberImpl& operator=(SubscriberImpl&& rhs) = delete;
    virtual ~SubscriberImpl() noexcept;

    ///
    /// @brief Take the samples from the top of the receive queue.
    /// @return Either a sample or a ChunkReceiveResult.
    /// @details The sample takes care of the cleanup. Don't store the raw pointer to the content of the sample, but
    /// always the whole sample.
    ///
    cxx::expected<Sample<const T, const H>, ChunkReceiveResult> take() noexcept;

    using PortType = typename BaseSubscriber_t::PortType;
    using SubscriberSampleDeleter = SampleDeleter<PortType>;

  protected:
    using BaseSubscriber_t::port;

  private:
    SubscriberSampleDeleter m_sampleDeleter{port()};
};

template <typename T, typename H = mepoo::NoUserHeader>
using Subscriber = SubscriberImpl<T, H>;

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/subscriber.inl"

#endif // IOX_POSH_POPO_TYPED_SUBSCRIBER_HPP
