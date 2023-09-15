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

#include "LocalReceiver.hpp"
#include "Core/ConnectionManager.hpp"
#include "Utils/ConnectionMacros.hpp"
using namespace daqling::connection;

REGISTER_RECEIVER(LocalReceiver)
LocalReceiver::LocalReceiver(uint chid, const nlohmann::json &j) : daqling::core::Receiver(chid) {
  auto &manager = daqling::core::ConnectionManager::instance();
  auto id = j.at("id").get<unsigned>();
  m_queue = std::static_pointer_cast<daqling::core::Queue>(manager.getLocalResource(id));
}

bool LocalReceiver::receive(DataTypeWrapper &bin) {
  if (m_queue->sizeGuess() != 0) {

    if (m_queue->read(bin)) {
      m_msg_handled++;
      return true;
    }
  }
  return false;
}
bool LocalReceiver::sleep_receive(DataTypeWrapper &bin) {
  if (m_queue->sizeGuess() != 0) {

    if (m_queue->sleep_read(bin)) {
      m_msg_handled++;
      return true;
    }
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(m_sleep_duration));
  return false;
}
void LocalReceiver::set_sleep_duration(uint ms) {
  m_sleep_duration = ms;
  if (m_queue) {
    m_queue->set_read_sleep_duration(ms);
  }
}