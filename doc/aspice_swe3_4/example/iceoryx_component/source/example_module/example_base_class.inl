// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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
#ifndef EXAMPLE_BASE_CLASS_INL_INCLUDED
#define EXAMPLE_BASE_CLASS_INL_INCLUDED


template <typename T>
inline ExampleBaseClass<T>::ExampleBaseClass(const uint32_t input)
    : m_memberVariable(input)
{
    // example code
}

template <typename T>
inline uint32_t ExampleBaseClass<T>::getMemberVariable() const noexcept
{
    return m_memberVariable;
}

template <typename T>
inline uint32_t ExampleBaseClass<T>::simplerMethod() const noexcept
{
    // example code
    return 99U;
}

#endif // EXAMPLE_BASE_CLASS_INL_INCLUDED
