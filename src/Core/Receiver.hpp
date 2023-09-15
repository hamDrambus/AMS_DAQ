/**
 * Copyright (C) 2019-2021 CERN
 *
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "Common/DataType.hpp"
#include "Queue.hpp"
#include <any>
#include <atomic>
#include <thread>
namespace daqling {
namespace core {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Receiver {
public:
  // Virtual destructor
  virtual ~Receiver() = default;

  virtual bool receive(DataTypeWrapper &bin) = 0;
  virtual bool sleep_receive(DataTypeWrapper &bin);
  virtual void set_sleep_duration(uint ms);
  virtual bool start();
  virtual bool stop();
  virtual std::atomic<size_t> &getMsgsHandled();
  virtual std::atomic<size_t> &getPcqSize();

protected:
  Receiver(uint chid);
  uint m_chid;
  std::atomic<size_t> m_msg_handled{};
  std::atomic<size_t> m_pcq_size{};
  uint m_sleep_duration{};
};
class QueueReceiver : public Receiver {
public:
  QueueReceiver(uint /*chid*/, std::unique_ptr<Queue> /*ptr*/,
                const std::shared_ptr<Receiver> & /*chained_ptr*/);
  bool receive(DataTypeWrapper &bin) override;
  bool sleep_receive(DataTypeWrapper &bin) override;
  bool start() override;
  bool stop() override;
  virtual void setChainedReceiver(std::shared_ptr<daqling::core::Receiver> /*ptr*/);
  std::atomic<size_t> &getMsgsHandled() override;
  void set_sleep_duration(uint ms) override;

protected:
  bool addReceiveHandler();
  std::thread m_handler_thread;
  std::shared_ptr<daqling::core::Receiver> m_chained_receiver;
  std::atomic<bool> m_stop_handler{};
  std::unique_ptr<Queue> m_queue;
};

} // namespace core
} // namespace daqling
