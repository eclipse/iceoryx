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

#ifndef IOX_POSH_POPO_UNTYPED_SUBSCRIBER_INL
#define IOX_POSH_POPO_UNTYPED_SUBSCRIBER_INL

namespace iox
{
namespace popo
{
template <typename BaseSubscriber_t>
inline UntypedSubscriberImpl<BaseSubscriber_t>::UntypedSubscriberImpl(const capro::ServiceDescription& service,
                                                                      const SubscriberOptions& subscriberOptions)
    : BaseSubscriber(service, subscriberOptions)
{
}

template <typename BaseSubscriber_t>
inline cxx::expected<const void*, ChunkReceiveResult> UntypedSubscriberImpl<BaseSubscriber_t>::take() noexcept
{
    auto result = BaseSubscriber::takeChunk();
    if (result.has_error())
    {
        return cxx::error<ChunkReceiveResult>(result.get_error());
    }
    return cxx::success<const void*>(result.value()->userPayload());
}

template <typename BaseSubscriber_t>
inline void UntypedSubscriberImpl<BaseSubscriber_t>::release(const void* const userPayload) noexcept
{
    auto chunkHeader = mepoo::ChunkHeader::fromUserPayload(userPayload);
    port().releaseChunk(chunkHeader);
}

template <typename BaseSubscriber_t>
inline UntypedSubscriberImpl<BaseSubscriber_t>::~UntypedSubscriberImpl() noexcept
{
    BaseSubscriber_t::m_trigger.reset();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_UNTYPED_SUBSCRIBER_INL
