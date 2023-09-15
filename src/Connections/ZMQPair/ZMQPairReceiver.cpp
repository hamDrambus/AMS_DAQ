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

#include "ZMQPairReceiver.hpp"
#include "Core/ConnectionManager.hpp"
#include "Core/ResourceFactory.hpp"
#include "Utils/ConnectionMacros.hpp"
#include "Utils/Ers.hpp"
#include "ZMQIssues.hpp"
#include <iomanip>
using namespace daqling::connection;

REGISTER_RECEIVER(ZMQPairReceiver)
ZMQPairReceiver::~ZMQPairReceiver() {
  m_socket->setsockopt(ZMQ_LINGER, 1);
  if (m_private_zmq_context) {
    m_socket.reset();
    delete m_context;
  }
}
ZMQPairReceiver::ZMQPairReceiver(uint chid, const nlohmann::json &j)
    : daqling::core::Receiver(chid) {
  try {
    std::string connStr;
    if (j.at("transport") == "ipc") {
      connStr = "ipc://" + j.at("path").get<std::string>();
      m_context = new zmq::context_t(ioT);
    } else if (j.at("transport") == "tcp") {
      connStr = "tcp://" + j.at("host").get<std::string>() + ":" +
                std::to_string(j.at("port").get<uint>());
      m_context = new zmq::context_t(ioT);
    } else if (j.at("transport") == "inproc") {
      connStr = "inproc://" + j.at("endpoint").get<std::string>();
      auto &manager = daqling::core::ConnectionManager::instance();
      auto id = j.at("id").get<unsigned>();
      auto context =
          std::static_pointer_cast<daqling::core::ZMQ_Context>(manager.getLocalResource(id));
      m_context = context->getContext();
      m_private_zmq_context = false;
    } else {
      throw InvalidTransportType(ERS_HERE, j.at("transport").get<std::string>().c_str());
    }
    m_socket = std::make_unique<zmq::socket_t>(*m_context, ZMQ_PAIR);
    m_socket->connect(connStr.c_str());

    ERS_INFO(" Adding CLIENT channel for: [" << m_chid << "] connect: " << connStr);
  } catch (ers::Issue &i) {
    throw CannotAddChannel(ERS_HERE, "caught issue", i);
  } catch (const std::exception &e) {
    throw CannotAddChannel(ERS_HERE, e.what());
  }
}
void ZMQPairReceiver::set_sleep_duration(uint ms) { m_socket->setsockopt(ZMQ_RCVTIMEO, ms); }

bool ZMQPairReceiver::receive(DataTypeWrapper &bin) {
  zmq::message_t msg;
  if (m_socket->recv(&msg, ZMQ_DONTWAIT)) {
    bin.reconstruct_or_store(msg.data(), msg.size());
    ++m_msg_handled;
    return true;
  }
  return false;
}
bool ZMQPairReceiver::sleep_receive(DataTypeWrapper &bin) {
  zmq::message_t msg;
  if (m_socket->recv(&msg)) {
    bin.reconstruct_or_store(msg.data(), msg.size());
    ++m_msg_handled;
    return true;
  }
  return false;
}