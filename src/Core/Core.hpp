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

#ifndef DAQLING_CORE_CORE_HPP
#define DAQLING_CORE_CORE_HPP

#include "Command.hpp"
#include "ModuleManager.hpp"
#include "Utils/Ers.hpp"

namespace daqling {
namespace core {

class Core {
public:
  Core(unsigned port) : m_port{port} {};

  void setupCommandServer();
  bool getShouldStop();
  std::mutex *getMutex() { return m_command.getMutex(); };
  std::condition_variable *getCondVar() { return m_command.getCondVar(); };

private:
  unsigned m_port;

  // ZMQ ConnectionManager
  daqling::core::ConnectionManager &m_connections = daqling::core::ConnectionManager::instance();
  // Command exchange
  daqling::core::Command &m_command = daqling::core::Command::instance();
  // JSON Configuration map
  daqling::core::Configuration &m_config = daqling::core::Configuration::instance();
  // Plugin manager
  daqling::core::ModuleManager &m_plugin = daqling::core::ModuleManager::instance();
};

} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_CORE_HPP
