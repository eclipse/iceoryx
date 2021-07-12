// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_HOOFS_ERROR_HANDLING_ERROR_HANDLING_HPP
#define IOX_HOOFS_ERROR_HANDLING_ERROR_HANDLING_HPP

#include "iceoryx_hoofs/cxx/generic_raii.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"

#include <assert.h>
#include <functional>
#include <iostream>
#include <mutex>

namespace iox
{
// clang-format off
#define ICEORYX_ERRORS(error) \
    error(NO_ERROR)\
    error(FILEREADER__FAILED_TO_OPEN_FILE) \
    error(POSH__ROUDI_PROCESS_SHUTDOWN_FAILED) \
    error(POSH__ROUDI_PROCESS_SEND_VIA_IPC_CHANNEL_FAILED)\
    error(POSH__RUNTIME_FACTORY_IS_NOT_SET) \
    error(POSH__RUNTIME_IS_CREATED_MULTIPLE_TIMES) \
    error(POSH__RUNTIME_PUBLISHER_PORT_NOT_UNIQUE) \
    error(POSH__RUNTIME_PUBLISHER_PORT_CREATION_UNKNOWN_ERROR) \
    error(POSH__RUNTIME_SUBSCRIBER_PORT_CREATION_UNKNOWN_ERROR) \
    error(POSH__RUNTIME_ROUDI_PUBLISHER_LIST_FULL) \
    error(POSH__RUNTIME_ROUDI_SUBSCRIBER_LIST_FULL) \
    error(POSH__RUNTIME_ROUDI_CONDITION_VARIABLE_LIST_FULL) \
    error(POSH__RUNTIME_ROUDI_EVENT_VARIABLE_LIST_FULL) \
    error(POSH__RUNTIME_ROUDI_REQUEST_PUBLISHER_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_REQUEST_SUBSCRIBER_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_REQUEST_CONDITION_VARIABLE_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_REQUEST_EVENT_VARIABLE_WRONG_MESSAGE_QUEUE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_GET_MW_INTERFACE_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_CREATE_NODE_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_GET_MW_APPLICATION_WRONG_IPC_MESSAGE_RESPONSE) \
    error(POSH__RUNTIME_ROUDI_CONDITION_VARIABLE_CREATION_UNKNOWN_ERROR) \
    error(POSH__RUNTIME_APP_WITH_SAME_RUNTIME_NAME_STILL_RUNNING) \
    error(POSH__RUNTIME_NO_WRITABLE_SHM_SEGMENT) \
    error(POSH__RUNTIME_NO_NAME_PROVIDED) \
    error(POSH__RUNTIME_NAME_EMPTY) \
    error(POSH__RUNTIME_LEADING_SLASH_PROVIDED) \
    error(POSH__PORT_MANAGER_PUBLISHERPORT_NOT_UNIQUE) \
    error(POSH__MEMPOOL_POSSIBLE_DOUBLE_FREE) \
    error(POSH__RECEIVERPORT_DELIVERYFIFO_OVERFLOW) \
    error(POSH__SENDERPORT_SAMPLE_SIZE_CHANGED_FOR_ACTIVE_PORT) \
    error(POSH__SENDERPORT_ACTIVATE_FIELD_WITHOUT_DATA) \
    error(POSH__SENDERPORT_FIELD_SUBSCRIBE_WITHOUT_DATA) \
    error(POSH__SENDERPORT_ACTIVATE_FIELD_WHEN_LATCHED_TOPIC_ACTIVATED) \
    error(POSH__SENDERPORT_ALLOCATE_FAILED) \
    error(POSH__SENDERPORT_SUBSCRIBER_LIST_OVERFLOW) \
    error(POSH__PUBLISHING_EMPTY_SAMPLE) \
    error(POSH__SHM_APP_BASEADDRESS_VIOLATES_SPECIFICATION) \
    error(POSH__SHM_APP_SEGMENT_BASEADDRESS_VIOLATES_SPECIFICATION) \
    error(POSH__SHM_APP_MAPP_ERR) \
    error(POSH__SHM_APP_SEGMENT_MAPP_ERR) \
    error(POSH__SHM_APP_SEGMENT_COUNT_OVERFLOW) \
    error(POSH__INTERFACEPORT_CAPRO_MESSAGE_DISMISSED) \
    error(POSH__SERVICE_DISCOVERY_INSTANCE_CONTAINER_OVERFLOW) \
    error(POSH__SERVICE_DISCOVERY_FIND_SERVICE_CALLBACKS_CONTAINER_OVERFLOW) \
    error(POPO__APPLICATION_PORT_QUEUE_OVERFLOW) \
    error(POPO__BASE_SUBSCRIBER_OVERRIDING_WITH_EVENT_SINCE_HAS_DATA_OR_DATA_RECEIVED_ALREADY_ATTACHED) \
    error(POPO__BASE_SUBSCRIBER_OVERRIDING_WITH_STATE_SINCE_HAS_DATA_OR_DATA_RECEIVED_ALREADY_ATTACHED) \
    error(POPO__CHUNK_QUEUE_POPPER_CHUNK_WITH_INCOMPATIBLE_CHUNK_HEADER_VERSION) \
    error(POPO__CHUNK_DISTRIBUTOR_OVERFLOW_OF_QUEUE_CONTAINER) \
    error(POPO__CHUNK_DISTRIBUTOR_CLEANUP_DEADLOCK_BECAUSE_BAD_APPLICATION_TERMINATION) \
    error(POPO__CHUNK_SENDER_INVALID_CHUNK_TO_FREE_FROM_USER) \
    error(POPO__CHUNK_SENDER_INVALID_CHUNK_TO_SEND_FROM_USER) \
    error(POPO__CHUNK_RECEIVER_INVALID_CHUNK_TO_RELEASE_FROM_USER) \
    error(POPO__CHUNK_LOCKING_ERROR) \
    error(POPO__CHUNK_UNLOCKING_ERROR) \
    error(POPO__CAPRO_PROTOCOL_ERROR) \
    error(POPO__CLIENT_PORT_INVALID_REQUEST_TO_FREE_FROM_USER) \
    error(POPO__CLIENT_PORT_INVALID_REQUEST_TO_SEND_FROM_USER) \
    error(POPO__CLIENT_PORT_INVALID_RESPONSE_TO_RELEASE_FROM_USER) \
    error(POPO__CONDITION_VARIABLE_DATA_FAILED_TO_CREATE_SEMAPHORE) \
    error(POPO__CONDITION_LISTENER_SEMAPHORE_CORRUPTED_IN_WAS_TRIGGERED) \
    error(POPO__CONDITION_LISTENER_SEMAPHORE_CORRUPTED_IN_WAIT) \
    error(POPO__CONDITION_LISTENER_SEMAPHORE_CORRUPTED_IN_TIMED_WAIT) \
    error(POPO__CONDITION_LISTENER_SEMAPHORE_CORRUPTED_IN_RESET) \
    error(POPO__CONDITION_LISTENER_SEMAPHORE_CORRUPTED_IN_DESTROY) \
    error(POPO__CONDITION_NOTIFIER_INDEX_TOO_LARGE) \
    error(POPO__CONDITION_NOTIFIER_SEMAPHORE_CORRUPT_IN_NOTIFY) \
    error(POPO__NOTIFICATION_INFO_TYPE_INCONSISTENCY_IN_GET_ORIGIN) \
    error(POPO__TRIGGER_INVALID_RESET_CALLBACK) \
    error(POPO__TRIGGER_INVALID_HAS_TRIGGERED_CALLBACK) \
    error(POPO__TRIGGER_HANDLE_INVALID_RESET_CALLBACK) \
    error(POPO__TYPED_UNIQUE_ID_ROUDI_HAS_NO_DEFINED_UNIQUE_ID) \
    error(POPO__TYPED_UNIQUE_ID_ROUDI_HAS_ALREADY_DEFINED_UNIQUE_ID) \
    error(POPO__TYPED_UNIQUE_ID_OVERFLOW) \
    error(POPO__WAITSET_COULD_NOT_DETACH_CONDITION) \
    error(MEPOO__MEMPOOL_CONFIG_MUST_BE_ORDERED_BY_INCREASING_SIZE) \
    error(MEPOO__MEMPOOL_GETCHUNK_CHUNK_WITHOUT_MEMPOOL) \
    error(MEPOO__MEMPOOL_GETCHUNK_CHUNK_IS_TOO_LARGE) \
    error(MEPOO__MEMPOOL_GETCHUNK_POOL_IS_RUNNING_OUT_OF_CHUNKS) \
    error(MEPOO__MEMPOOL_CHUNKSIZE_MUST_BE_MULTIPLE_OF_CHUNK_MEMORY_ALIGNMENT) \
    error(MEPOO__MEMPOOL_ADDMEMPOOL_AFTER_GENERATECHUNKMANAGEMENTPOOL) \
    error(MEPOO__TYPED_MEMPOOL_HAS_INCONSISTENT_STATE) \
    error(MEPOO__TYPED_MEMPOOL_MANAGEMENT_SEGMENT_IS_BROKEN) \
    error(MEPOO__USER_WITH_MORE_THAN_ONE_WRITE_SEGMENT) \
    error(MEPOO__SEGMENT_COULD_NOT_APPLY_POSIX_RIGHTS_TO_SHARED_MEMORY) \
    error(MEPOO__SEGMENT_UNABLE_TO_CREATE_SHARED_MEMORY_OBJECT) \
    error(MEPOO__INTROSPECTION_CONTAINER_FULL) \
    error(MEPOO__CANNOT_ALLOCATE_CHUNK) \
    error(MEPOO__MAXIMUM_NUMBER_OF_MEMPOOLS_REACHED) \
    error(PORT_POOL__PUBLISHERLIST_OVERFLOW) \
    error(PORT_POOL__SUBSCRIBERLIST_OVERFLOW) \
    error(PORT_POOL__INTERFACELIST_OVERFLOW) \
    error(PORT_POOL__APPLICATIONLIST_OVERFLOW) \
    error(PORT_POOL__NODELIST_OVERFLOW) \
    error(PORT_POOL__CONDITION_VARIABLE_LIST_OVERFLOW) \
    error(PORT_POOL__EVENT_VARIABLE_LIST_OVERFLOW) \
    error(PORT_MANAGER__PORT_POOL_UNAVAILABLE) \
    error(PORT_MANAGER__INTROSPECTION_MEMORY_MANAGER_UNAVAILABLE) \
    error(PORT_MANAGER__HANDLE_PUBLISHER_PORTS_INVALID_CAPRO_MESSAGE) \
    error(PORT_MANAGER__HANDLE_SUBSCRIBER_PORTS_INVALID_CAPRO_MESSAGE) \
    error(PORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONPORTSERVICE) \
    error(PORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONPORTTHROUGHPUTSERVICE) \
    error(PORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTIONCHANGINGDATASERVICE) \
    error(PORT_MANAGER__NO_PUBLISHER_PORT_FOR_INTROSPECTION_SENDER_PORT) \
    error(ROUDI_COMPONENTS__SHARED_MEMORY_UNAVAILABLE) \
    error(ROUDI_APP__FAILED_TO_CREATE_SEMAPHORE) \
    error(ROUDI_APP__FAILED_TO_UNLOCK_SEMAPHORE_IN_SIG_HANDLER) \
    error(ROUDI__DEFAULT_ROUDI_MEMORY_FAILED_TO_ADD_SEGMENT_MANAGER_MEMORY_BLOCK) \
    error(ROUDI__DEFAULT_ROUDI_MEMORY_FAILED_TO_ADD_INTROSPECTION_MEMORY_BLOCK) \
    error(ROUDI__PRECONDITIONS_FOR_PROCESS_MANAGER_NOT_FULFILLED) \
    error(ICEORYX_ROUDI_MEMORY_MANAGER__COULD_NOT_ACQUIRE_FILE_LOCK) \
    error(ICEORYX_ROUDI_MEMORY_MANAGER__ROUDI_STILL_RUNNING) \
    error(ICEORYX_ROUDI_MEMORY_MANAGER__FAILED_TO_ADD_PORTPOOL_MEMORY_BLOCK) \
    error(ICEORYX_ROUDI_MEMORY_MANAGER__FAILED_TO_ADD_MANAGEMENT_MEMORY_BLOCK) \
    error(MQ_UNKNOWN_MSG) \
    error(MQ_INVALID_MSG) \
    error(IPC_INTERFACE__UNABLE_TO_CREATE_APPLICATION_CHANNEL) \
    error(IPC_INTERFACE__REG_ROUDI_NOT_AVAILABLE) \
    error(IPC_INTERFACE__REG_UNABLE_TO_WRITE_TO_ROUDI_CHANNEL) \
    error(IPC_INTERFACE__REG_ACK_INVALIG_NUMBER_OF_PARAMS) \
    error(IPC_INTERFACE__REG_ACK_NO_RESPONSE) \
    error(IPC_INTERFACE__CHECK_MQ_MAPS_TO_FILE) \
    error(IPC_INTERFACE__APP_WITH_SAME_NAME_STILL_RUNNING) \
    error(IPC_INTERFACE__COULD_NOT_ACQUIRE_FILE_LOCK) \
    error(POSIX_WRAPPER__FAILED_TO_CREATE_SEMAPHORE) \
    error(POSIX_TIMER__FIRED_TIMER_BUT_STATE_IS_INVALID) \
    error(POSIX_TIMER__TIMERPOOL_OVERFLOW) \
    error(POSIX_TIMER__INCONSISTENT_STATE) \
    error(POSIX_TIMER__CALLBACK_RUNTIME_EXCEEDS_RETRIGGER_TIME) \
    error(BINDING_C__UNDEFINED_STATE_IN_IOX_QUEUE_FULL_POLICY) \
    error(BINDING_C__UNDEFINED_STATE_IN_IOX_SUBSCRIBER_TOO_SLOW_POLICY) \
    error(BINDING_C__PUBLISHER_OPTIONS_NOT_INITIALIZED) \
    error(BINDING_C__SUBSCRIBER_OPTIONS_NOT_INITIALIZED) \
    error(BINDING_C__C2CPP_ENUM_TRANSLATION_INVALID_SUBSCRIBER_EVENT_VALUE) \
    error(BINDING_C__C2CPP_ENUM_TRANSLATION_INVALID_SUBSCRIBER_STATE_VALUE)

// clang-format on

// DO NOT TOUCH START, you can doodle around with the lines above!!!
#define CREATE_ICEORYX_ERROR_ENUM(name) k##name,
#define CREATE_ICEORYX_ERROR_STRING(name) #name,

enum class Error : uint32_t
{
    ICEORYX_ERRORS(CREATE_ICEORYX_ERROR_ENUM)
};

/// @brief Convenience stream operator to easily use the Error enum with std::ostream
/// @param[in] stream sink to write the message to
/// @param[in] value to convert to a string literal
/// @return the reference to `stream` which was provided as input parameter
std::ostream& operator<<(std::ostream& stream, Error value);

/// @brief the available error levels
/// FATAL
/// - Log message with FATAL
/// - RouDi cannot recover from that error. RouDi is terminated
/// - Assert (in DEBUG) and terminate
/// - Reporting code must handle this and continue or go to a save state. Error handler could return (e.g. in test)
/// SEVERE
/// - Log message with ERROR
/// - RouDi can still run. Error is reported
/// - Assert in DEBUG, in RELEASE continue to run
/// - Reporting code must handle this and continue
/// MODERATE
/// - Log message with ERROR
/// - RouDi can still run. Error is reported
/// - NO assert
/// - Reporting code must handle this and continue
enum class ErrorLevel : uint32_t
{
    /// Log error entry + Assert + terminate
    FATAL,
    /// warning log entry + Assert
    SEVERE,
    /// warning log entry
    MODERATE
};

using HandlerFunction = std::function<void(const Error error, const std::function<void()>, const ErrorLevel)>;

/// @brief This handler is needed for unit testing, special debugging cases and
///         other corner cases where we'd like to explicitly suppress the
///         error handling.
class ErrorHandler
{
    friend void errorHandler(const Error error, const std::function<void()> errorCallBack, const ErrorLevel level);

