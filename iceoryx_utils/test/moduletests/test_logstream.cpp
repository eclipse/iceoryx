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

#include "iceoryx_utils/log/logging.hpp"
#include "iceoryx_utils/log/logstream.hpp"
#include "iceoryx_utils/testing/mocks/logger_mock.hpp"
#include "iceoryx_utils/testing/test.hpp"

#include <cstdint>
#include <limits>

namespace
{
using namespace ::testing;

class IoxLogStream_test : public Test
{
  public:
    Logger_Mock loggerMock;
};

TEST_F(IoxLogStream_test, CTor_Default)
{
    iox::log::LogStream(loggerMock) << "";

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(""));
    EXPECT_THAT(loggerMock.m_logs[0].level, Eq(iox::log::LogLevel::kWarn));
}

TEST_F(IoxLogStream_test, CTor_WithLogLevel)
{
    iox::log::LogStream(loggerMock, iox::log::LogLevel::kOff) << "";

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(""));
    EXPECT_THAT(loggerMock.m_logs[0].level, Eq(iox::log::LogLevel::kOff));
}

TEST_F(IoxLogStream_test, UnnamedTemporaryLogStreamObject)
{
    const std::string claim = "The answer is ";
    const uint8_t answer = 42;
    const std::string bang = "!";

    iox::log::LogStream(loggerMock) << claim << answer << bang;

    std::string expected = claim + std::to_string(answer) + bang;

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(expected));
}

TEST_F(IoxLogStream_test, LocalLogStreamObject)
{
    const std::string claim = "The answer is ";
    const uint8_t answer = 42;
    const std::string bang = "!";

    {
        auto log = iox::log::LogStream(loggerMock);
        log << claim;
        log << answer;
        log << bang;

        // the destructor flushes the log to the logger
        EXPECT_THAT(loggerMock.m_logs.size(), Eq(0u));
    }

    std::string expected = claim + std::to_string(answer) + bang;

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(expected));
}

TEST_F(IoxLogStream_test, ExplicitFlush)
{
    const std::string claim = "The answer is ";
    const uint8_t answer = 42;
    const std::string bang = "!";

    {
        auto log = iox::log::LogStream(loggerMock);
        log << claim;
        log.Flush();
        log << answer;
        log.Flush();
        log << bang;

        // the destructor flushes remaining logs to the logger
        EXPECT_THAT(loggerMock.m_logs.size(), Eq(2u));
    }

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(3u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(claim));
    EXPECT_THAT(loggerMock.m_logs[1].message, Eq(std::to_string(answer)));
    EXPECT_THAT(loggerMock.m_logs[2].message, Eq(bang));
}

TEST_F(IoxLogStream_test, NoFlushWhenAlreadyFlushed)
{
    {
        auto log = iox::log::LogStream(loggerMock);
        log << "fubar";
        log.Flush();

        EXPECT_THAT(loggerMock.m_logs.size(), Eq(1u));
    }

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
}

TEST_F(IoxLogStream_test, StreamOperatorCharArray)
{
    char logValue[]{"This is the iceoryx logger!"};
    const char constLogValue[]{"Nothing to see here, move along!"};
    iox::log::LogStream(loggerMock) << logValue;
    iox::log::LogStream(loggerMock) << constLogValue;

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(2u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(std::string(logValue)));
    EXPECT_THAT(loggerMock.m_logs[1].message, Eq(std::string(constLogValue)));
}

TEST_F(IoxLogStream_test, StreamOperatorStdString)
{
    std::string logValue{"This is the iceoryx logger!"};
    const std::string constLogValue{"Nothing to see here, move along!"};
    iox::log::LogStream(loggerMock) << logValue;
    iox::log::LogStream(loggerMock) << constLogValue;

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(2u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(logValue));
    EXPECT_THAT(loggerMock.m_logs[1].message, Eq(constLogValue));
}

TEST_F(IoxLogStream_test, StreamOperatorLogLevel)
{
    std::string logValue{"This is the iceoryx logger!"};
    iox::log::LogLevel logLevel = iox::log::LogLevel::kWarn;
    iox::log::LogStream(loggerMock) << logValue << logLevel;

    EXPECT_THAT(loggerMock.m_logs[0].message, Eq("This is the iceoryx logger!Warn"));
}

TEST_F(IoxLogStream_test, StreamOperatorLogRawBuffer)
{
    struct DummyStruct
    {
        uint16_t a{0xAFFE};
        uint16_t b{0xDEAD};
        uint32_t c{0xC0FFEE};
    };

    DummyStruct d;

    iox::log::LogStream(loggerMock) << iox::log::RawBuffer(d);
    volatile uint16_t endianess{0x0100};
    auto bigEndian = reinterpret_cast<volatile uint8_t*>(&endianess);
    if (*bigEndian)
    {
        EXPECT_THAT(loggerMock.m_logs[0].message, Eq("0x[af fe de ad 00 c0 ff ee]"));
    }
    else
    {
        EXPECT_THAT(loggerMock.m_logs[0].message, Eq("0x[fe af ad de ee ff c0 00]"));
    }
}

