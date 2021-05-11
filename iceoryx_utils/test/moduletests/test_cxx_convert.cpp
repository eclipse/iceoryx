// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_utils/cxx/convert.hpp"
#include "test.hpp"


#include <cstdint>
namespace
{
using namespace ::testing;

using NumberType = iox::cxx::convert::NumberType;

class convert_test : public Test
{
  public:
    void SetUp()
    {
        internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};


TEST_F(convert_test, toString_Integer)
{
    EXPECT_THAT(iox::cxx::convert::toString(123), Eq("123"));
}

TEST_F(convert_test, toString_Float)
{
    EXPECT_THAT(iox::cxx::convert::toString(12.3f), Eq("12.3"));
}

TEST_F(convert_test, toString_LongLongUnsignedInt)
{
    EXPECT_THAT(iox::cxx::convert::toString(123LLU), Eq("123"));
}

TEST_F(convert_test, toString_Char)
{
    EXPECT_THAT(iox::cxx::convert::toString('x'), Eq("x"));
}

TEST_F(convert_test, toString_String)
{
    EXPECT_THAT(iox::cxx::convert::toString(std::string("hello")), Eq("hello"));
}

TEST_F(convert_test, toString_StringConvertableClass)
{
    struct A
    {
        operator std::string() const
        {
            return "fuu";
        }
    };

    EXPECT_THAT(iox::cxx::convert::toString(A()), Eq("fuu"));
}

TEST_F(convert_test, FromString_String)
{
    std::string source = "hello";
    std::string destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(source, Eq(destination));
}

TEST_F(convert_test, fromString_Char_Success)
{
    std::string source = "h";
    char destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(source[0], Eq(destination));
}

TEST_F(convert_test, fromString_Char_Fail)
{
    std::string source = "hasd";
    char destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, stringIsNumber_IsINTEGER)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("123921301", NumberType::INTEGER), Eq(true));
}

TEST_F(convert_test, stringIsNumber_IsEmpty)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("", NumberType::INTEGER), Eq(false));
}

TEST_F(convert_test, stringIsNumber_IsZero)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("0", NumberType::INTEGER), Eq(true));
}

TEST_F(convert_test, stringIsNumber_INTEGERWithSign)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("-123", NumberType::INTEGER), Eq(true));
}

TEST_F(convert_test, stringIsNumber_INTEGERWithSignPlacedWrongly)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("2-3", NumberType::UNSIGNED_INTEGER), Eq(false));
}

TEST_F(convert_test, stringIsNumber_SimpleFLOAT)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("123.123", NumberType::FLOAT), Eq(true));
}

TEST_F(convert_test, stringIsNumber_MultiDotFLOAT)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("12.3.123", NumberType::FLOAT), Eq(false));
}

TEST_F(convert_test, stringIsNumber_FLOATWithSign)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("+123.123", NumberType::FLOAT), Eq(true));
}

TEST_F(convert_test, stringIsNumber_NumberWithLetters)
{
    EXPECT_THAT(iox::cxx::convert::stringIsNumber("+123a.123", NumberType::FLOAT), Eq(false));
}

TEST_F(convert_test, fromString_FLOAT_Success)
{
    std::string source = "123.01";
    float destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_FLOAT_EQ(destination, 123.01f);
}

TEST_F(convert_test, fromString_FLOAT_Fail)
{
    std::string source = "hasd";
    float destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_Double_Success)
{
    std::string source = "123.04";
    double destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(static_cast<double>(123.04)));
}

TEST_F(convert_test, fromString_Double_Fail)
{
    std::string source = "hasd";
    double destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_LongDouble_Success)
{
    std::string source = "123.01";
    long double destination;
    long double verify = 123.01;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Ge(verify - 0.00001));
    EXPECT_THAT(destination, Le(verify + 0.00001));
}

TEST_F(convert_test, fromString_LongDouble_Fail)
{
    std::string source = "hasd";
    double destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_UNSIGNED_Int_Success)
{
    std::string source = "123";
    unsigned int destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(123u));
}

TEST_F(convert_test, fromString_UNSIGNED_Int_Fail)
{
    std::string source = "-123";
    unsigned int destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_UNSIGNED_LongInt_Success)
{
    std::string source = "123";
    uint64_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(123lu));
}

TEST_F(convert_test, fromString_UNSIGNED_LongInt_Fail)
{
    std::string source = "-a123";
    uint64_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_Int_Success)
{
    std::string source = "123";
    int destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(123));
}

TEST_F(convert_test, fromString_Int_Fail)
{
    std::string source = "-+123";
    int destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_ShortInt_Success)
{
    std::string source = "123";
    short destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(123));
}

TEST_F(convert_test, fromString_ShortInt_Fail)
{
    std::string source = "-+123";
    short destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_Bool_Success)
{
    std::string source = "1";
    bool destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(true));
}

TEST_F(convert_test, fromString_Bool_Fail)
{
    std::string source = "-+123";
    bool destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_UShortInt_Success)
{
    std::string source = "123";
    unsigned short destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(123));
}

TEST_F(convert_test, fromString_UShortInt_Fail)
{
    std::string source = "-+123";
    unsigned short destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_LongInt_Success)
{
    std::string source = "-1123";
    int64_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    EXPECT_THAT(destination, Eq(-1123l));
}

TEST_F(convert_test, fromString_LongInt_Fail)
{
    std::string source = "-a123";
    int64_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_MinMaxShort)
{
    std::string source = "32767";
    std::int16_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "32768";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
    source = "-32768";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "-32769";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_MinMaxUNSIGNED_Short)
{
    std::string source = "65535";
    std::uint16_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "65536";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
    source = "0";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "-1";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_MinMaxInt)
{
    std::string source = "2147483647";
    std::int32_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "2147483648";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
    source = "-2147483648";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "-2147483649";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}

TEST_F(convert_test, fromString_MinMaxUNSIGNED_Int)
{
    std::string source = "4294967295";
    std::uint32_t destination;
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "4294967296";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
    source = "0";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(true));
    source = "-1";
    EXPECT_THAT(iox::cxx::convert::fromString(source.c_str(), destination), Eq(false));
}
} // namespace
