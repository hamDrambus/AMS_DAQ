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

#include "BoostAsioUdpSender.hpp"
#include "BoostAsioUdpIssues.hpp"
#include "Utils/ConnectionMacros.hpp"
#include "Utils/Ers.hpp"
#include <boost/bind.hpp>

using namespace daqling::connection;

REGISTER_SENDER(BoostAsioUdpSender)

BoostAsioUdpSender::BoostAsioUdpSender(uint chid, const nlohmann::json &j)
    : daqling::core::Sender(chid) {
  if (j.at("transport") == "udp") {
    try {
      int src_port = (j.contains("src_port") ? j.at("src_port").get<int>() : 0);
      auto host = boost::asio::ip::address::from_string(j.at("host").get<std::string>());
      m_socket = std::make_unique<udp::socket>(m_io_context, udp::endpoint(host, src_port));
      m_dest_endpoint = std::make_unique<udp::endpoint>(host, j.at("dest_port").get<int>());
      m_timeout = std::make_unique<boost::asio::deadline_timer>(m_io_context);
    } catch (const std::exception &e) {
      throw CannotAddChannel(ERS_HERE, e.what());
    }
  } else {
    throw InvalidTransportType(ERS_HERE, j.at("transport").get<std::string>().c_str());
  }
}

bool BoostAsioUdpSender::send(DataTypeWrapper &bin) {
  try {
    auto any_data = bin.getDataTypePtr();
    ERS_DEBUG(0, "Sending message of size: " << any_data->size());
    if (m_socket->send_to(boost::asio::buffer(any_data->data(), any_data->size()),
                          *m_dest_endpoint) == any_data->size()) {
      ++m_msg_handled;
      return true;
    }
    return false;

  } catch (std::exception &e) {
    throw SendFailed(ERS_HERE, e.what());
  }
}
bool BoostAsioUdpSender::sleep_send(DataTypeWrapper &bin) {
  try {
    auto any_data = bin.getDataTypePtr();
    uint len;
    m_socket->async_send_to(boost::asio::buffer(any_data->data(), any_data->size()),
                            *m_dest_endpoint,
                            [&](const boost::system::error_code &error, std::size_t size) {
                              if (error || (size == 0u)) {
                                len = 0;
                              } else {
                                m_timeout->cancel();
                                len = size;
                              }
                            });
    ERS_DEBUG(0, "Sending message of size: " << any_data->size());
    m_timeout->expires_from_now(boost::posix_time::milliseconds(m_sleep_duration));
    m_timeout->async_wait([&](const boost::system::error_code &error) {
      if (error) {
        return;
      }
      m_socket->cancel();
    });
    m_io_context.restart();
    m_io_context.run();

    if (len == any_data->size()) {
      ++m_msg_handled;
      return true;
    }
    ERS_DEBUG(0, "Sleep send failed.");
    return false;

  } catch (std::exception &e) {
    throw SendFailed(ERS_HERE, e.what());
  }
}