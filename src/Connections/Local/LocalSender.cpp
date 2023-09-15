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

#include "LocalSender.hpp"
#include "Core/ConnectionManager.hpp"
#include "Utils/ConnectionMacros.hpp"
using namespace daqling::connection;

REGISTER_SENDER(LocalSender)
LocalSender::LocalSender(uint chid, const nlohmann::json &j) : daqling::core::Sender(chid) {
  auto &manager = daqling::core::ConnectionManager::instance();
  auto id = j.at("id").get<unsigned>();
  m_queue = std::static_pointer_cast<daqling::core::Queue>(manager.getLocalResource(id));
}
bool LocalSender::send(DataTypeWrapper &bin) {
  if (m_queue->write(bin)) {
    m_msg_handled++;
    return true;
  }
  return false;
}
bool LocalSender::sleep_send(DataTypeWrapper &bin) {
  if (m_queue->sleep_write(bin)) {
    m_msg_handled++;
    return true;
  }
  return false;
}
void LocalSender::set_sleep_duration(uint ms) {
  m_sleep_duration = ms;
  if (m_queue) {
    m_queue->set_write_sleep_duration(ms);
  }
}