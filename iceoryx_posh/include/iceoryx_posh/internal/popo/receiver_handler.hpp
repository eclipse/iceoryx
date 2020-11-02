// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_POPO_RECEIVER_HANDLER_HPP
#define IOX_POSH_POPO_RECEIVER_HANDLER_HPP

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/popo/receiver_port.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/error_handling/error_handling.hpp"
#include "iceoryx_utils/internal/relocatable_pointer/relative_ptr.hpp"

#include <mutex>

namespace iox
{
namespace popo
{
class ThreadSafe
{
    using mutex_t = posix::mutex; // std::mutex
  public:                         // needs to be public since we want to use std::lock_guard
    ThreadSafe();
    void lock();
    void unlock();

  private:
    mutex_t m_mutex{mutex_t::Recursive::ON, mutex_t::Robust::ON};
};

class SingleThreaded
{
  public: // needs to be public since we want to use std::lock_guard
    void lock();
    void unlock();
};


template <uint32_t MaxReceivers, typename LockingPolicy>
class ReceiverHandler : public LockingPolicy
{
    using ReceiverHandler_t = ReceiverHandler<MaxReceivers, LockingPolicy>;
    using this_type = ReceiverHandler_t;

  public:
    using ReceiverVector_t = cxx::vector<relative_ptr<ReceiverPortType::MemberType_t>, MaxReceivers>;
    class AppContext
    {
        friend ReceiverHandler_t;

      public:
        bool hasLastChunk();
        void deliverChunk(const mepoo::SharedChunk f_chunk);
        void updateLastChunk(const mepoo::SharedChunk f_chunk);
        bool hasReceivers();
        void enableDoDeliverOnSubscription();
        ReceiverVector_t& getReceiverList() noexcept;

      private:
        AppContext(ReceiverHandler_t& f_receiverHandler);

        ReceiverHandler_t& m_receiverHandler;
    };

    class RouDiContext
    {
        friend ReceiverHandler_t;

      public:
        bool addNewReceiver(ReceiverPortType::MemberType_t* const f_receiver);
        void removeReceiver(ReceiverPortType::MemberType_t* const f_receiver);
        void removeAll();

      private:
        RouDiContext(ReceiverHandler_t& f_receiverHandler);

        ReceiverHandler_t& m_receiverHandler;
    };


  public:
    using lockGuard_t = std::lock_guard<ReceiverHandler<MaxReceivers, LockingPolicy>>;

    AppContext appContext()
    {
        return AppContext(*this);
    }

    RouDiContext roudiContext()
    {
        return RouDiContext(*this);
    }

    void updateLastChunk(const mepoo::SharedChunk f_chunk);
    /// checks for a sample for delivering on subscription
    /// @return true if there is a valid sample for delivering on subscription
    bool hasLastChunk();
    bool hasReceivers();
    bool addNewReceiver(ReceiverPortType::MemberType_t* const f_receiver);
    void removeReceiver(ReceiverPortType::MemberType_t* const f_receiver);
    void removeAll();
    void enableDoDeliverOnSubscription();
    /// checks if delivering on subscription in enabled
    /// @return true if delivering on subscription is enabled
    bool doesDeliverOnSubscribe() const;
    uint32_t getMaxDeliveryFiFoCapacity();
    /// Returns the list of receivers
    ReceiverVector_t& getReceiverList() noexcept;

  private:
    std::atomic_bool m_doDeliverOnSubscription{false};
    ReceiverVector_t m_receiverVector;
    mepoo::SharedChunk m_lastChunk{nullptr};
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/receiver_handler.inl"

#endif // IOX_POSH_POPO_RECEIVER_HANDLER_HPP
