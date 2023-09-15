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
#include <mutex>
#include <thread>
namespace daqling {
namespace core {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Sender {
public:
  // virtual destructor
  virtual ~Sender() = default;
  virtual bool send(DataTypeWrapper &bin) = 0;
  virtual bool sleep_send(DataTypeWrapper &bin);
  virtual bool start();
  virtual bool stop();
  virtual void set_sleep_duration(uint ms);
  virtual std::atomic<size_t> &getMsgsHandled();
  virtual std::atomic<size_t> &getPcqSize();

protected:
  Sender(uint chid);
  uint m_chid;
  std::atomic<size_t> m_msg_handled{};
  std::atomic<size_t> m_pcq_size{};
  uint m_sleep_duration{};

private:
};

class QueueSender : public Sender {
public:
  QueueSender(uint /*chid*/, std::unique_ptr<Queue> /*ptr*/,
              const std::shared_ptr<Sender> & /*chained_ptr*/);
  bool send(DataTypeWrapper &bin) override;
  bool sleep_send(DataTypeWrapper &bin) override;
  bool start() override;
  bool stop() override;
  virtual void setChainedSender(std::shared_ptr<daqling::core::Sender> /*ptr*/);
  std::atomic<size_t> &getMsgsHandled() override;
  void set_sleep_duration(uint ms) override;

protected:
  bool addSendHandler();
  std::thread m_handler_thread;
  std::shared_ptr<daqling::core::Sender> m_chained_sender;
  std::atomic<bool> m_stop_handler{};
  std::unique_ptr<Queue> m_queue;
};

} // namespace core
} // namespace daqling