  public:
    static cxx::GenericRAII SetTemporaryErrorHandler(const HandlerFunction& newHandler);

    static const char* ToString(const Error error);

  protected:
    static void ReactOnErrorLevel(const ErrorLevel level, const char* errorText);

  private:
    static void DefaultHandler(const Error error,
                               const std::function<void()> errorCallBack,
                               const ErrorLevel level = ErrorLevel::FATAL);

    static const char* errorNames[];
    static iox::HandlerFunction handler;
    /// Needed, if you want to exchange the handler. Remember the old one and call it if it is not your error. The error
    /// mock needs to be the last one exchanging the handler in tests.
    static std::mutex handler_mutex;
};

/// @brief Howto use the error handler correctly
///     1.) If the error you would like to handle is not listed in ICEORYX_ERRORS(error)\...
///             macro just add them like:
///             error(MODULE_NAME__MY_FUNKY_ERROR)
///         Attention: Create an error after the following convention:
///             MODULE_NAME__A_CLEAR_BUT_SHORT_ERROR_DESCRIPTION
///         And a long name is alright!
///
///     2.) Call errorHandler(Error::kMODULE_NAME__MY_FUNKY_ERROR);
///             Please pay attention to the "k" prefix
///         The defaults for errorCallback and ErrorLevel can also be overwritten:
///             errorHandler(
///                 Error::kMODULE_NAME__MY_FUNKY_ERROR,
///                 []{ std::cout << "MyCustomCallback" << std::endl; },
///                 ErrorLevel::MODERATE
///             );
///
/// @code
/// class PrettyClass {
///     float division(float a, float b) {
///         if ( b == 0.0f ) {
///             errorHandler(Error::kPRETTY_CLASS__DIVISION_BY_ZERO);
///         }
///     }
/// };
/// @endcode
///
/// @code
/// bool called = false;
/// auto temporaryErrorHandler = ErrorHandler::SetTemporaryErrorHandler(
///     [&](const Error e, std::function<void()>, const ErrorLevel) {
///         called = true;
///     });
///
/// errorHandler(Error::kTEST__ASSERT_CALLED);
/// ASSERT_TRUE(called);
/// @endcode
void errorHandler(const Error error,
                  const std::function<void()> errorCallBack = std::function<void()>(),
                  const ErrorLevel level = ErrorLevel::FATAL);
} // namespace iox

#endif // IOX_HOOFS_ERROR_HANDLING_ERROR_HANDLING_HPP
