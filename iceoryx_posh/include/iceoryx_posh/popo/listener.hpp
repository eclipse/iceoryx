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

#ifndef IOX_POSH_POPO_LISTENER_HPP
#define IOX_POSH_POPO_LISTENER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_listener.hpp"
#include "iceoryx_posh/popo/enum_trigger_type.hpp"
#include "iceoryx_posh/popo/notification_attorney.hpp"
#include "iceoryx_posh/popo/notification_callback.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_utils/cxx/expected.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/cxx/type_traits.hpp"
#include "iceoryx_utils/internal/concurrent/loffli.hpp"
#include "iceoryx_utils/internal/concurrent/smart_lock.hpp"

#include <thread>

namespace iox
{
namespace popo
{
enum class ListenerError
{
    INVALID_STATE,
    LISTENER_FULL,
    EVENT_ALREADY_ATTACHED,
    EMPTY_INVALIDATION_CALLBACK
};

/// @brief The Listener is a class which reacts to registered events by
///        executing a corresponding callback concurrently. This is achieved via
///        an encapsulated thread inside this class.
/// @note  The Listener is threadsafe and can be used without any restrictions concurrently.
/// @attention Calling detachEvent for the same event from multiple threads is supported but
///            can cause a race condition if you attach the same event again concurrently from
///            another thread.
///            Example:
///             1. One calls detachEvent [1] from thread A, B and C
///             2. thread B wins and detaches event [1]
///             3. A new thread D spawns and would like to attach event [1] again while thread A and C are
///                 still waiting to detach [1].
///             4. Thread A wins but cannot detach event [1] since it is not attached.
///             5. Thread D wins and attaches event [1].
///             6. Finally thread C can continue and detaches event [1] again.
///
///            If thread D is executed last then the event is attached. So depending on the operating
///            system defined execution order the event is either attached or detached.
///
///            Best practice: Detach a specific event only from one specific thread and not
///                           from multiple contexts.
class Listener
{
  public:
    Listener() noexcept;
    Listener(const Listener&) = delete;
    Listener(Listener&&) = delete;
    ~Listener();

    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&&) = delete;

    /// @brief Attaches an event. Hereby the event is defined as a class T, the eventOrigin, an enum which further
    ///        defines the event inside the class and the corresponding callback which will be called when the event
    ///        occurs.
    /// @note This method can be called from any thread concurrently without any restrictions!
    ///        Furthermore, attachEvent does not take ownership of callback in the underlying eventCallback or the
    ///        optional contextData. The user has to ensure that both will live as long as the event is attached.
    /// @tparam[in] T type of the class which will signal the event
    /// @param[in] eventOrigin the object which will signal the event (the origin)
    /// @param[in] eventType enum required to specify the type of event inside of eventOrigin
    /// @param[in] eventCallback callback which will be executed concurrently when the event occurs. has to be created
    /// with iox::popo::createNotificationCallback
    /// @return If an error occurs the enum packed inside an expected which describes the error.
    template <typename T,
              typename EventType,
              typename ContextDataType,
              typename = std::enable_if_t<std::is_enum<EventType>::value>>
    cxx::expected<ListenerError> attachEvent(T& eventOrigin,
                                             const EventType eventType,
                                             const NotificationCallback<T, ContextDataType>& eventCallback) noexcept;

    /// @brief Attaches an event. Hereby the event is defined as a class T, the eventOrigin and
    ///        the corresponding callback which will be called when the event occurs.
    /// @note This method can be called from any thread concurrently without any restrictions!
    ///        Furthermore, attachEvent does not take ownership of callback in the underlying eventCallback or the
    ///        optional contextData. The user has to ensure that both will live as long as the event is attached.
    /// @tparam[in] T type of the class which will signal the event
    /// @param[in] eventOrigin the object which will signal the event (the origin)
    /// @param[in] eventCallback callback which will be executed concurrently when the event occurs. Has to be created
    /// with iox::popo::createNotificationCallback
    /// @return If an error occurs the enum packed inside an expected which describes the error.
    template <typename T, typename ContextDataType>
    cxx::expected<ListenerError> attachEvent(T& eventOrigin,
                                             const NotificationCallback<T, ContextDataType>& eventCallback) noexcept;

