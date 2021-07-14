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

#ifndef IOX_POSH_POPO_SAMPLE_INL
#define IOX_POSH_POPO_SAMPLE_INL

namespace iox
{
namespace popo
{
template <typename T, typename H>
template <typename S, typename>
inline Sample<T, H>::Sample(cxx::unique_ptr<T>&& sampleUniquePtr, PublisherInterface<T, H>& publisher) noexcept
    : SmartChunk<PublisherInterface<T, H>, T, H>(std::move(sampleUniquePtr), publisher)
{
}

template <typename T, typename H>
template <typename S, typename>
inline Sample<T, H>::Sample(cxx::unique_ptr<const T>&& sampleUniquePtr) noexcept
    : SmartChunk<PublisherInterface<T, H>, T, H>(std::move(sampleUniquePtr))
{
}

template <typename T, typename H>
template <typename R, typename>
inline R& Sample<T, H>::getUserHeader() noexcept
{
    return *static_cast<R*>(mepoo::ChunkHeader::fromUserPayload(
                                SmartChunk<PublisherInterface<T, H>, T, H>::m_members.smartchunkUniquePtr.get())
                                ->userHeader());
}

template <typename T, typename H>
template <typename R, typename>
inline const R& Sample<T, H>::getUserHeader() const noexcept
{
    return const_cast<Sample<T, H>*>(this)->getUserHeader();
}

template <typename T, typename H>
template <typename S, typename>
inline void Sample<T, H>::publish() noexcept
{
    if (SmartChunk<PublisherInterface<T, H>, T, H>::m_members.smartchunkUniquePtr)
    {
        SmartChunk<PublisherInterface<T, H>, T, H>::m_members.transmitterRef.get().publish(std::move(*this));
    }
    else
    {
        LogError() << "Tried to publish empty Sample! Might be an already published or moved Sample!";
        errorHandler(Error::kPOSH__PUBLISHING_EMPTY_SAMPLE, nullptr, ErrorLevel::MODERATE);
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_SAMPLE_INL
