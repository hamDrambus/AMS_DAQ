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
#include "Core/Sender.hpp"
#include "nlohmann/json.hpp"
#include <memory>
#include <zmq.hpp>

namespace daqling {
namespace connection {

class ZMQPubSubSender : public daqling::core::Sender {
public:
  ZMQPubSubSender(uint chid, const nlohmann::json &j);
  ~ZMQPubSubSender() override;
  // Prevent copying and moving.
  ZMQPubSubSender(ZMQPubSubSender const &) = delete;            // Copy construct
  ZMQPubSubSender(ZMQPubSubSender &&) = delete;                 // Move construct
  ZMQPubSubSender &operator=(ZMQPubSubSender const &) = delete; // Copy assign
  ZMQPubSubSender &operator=(ZMQPubSubSender &&) = delete;      // Move assign

protected:
  bool send(DataTypeWrapper &bin) override;
  bool m_private_zmq_context{true};
  // ZMQ Context number of threads to use
  uint8_t ioT = 1;
  zmq::context_t *m_context;
  std::unique_ptr<zmq::socket_t> m_socket;
};
} // namespace connection
} // namespace daqling