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
#ifndef IOX_UTILS_POSIX_WRAPPER_POSIX_CALL_HPP
#define IOX_UTILS_POSIX_WRAPPER_POSIX_CALL_HPP

#include "iceoryx_utils/cxx/attributes.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/string.hpp"

#include <cstdint>
#include <cstring>

namespace iox
{
namespace posix
{
static constexpr uint32_t POSIX_CALL_ERROR_STRING_SIZE = 128U;
static constexpr uint64_t POSIX_CALL_EINTR_REPETITIONS = 5U;
static constexpr int32_t POSIX_CALL_INVALID_ERRNO = -1;

template <typename ReturnType, typename... FunctionArguments>
class PosixCallBuilder;

/// @brief result of a posix call
template <typename T>
struct PosixCallResult
{
    static const PosixCallResult INVALID_STATE;

    PosixCallResult() noexcept = default;

    /// @brief returns the result of std::strerror(errnum) which acquires a
    ///        human readable error string
    cxx::string<POSIX_CALL_ERROR_STRING_SIZE> getHumanReadableErrnum() const noexcept;

    /// @brief the return value of the posix function call
    T value;

    /// @brief the errno value which was set by the posix function call
    int32_t errnum = POSIX_CALL_INVALID_ERRNO;
};

template <typename T>
PosixCallResult<T> const PosixCallResult<T>::INVALID_STATE{{}, POSIX_CALL_INVALID_ERRNO};

namespace internal
{
template <typename ReturnType, typename... FunctionArguments>
PosixCallBuilder<ReturnType, FunctionArguments...> createPosixCallBuilder(ReturnType (*posixCall)(FunctionArguments...),
                                                                          const char* posixFunctionName,
                                                                          const char* file,
                                                                          const int32_t line,
                                                                          const char* callingFunction) noexcept;

template <typename ReturnType>
struct PosixCallDetails
{
    const char* posixFunctionName = nullptr;
    const char* file = nullptr;
    int32_t line = 0;
    const char* callingFunction = nullptr;
    bool hasSuccess = true;

    PosixCallResult<ReturnType> result;
};
} // namespace internal

/// @brief Calling a posix function with automated error handling. If the posix function returns
///        void you do not need to use posixCall since it cannot fail, (see: man errno).
///        We use a builder pattern to create a design which sets the usage contract so that it
///        cannot be used in the wrong way.
/// @code
///        iox::posix::posixCall(sem_timedwait)(handle, timeout)
///             .successReturnValue(0)
///             .evaluateWithIgnoredErrnos(ETIMEDOUT) // can be a comma separated list of errnos
///             .and_then([](auto & result){
///                 std::cout << result.value << std::endl; // return value of sem_timedwait
///                 std::cout << result.errno << std::endl; // errno which was set by sem_timedwait
///                 std::cout << result.getHumanReadableErrnum() << std::endl; // get string returned by strerror(errno)
///             })
///             .or_else([](auto & result){
///                 std::cout << result.value << std::endl; // return value of sem_timedwait
///                 std::cout << result.errno << std::endl; // errno which was set by sem_timedwait
///                 std::cout << result.getHumanReadableErrnum() << std::endl; // get string returned by strerror(errno)
///             })
///
///        // when your posix call signals failure with one specific return value use
///        // .failureReturnValue(_) instead of .successReturnValue(_)
///
///        // if you do not want to ignore errnos use
///        // .evaluate() instead of .evaluateWithIgnoredErrnos(_)
/// @endcode
#define posixCall(f) internal::createPosixCallBuilder(f, #f, __FILE__, __LINE__, __PRETTY_FUNCTION__)

/// @brief class which is created by the verificator to evaluate the result of a posix call
template <typename ReturnType>
class IOX_NO_DISCARD PosixCallEvaluator
{
  public:
    /// @brief evaluate the result of a posix call and ignore specified errnos
    /// @tparam IgnoredErrnos a list of int32_t variables
    /// @param[in] ignoredErrnos the int32_t values of the errnos which should be ignored
    /// @return returns an expected which contains in both cases a PosixCallResult<ReturnType> with the return value
    /// (.value) and the errno value (.errnum) of the function call
    template <typename... IgnoredErrnos>
    cxx::expected<PosixCallResult<ReturnType>, PosixCallResult<ReturnType>>
    evaluateWithIgnoredErrnos(const IgnoredErrnos... ignoredErrnos) const&& noexcept;

    /// @brief evaluate the result of a posix call
    /// @return returns an expected which contains in both cases a PosixCallResult<ReturnType> with the return value
    /// (.value) and the errno value (.errnum) of the function call
    cxx::expected<PosixCallResult<ReturnType>, PosixCallResult<ReturnType>> evaluate() const&& noexcept;

  private:
    template <typename>
    friend class PosixCallVerificator;

    explicit PosixCallEvaluator(internal::PosixCallDetails<ReturnType>& details) noexcept;

  private:
    internal::PosixCallDetails<ReturnType>& m_details;
};

/// @brief class which verifies the return value of a posix function call
template <typename ReturnType>
class IOX_NO_DISCARD PosixCallVerificator
{
  public:
    /// @brief the posix function call defines success through a single value
    /// @param[in] value the value which defines success
    /// @return the PosixCallEvaluator which evaluates the errno values
    PosixCallEvaluator<ReturnType> successReturnValue(const ReturnType value) && noexcept;

    /// @brief the posix function call defines failure through a single value
    /// @param[in] value the value which defines failure
    /// @return the PosixCallEvaluator which evaluates the errno values
    PosixCallEvaluator<ReturnType> failureReturnValue(const ReturnType value) && noexcept;

  private:
    template <typename, typename...>
    friend class PosixCallBuilder;

    explicit PosixCallVerificator(internal::PosixCallDetails<ReturnType>& details) noexcept;

  private:
    internal::PosixCallDetails<ReturnType>& m_details;
};

template <typename ReturnType, typename... FunctionArguments>
class IOX_NO_DISCARD PosixCallBuilder
{
  public:
    /// @brief input function type
    using FunctionType_t = ReturnType (*)(FunctionArguments...);

    /// @brief Call the underlying function with the provided arguments. If the underlying function fails and sets the
    /// errno to EINTR the call is repeated at most POSIX_CALL_EINTR_REPETITIONS times
    /// @param[in] arguments arguments which will be provided to the posix function
    /// @return the PosixCallVerificator to verify the return value
    PosixCallVerificator<ReturnType> operator()(FunctionArguments... arguments) && noexcept;

  private:
    template <typename ReturnType_, typename... FunctionArguments_>
    friend PosixCallBuilder<ReturnType_, FunctionArguments_...>
    internal::createPosixCallBuilder(ReturnType_ (*posixCall)(FunctionArguments_...),
                                     const char* posixFunctionName,
                                     const char* file,
                                     const int32_t line,
                                     const char* callingFunction) noexcept;

    PosixCallBuilder(FunctionType_t posixCall,
                     const char* posixFunctionName,
                     const char* file,
                     const int32_t line,
                     const char* callingFunction) noexcept;

  private:
    FunctionType_t m_posixCall = nullptr;
    internal::PosixCallDetails<ReturnType> m_details;
};
} // namespace posix
} // namespace iox

#include "iceoryx_utils/internal/posix_wrapper/posix_call.inl"

#endif
