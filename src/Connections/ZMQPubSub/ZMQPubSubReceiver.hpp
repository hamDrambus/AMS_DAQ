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
#include "Core/Receiver.hpp"
#include "nlohmann/json.hpp"
#include <memory>
#include <zmq.hpp>

namespace daqling {
namespace connection {
class ZMQPubSubReceiver : public daqling::core::Receiver {
public:
  ZMQPubSubReceiver(uint chid, const nlohmann::json &j);
  ~ZMQPubSubReceiver() override;
  // Prevent copying and moving.
  ZMQPubSubReceiver(ZMQPubSubReceiver const &) = delete;            // Copy construct
  ZMQPubSubReceiver(ZMQPubSubReceiver &&) = delete;                 // Move construct
  ZMQPubSubReceiver &operator=(ZMQPubSubReceiver const &) = delete; // Copy assign
  ZMQPubSubReceiver &operator=(ZMQPubSubReceiver &&) = delete;      // Move assign
  void set_sleep_duration(uint ms) override;

protected:
  bool receive(DataTypeWrapper &bin) override;
  bool sleep_receive(DataTypeWrapper &bin) override;
  bool m_private_zmq_context{true};
  // ZMQ Context number of threads to use
  uint8_t ioT = 1;
  zmq::context_t *m_context;
  std::unique_ptr<zmq::socket_t> m_socket;
};
} // namespace connection
} // namespace daqling