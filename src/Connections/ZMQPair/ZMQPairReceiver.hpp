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
class ZMQPairReceiver : public daqling::core::Receiver {
public:
  ZMQPairReceiver(uint chid, const nlohmann::json &j);
  void set_sleep_duration(uint ms) override;
  ~ZMQPairReceiver() override;
  // Prevent copying and moving.
  ZMQPairReceiver(ZMQPairReceiver const &) = delete;            // Copy construct
  ZMQPairReceiver(ZMQPairReceiver &&) = delete;                 // Move construct
  ZMQPairReceiver &operator=(ZMQPairReceiver const &) = delete; // Copy assign
  ZMQPairReceiver &operator=(ZMQPairReceiver &&) = delete;      // Move assign
protected:
  bool m_private_zmq_context{true};
  bool receive(DataTypeWrapper &bin) override;
  bool sleep_receive(DataTypeWrapper &bin) override;
  // ZMQ Context number of threads to use
  uint8_t ioT = 1;
  zmq::context_t *m_context;
  std::unique_ptr<zmq::socket_t> m_socket;
};
} // namespace connection
} // namespace daqling