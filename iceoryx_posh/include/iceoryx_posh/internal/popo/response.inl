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

#ifndef IOX_POSH_POPO_RESPONSE_INL
#define IOX_POSH_POPO_RESPONSE_INL

namespace iox
{
namespace popo
{
template <typename T, typename H>
template <typename S, typename>
inline Response<T, H>::Response(cxx::unique_ptr<T>&& responseUniquePtr, RpcInterface<T, H>& producer) noexcept
    : SmartChunk<RpcInterface<T, H>, T, H>(std::move(responseUniquePtr), producer)
{
}

template <typename T, typename H>
template <typename S, typename>
inline Response<T, H>::Response(cxx::unique_ptr<const T>&& responseUniquePtr) noexcept
    : SmartChunk<RpcInterface<T, H>, T, H>(std::move(responseUniquePtr))
{
}

template <typename T, typename H>
template <typename R, typename>
inline R& Response<T, H>::getResponseHeader() noexcept
{
    return *static_cast<R*>(mepoo::ChunkHeader::fromUserPayload(m_members.smartchunkUniquePtr.get())->userHeader());
}

template <typename T, typename H>
template <typename R, typename>
inline const R& Response<T, H>::getResponseHeader() const noexcept
{
    return const_cast<Response<T, H>*>(this)->getResponseHeader();
}

template <typename T, typename H>
template <typename S, typename>
inline void Response<T, H>::send() noexcept
{
    if (m_members.smartchunkUniquePtr)
    {
        m_members.producerRef.get().sendResponse(std::move(*this));
    }
    else
    {
        LogError() << "Tried to send empty Response! Might be an already sent or moved Response!";
        errorHandler(Error::kPOSH__PUBLISHING_EMPTY_RESPONSE, nullptr, ErrorLevel::MODERATE);
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_RESPONSE_INL
