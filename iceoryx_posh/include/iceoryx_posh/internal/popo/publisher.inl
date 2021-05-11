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

#ifndef IOX_POSH_POPO_TYPED_PUBLISHER_INL
#define IOX_POSH_POPO_TYPED_PUBLISHER_INL

#include <cstdint>

namespace iox
{
namespace popo
{
template <typename T, typename H, typename BasePublisher_t>
inline PublisherImpl<T, H, BasePublisher_t>::PublisherImpl(const capro::ServiceDescription& service,
                                                           const PublisherOptions& publisherOptions)
    : BasePublisher_t(service, publisherOptions)
{
}

template <typename T, typename H, typename BasePublisher_t>
template <typename... Args>
inline cxx::expected<Sample<T, H>, AllocationError> PublisherImpl<T, H, BasePublisher_t>::loan(Args&&... args) noexcept
{
    return std::move(loanSample().and_then([&](auto& sample) { new (sample.get()) T(std::forward<Args>(args)...); }));
}

template <typename T, typename H, typename BasePublisher_t>
template <typename Callable, typename... ArgTypes>
inline cxx::expected<AllocationError> PublisherImpl<T, H, BasePublisher_t>::publishResultOf(Callable c,
                                                                                            ArgTypes... args) noexcept
{
    static_assert(cxx::is_invocable<Callable, T*, ArgTypes...>::value,
                  "Publisher<T>::publishResultOf expects a valid callable with a specific signature as the "
                  "first argument");
    static_assert(cxx::has_signature<Callable, void(T*, ArgTypes...)>::value,
                  "callable provided to Publisher<T>::publishResultOf must have signature void(T*, ArgsTypes...)");

    return loanSample().and_then([&](auto& sample) {
        c(sample.get(), std::forward<ArgTypes>(args)...);
        sample.publish();
    });
}

template <typename T, typename H, typename BasePublisher_t>
inline cxx::expected<AllocationError> PublisherImpl<T, H, BasePublisher_t>::publishCopyOf(const T& val) noexcept
{
    return loanSample().and_then([&](auto& sample) {
        *sample.get() = val; // Copy assignment of value into sample's memory allocation.
        sample.publish();
    });
}

template <typename T, typename H, typename BasePublisher_t>
inline cxx::expected<Sample<T, H>, AllocationError> PublisherImpl<T, H, BasePublisher_t>::loanSample() noexcept
{
    static constexpr uint32_t USER_HEADER_SIZE{std::is_same<H, mepoo::NoUserHeader>::value ? 0U : sizeof(H)};

    auto result = port().tryAllocateChunk(sizeof(T), alignof(T), USER_HEADER_SIZE, alignof(H));
    if (result.has_error())
    {
        return cxx::error<AllocationError>(result.get_error());
    }
    else
    {
        return cxx::success<Sample<T, H>>(convertChunkHeaderToSample(result.value()));
    }
}

template <typename T, typename H, typename BasePublisher_t>
inline void PublisherImpl<T, H, BasePublisher_t>::publish(Sample<T, H>&& sample) noexcept
{
    auto userPayload = sample.release(); // release the Samples ownership of the chunk before publishing
    auto chunkHeader = mepoo::ChunkHeader::fromUserPayload(userPayload);
    port().sendChunk(chunkHeader);
}

template <typename T, typename H, typename BasePublisher_t>
inline Sample<T, H>
PublisherImpl<T, H, BasePublisher_t>::convertChunkHeaderToSample(mepoo::ChunkHeader* const header) noexcept
{
    return Sample<T, H>(cxx::unique_ptr<T>(reinterpret_cast<T*>(header->userPayload()), m_sampleDeleter), *this);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_TYPED_PUBLISHER_INL
