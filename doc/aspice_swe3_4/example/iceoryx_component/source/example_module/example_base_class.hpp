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
#ifndef IOX_DOC_EXAMPLE_MODULE_EXAMPLE_BASE_CLASS_HPP
#define IOX_DOC_EXAMPLE_MODULE_EXAMPLE_BASE_CLASS_HPP

#include <stdint.h>

namespace example
{
/// @brief Short description
/// @code
///   minimalistic working example which uses all public methods
/// @endcode
/// @swcomponent example_component
void someOtherFreeFunction() noexcept;

/// @brief Base class which is not part of the public API
/// @details Detailed description
/// @startuml
/// 	Alice -> Bob: Authentication Request
/// 	Bob --> Alice: Authentication Response
///
/// 	Alice -> Bob: Another authentication Request
/// 	Alice <-- Bob: another authentication Response
/// @enduml
/// @swcomponent example_component
template <typename T>
class ExampleBaseClass
{
  public:
    /// @brief Short description
    /// @details Detailed description
    /// @param[in] input Description of input parameter
    ExampleBaseClass(const uint32_t input);

    /// @brief Short description
    ExampleBaseClass() = default;

    /// @brief Short description
    uint32_t getMemberVariable() const noexcept;

    /// @brief Short description
    uint32_t simplerMethod() const noexcept;

    private:
    /// @brief Short description
    uint32_t m_memberVariable{0U};
};

#include "example_base_class.inl"

} // namespace example


#endif // IOX_DOC_EXAMPLE_MODULE_EXAMPLE_BASE_CLASS_HPP