template <class T>
class IoxLogStreamHexBin_test : public IoxLogStream_test
{
  public:
    T LogValueLow = std::numeric_limits<T>::lowest();
    T LogValueMin = std::numeric_limits<T>::min();
    T LogValueMax = std::numeric_limits<T>::max();
};

using LogHexBinTypes = Types<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(IoxLogStreamHexBin_test, LogHexBinTypes);
#pragma GCC diagnostic pop

template <typename LogType>
void testStreamOperatorLogHex(Logger_Mock& loggerMock, LogType logValue)
{
    iox::log::LogStream(loggerMock) << iox::log::HexFormat(logValue);

    // we need to check negative numbers in two's complement, therfore make the output value unsigned
    using TestType = typename std::make_unsigned<LogType>::type;
    auto outputValue = static_cast<TestType>(logValue);

    std::stringstream ss;
    ss << "0x" << std::hex << +outputValue; // the '+' is to prevent to interpret the (u)int8_t as char

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(loggerMock.m_logs[0].message, Eq(ss.str()));
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogHex_ValueLow)
{
    testStreamOperatorLogHex(this->loggerMock, this->LogValueLow);
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogHex_ValueMin)
{
    testStreamOperatorLogHex(this->loggerMock, this->LogValueMin);
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogHex_ValueMax)
{
    testStreamOperatorLogHex(this->loggerMock, this->LogValueMax);
}

template <typename LogType>
void testStreamOperatorLogBin(Logger_Mock& loggerMock, LogType logValue)
{
    iox::log::LogStream(loggerMock) << iox::log::BinFormat(logValue);

    // we need to check negative numbers in two's complement, therfore make the output value unsigned
    using TestType = typename std::make_unsigned<LogType>::type;
    auto outputValue = static_cast<TestType>(logValue);

    ASSERT_THAT(loggerMock.m_logs.size(), Eq(1u));

    EXPECT_THAT(loggerMock.m_logs[0].message,
                Eq("0b" + std::bitset<std::numeric_limits<TestType>::digits>(outputValue).to_string()));
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogBin_ValueLow)
{
    testStreamOperatorLogBin(this->loggerMock, this->LogValueLow);
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogBin_ValueMin)
{
    testStreamOperatorLogBin(this->loggerMock, this->LogValueMin);
}

TYPED_TEST(IoxLogStreamHexBin_test, StreamOperatorLogBin_ValueMax)
{
    testStreamOperatorLogBin(this->loggerMock, this->LogValueMax);
}

using ArithmeticTypes =
    Types<bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, size_t, float, double>;

/// we require TYPED_TEST since we support gtest 1.8 for our safety targets
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST_CASE(IoxLogStreamArithmetic_test, ArithmeticTypes);
#pragma GCC diagnostic pop

template <typename Arithmetic>
class IoxLogStreamArithmetic_test : public IoxLogStream_test
{
  public:
    Arithmetic LogValueLow = std::numeric_limits<Arithmetic>::lowest();
    Arithmetic LogValueMin = std::numeric_limits<Arithmetic>::min();
    Arithmetic LogValueMax = std::numeric_limits<Arithmetic>::max();

    const Arithmetic ConstLogValueLow = std::numeric_limits<Arithmetic>::lowest();
    const Arithmetic ConstLogValueMin = std::numeric_limits<Arithmetic>::min();
    const Arithmetic ConstLogValueMax = std::numeric_limits<Arithmetic>::max();

    static constexpr Arithmetic ConstexprLogValueLow = std::numeric_limits<Arithmetic>::lowest();
    static constexpr Arithmetic ConstexprLogValueMin = std::numeric_limits<Arithmetic>::min();
    static constexpr Arithmetic ConstexprLogValueMax = std::numeric_limits<Arithmetic>::max();
};

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ValueLow)
{
    iox::log::LogStream(this->loggerMock) << this->LogValueLow;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->LogValueLow)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ValueMin)
{
    iox::log::LogStream(this->loggerMock) << this->LogValueMin;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->LogValueMin)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ValueMax)
{
    iox::log::LogStream(this->loggerMock) << this->LogValueMax;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->LogValueMax)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstValueLow)
{
    iox::log::LogStream(this->loggerMock) << this->ConstLogValueLow;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstLogValueLow)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstValueMin)
{
    iox::log::LogStream(this->loggerMock) << this->ConstLogValueMin;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstLogValueMin)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstValueMax)
{
    iox::log::LogStream(this->loggerMock) << this->ConstLogValueMax;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstLogValueMax)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstexprValueLow)
{
    iox::log::LogStream(this->loggerMock) << this->ConstexprLogValueLow;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstexprLogValueLow)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstexprValueMin)
{
    iox::log::LogStream(this->loggerMock) << this->ConstexprLogValueMin;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstexprLogValueMin)));
}

TYPED_TEST(IoxLogStreamArithmetic_test, StreamOperator_ConstexprValueMax)
{
    iox::log::LogStream(this->loggerMock) << this->ConstexprLogValueMax;

    ASSERT_THAT(this->loggerMock.m_logs.size(), Eq(1u));
    EXPECT_THAT(this->loggerMock.m_logs[0].message, Eq(std::to_string(this->ConstexprLogValueMax)));
}
} // namespace
