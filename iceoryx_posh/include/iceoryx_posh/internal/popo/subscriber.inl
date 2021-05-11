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

#ifndef IOX_POSH_POPO_TYPED_SUBSCRIBER_INL
#define IOX_POSH_POPO_TYPED_SUBSCRIBER_INL

namespace iox
{
namespace popo
{
template <typename T, typename H, typename BaseSubscriber_t>
inline SubscriberImpl<T, H, BaseSubscriber_t>::SubscriberImpl(const capro::ServiceDescription& service,
                                                              const SubscriberOptions& subscriberOptions)
    : BaseSubscriber_t(service, subscriberOptions)
{
}

template <typename T, typename H, typename BaseSubscriber_t>
inline cxx::expected<Sample<const T, const H>, ChunkReceiveResult>
SubscriberImpl<T, H, BaseSubscriber_t>::take() noexcept
{
    auto result = BaseSubscriber_t::takeChunk();
    if (result.has_error())
    {
        return cxx::error<ChunkReceiveResult>(result.get_error());
    }
    auto userPayloadPtr = static_cast<const T*>(result.value()->userPayload());
    auto samplePtr = cxx::unique_ptr<const T>(userPayloadPtr, m_sampleDeleter);
    return cxx::success<Sample<const T, const H>>(std::move(samplePtr));
}

template <typename T, typename H, typename BaseSubscriber_t>
inline SubscriberImpl<T, H, BaseSubscriber_t>::~SubscriberImpl() noexcept
{
    BaseSubscriber_t::m_trigger.reset();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_TYPED_SUBSCRIBER_INL
