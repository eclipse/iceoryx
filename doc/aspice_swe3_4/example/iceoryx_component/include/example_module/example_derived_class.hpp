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
#ifndef IOX_DOC_EXAMPLE_MODULE_EXAMPLE_DERIVED_CLASS_HPP
#define IOX_DOC_EXAMPLE_MODULE_EXAMPLE_DERIVED_CLASS_HPP

#include "example_module/example_base_class.hpp"


namespace example
{
/// @brief Short description
/// @code
///   minimalistic working example which uses all public methods
/// @endcode
/// @swcomponent example_component
void someFreeFunction();

/// @brief Forward declaration of other class
class SomeOtherClass;

/// @brief Short description
/// @details Detailed description
/// @code
///     ExampleDerivedClass<uint32_t> fuu(1U, 2U);
///     fuu.simpleMethod();
/// @endcode
/// @startuml
/// (*) --> "Init"
/// if "Condition" then
///   -->[true] "Action Processing"
///   --> "Action Errorhandling"
///   -right-> (*)
/// else
///   ->[false] "Do something else"
///   -->[Finish] (*)
/// endif
/// @enduml
/// @note Important note for user/developer
/// @swcomponent cpp
template <typename T>
class ExampleDerivedClass : public ExampleBaseClass<T>
{
  public:
    /// @brief Short description
    /// @details Detailed description
    /// @param[in] a Description of input parameter a
    /// @param[in] b Description of input parameter b
    ExampleDerivedClass(const uint64_t a, const uint64_t b) noexcept;

    /// @brief Short description
    void simpleMethod() const noexcept;

    /// @brief Short description
    /// @param[in] c Description of input parameter c
    /// @param[out] d Description of output parameter d
    /// @return Description of return value
    uint64_t complexMethod(const uint32_t c, const uint32_t* d) noexcept;

    /// @brief A good example method which sets some kind of speed
    /// @param[in] speed Description of input parameter speed
    /// @code
    ///     myClass.goodExampleMethod(200_kmh); // sets it to 200 km/h
    ///     myClass.goodExampleMethod(40_ms);   // sets it to 40 m/s
    /// @endcode
    void goodExampleMethod(const speed_t speed) noexcept;

    /// @brief Short description
    /// @pre 	Must be called before another method is called
    /// @post 	Cannot be called twice, once it is called everything is done
    /// @param[in] fuu some clarification, min and max is also defined here
    ///            and not specified with a custom tag. 0 <=
    ///            fuu <= 1000
    /// @deprecated will be removed when feature iox-#123 is implemented
    void preInitStuff(const uint32_t fuu) noexcept;
};

} // namespace example

#endif // IOX_DOC_EXAMPLE_MODULE_EXAMPLE_DERIVED_CLASS_HPP