    /// @brief Detaches an event. Hereby, the event is defined as a class T, the eventOrigin and
    ///        the eventType with further specifies the event inside of eventOrigin
    /// @note This method can be called from any thread concurrently without any restrictions!
    /// @tparam[in] T type of the class which will signal the event
    /// @param[in] eventOrigin the object which will signal the event (the origin)
    /// @param[in] eventType enum required to specify the type of event inside of eventOrigin
    template <typename T, typename EventType, typename = std::enable_if_t<std::is_enum<EventType>::value>>
    void detachEvent(T& eventOrigin, const EventType eventType) noexcept;

    /// @brief Detaches an event. Hereby, the event is defined as a class T, the eventOrigin.
    /// @note This method can be called from any thread concurrently without any restrictions!
    /// @tparam[in] T type of the class which will signal the event
    template <typename T>
    void detachEvent(T& eventOrigin) noexcept;

    /// @brief Returns the capacity of the Listener
    /// @return capacity of the Listener
    static constexpr uint64_t capacity() noexcept;

    /// @brief Returns the size of the Listener
    /// @return size of the Listener
    uint64_t size() const noexcept;

  protected:
    Listener(ConditionVariableData& conditionVariableData) noexcept;

  private:
    class Event_t;

    void threadLoop() noexcept;
    cxx::expected<uint32_t, ListenerError>
    addEvent(void* const origin,
             void* const userType,
             const uint64_t eventType,
             const uint64_t eventTypeHash,
             internal::GenericCallbackRef_t callback,
             internal::TranslationCallbackRef_t translationCallback,
             const cxx::MethodCallback<void, uint64_t> invalidationCallback) noexcept;

    void removeTrigger(const uint64_t index) noexcept;

  private:
    enum class NoEnumUsed : EventEnumIdentifier
    {
        PLACEHOLDER = 0
    };

    class Event_t
    {
      public:
        ~Event_t();

        bool isEqualTo(const void* const origin, const uint64_t eventType, const uint64_t eventTypeHash) const noexcept;
        bool reset() noexcept;
        bool init(const uint64_t eventId,
                  void* const origin,
                  void* const userType,
                  const uint64_t eventType,
                  const uint64_t eventTypeHash,
                  internal::GenericCallbackRef_t callback,
                  internal::TranslationCallbackRef_t translationCallback,
                  const cxx::MethodCallback<void, uint64_t> invalidationCallback) noexcept;
        void executeCallback() noexcept;
        bool isInitialized() const noexcept;

      private:
        static constexpr uint64_t INVALID_ID = std::numeric_limits<uint64_t>::max();

        void* m_origin = nullptr;
        uint64_t m_eventType = INVALID_ID;
        uint64_t m_eventTypeHash = INVALID_ID;

        internal::GenericCallbackPtr_t m_callback = nullptr;
        internal::TranslationCallbackPtr_t m_translationCallback = nullptr;
        void* m_userType = nullptr;

        uint64_t m_eventId = INVALID_ID;
        cxx::MethodCallback<void, uint64_t> m_invalidationCallback;
    };

    class IndexManager_t
    {
      public:
        IndexManager_t() noexcept;
        bool pop(uint32_t& index) noexcept;
        void push(const uint32_t index) noexcept;
        uint64_t indicesInUse() const noexcept;

        using LoFFLi = concurrent::LoFFLi;
        LoFFLi::Index_t
            m_loffliStorage[LoFFLi::requiredIndexMemorySize(MAX_NUMBER_OF_EVENTS_PER_LISTENER) / sizeof(uint32_t)];
        LoFFLi m_loffli;
        std::atomic<uint64_t> m_indicesInUse{0U};
    } m_indexManager;


    std::thread m_thread;
    concurrent::smart_lock<Event_t, std::recursive_mutex> m_events[MAX_NUMBER_OF_EVENTS_PER_LISTENER];
    std::mutex m_addEventMutex;

    std::atomic_bool m_wasDtorCalled{false};
    ConditionVariableData* m_conditionVariableData = nullptr;
    ConditionListener m_conditionListener;
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/listener.inl"

#endif
