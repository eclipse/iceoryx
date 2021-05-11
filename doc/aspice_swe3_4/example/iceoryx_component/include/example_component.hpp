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
#ifndef IOX_DOC_EXAMPLE_COMPONENT_EXAMPLE_COMPONENT_HPP
#define IOX_DOC_EXAMPLE_COMPONENT_EXAMPLE_COMPONENT_HPP


#include "example_base_class.hpp"
#include "example_module/example_derived_class.hpp"

/// @brief This is the Example Component Description
/// @details This is the detailed Example Component Description
/// @code
///     MySampleClass<T> m_sample;
/// @endcode
/// Link to generic Requirements, which are fulfilled by this component
/// @req IOX_SWRS_200
/// https://foo-doors-bla.de.example.com:1234/dwa/rm/urn:rational::1-O-112-00000d86?doors.view=00000003
/// @link <a
/// href="ea://eap?c=Jwr%2ff4Cq7ZYIhPQMSQZVZ4">architecture diagram
/// link</a>
/// @link Some Diagram <a href="http://link.com/to/some/documentation.html">Documentation</a>
namespace example
{
template <typename T>
class MySampleClass;

/// @brief link only for requirements which are on class level
/// @req IOX_SWRS_112
/// https://foo-doors-bla.de.example.com:1234/dwa/rm/urn:rational::1-O-112-00000d86?doors.view=00000003
template <typename T>
class ExampleBaseClass;

void someOtherFreeFunction();

} // namespace example


#endif // IOX_DOC_EXAMPLE_COMPONENT_EXAMPLE_COMPONENT_HPP
