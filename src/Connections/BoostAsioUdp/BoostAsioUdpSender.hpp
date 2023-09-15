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
#include <atomic>
#include <boost/asio.hpp>
#include <memory>

namespace daqling {
namespace connection {
using namespace boost::asio::ip;

class BoostAsioUdpSender : public daqling::core::Sender {
public:
  BoostAsioUdpSender(uint chid, const nlohmann::json &j = NULL);

protected:
  bool send(DataTypeWrapper &bin) override;
  bool sleep_send(DataTypeWrapper &bin) override;

  boost::asio::io_context m_io_context;
  std::unique_ptr<udp::endpoint> m_dest_endpoint;
  std::unique_ptr<udp::socket> m_socket;
  std::unique_ptr<boost::asio::deadline_timer> m_timeout;
};
} // namespace connection
} // namespace daqling