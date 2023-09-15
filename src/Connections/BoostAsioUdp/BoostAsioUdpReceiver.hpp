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
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <memory>

namespace daqling {
namespace connection {
using namespace boost::asio::ip;

class BoostAsioUdpReceiver : public daqling::core::Receiver {
public:
  BoostAsioUdpReceiver(uint chid, const nlohmann::json &j = NULL);
  bool stop() override;

protected:
  bool receive(DataTypeWrapper &bin) override;
  bool sleep_receive(DataTypeWrapper &bin) override;
  void handle_receive(const boost::system::error_code & /*error*/, std::size_t /*size*/);
  void wait_callback(const boost::system::error_code &error);

  boost::asio::io_context m_io_context;
  std::unique_ptr<udp::endpoint> m_dest_endpoint;
  std::unique_ptr<udp::endpoint> m_src_endpoint;
  std::unique_ptr<udp::socket> m_socket;
  std::unique_ptr<boost::asio::deadline_timer> m_timeout;
  boost::array<char, 2048> m_recv_buf{};
  size_t m_len{};
};
} // namespace connection
} // namespace daqling