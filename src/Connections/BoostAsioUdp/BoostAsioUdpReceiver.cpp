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

#include "BoostAsioUdpReceiver.hpp"
#include "BoostAsioUdpIssues.hpp"
#include "Utils/ConnectionMacros.hpp"
#include "Utils/Ers.hpp"
#include <boost/bind.hpp>

using namespace daqling::connection;

REGISTER_RECEIVER(BoostAsioUdpReceiver)
BoostAsioUdpReceiver::BoostAsioUdpReceiver(uint chid, const nlohmann::json &j)
    : daqling::core::Receiver(chid), m_io_context() {
  std::string connStr;
  if (j.at("transport") == "udp") {
    try {
      int src_port = (j.contains("src_port") ? j.at("src_port").get<int>() : 0);
      auto host = boost::asio::ip::address::from_string(j.at("host").get<std::string>());
      m_socket = std::make_unique<udp::socket>(m_io_context);
      m_dest_endpoint = std::make_unique<udp::endpoint>(host, j.at("dest_port").get<int>());
      m_src_endpoint =
          std::make_unique<udp::endpoint>(boost::asio::ip::address_v4::any(), src_port);
      m_socket->open(udp::v4());
      m_socket->bind(*m_dest_endpoint);
      m_timeout = std::make_unique<boost::asio::deadline_timer>(m_io_context);
    } catch (const std::exception &e) {
      throw CannotAddChannel(ERS_HERE, e.what());
    }
  } else {
    throw InvalidTransportType(ERS_HERE, j.at("transport").get<std::string>().c_str());
  }
}
bool BoostAsioUdpReceiver::receive(DataTypeWrapper &bin) {
  m_socket->async_receive_from(boost::asio::buffer(m_recv_buf), *m_src_endpoint,
                               boost::bind(&BoostAsioUdpReceiver::handle_receive, this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
  m_timeout->expires_from_now(boost::posix_time::milliseconds(1));
  m_timeout->async_wait(
      boost::bind(&BoostAsioUdpReceiver::wait_callback, this, boost::asio::placeholders::error));
  m_io_context.restart();
  m_io_context.run();
  if (m_len != 0u) {
    ERS_DEBUG(0, "Received msg with size: " << m_len);
    bin.reconstruct_or_store(m_recv_buf.data(), m_len);
    ++m_msg_handled;
    return true;
  }
  return false;
}
bool BoostAsioUdpReceiver::sleep_receive(DataTypeWrapper &bin) {
  m_socket->async_receive_from(boost::asio::buffer(m_recv_buf), *m_src_endpoint,
                               boost::bind(&BoostAsioUdpReceiver::handle_receive, this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
  m_timeout->expires_from_now(boost::posix_time::milliseconds(m_sleep_duration));
  m_timeout->async_wait(
      boost::bind(&BoostAsioUdpReceiver::wait_callback, this, boost::asio::placeholders::error));
  m_io_context.restart();
  m_io_context.run();
  if (m_len != 0u) {
    ERS_DEBUG(0, "Received msg with size: " << m_len);
    bin.reconstruct_or_store(m_recv_buf.data(), m_len);
    ++m_msg_handled;
    return true;
  }
  return false;
}
bool BoostAsioUdpReceiver::stop() {
  m_socket->cancel();
  return true;
}
void BoostAsioUdpReceiver::wait_callback(const boost::system::error_code &error) {
  if (error) {
    return;
  }
  m_socket->cancel();
}
void BoostAsioUdpReceiver::handle_receive(const boost::system::error_code &error,
                                          std::size_t size) {
  if (error || (size == 0u)) {
    m_len = 0;
  } else {
    m_timeout->cancel();
    m_len = size;
  }
}