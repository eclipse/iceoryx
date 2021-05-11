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

#ifndef IOX_EXAMPLES_COMPLEXDATA_TOPIC_DATA_HPP
#define IOX_EXAMPLES_COMPLEXDATA_TOPIC_DATA_HPP

#include "iceoryx_utils/cxx/forward_list.hpp"
#include "iceoryx_utils/cxx/list.hpp"
#include "iceoryx_utils/cxx/optional.hpp"
#include "iceoryx_utils/cxx/stack.hpp"
#include "iceoryx_utils/cxx/string.hpp"
#include "iceoryx_utils/cxx/variant.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

using namespace iox::cxx;

//! [complexdata type]
struct ComplexDataType
{
    forward_list<string<10>, 5> stringForwardList;
    list<uint64_t, 10> integerList;
    list<optional<int32_t>, 15> optionalList;
    stack<float, 5> floatStack;
    string<20> someString;
    vector<double, 5> doubleVector;
    vector<variant<string<10>, double>, 10> variantVector;
};
//! [complexdata type]

#endif // IOX_EXAMPLES_COMPLEXDATA_TOPIC_DATA_HPP

