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

#ifndef DAQLING_MODULE_NAME
#error "This header should only be automatically included during the build with CMake"
#endif

#include "Utils/Ers.hpp"
#include <chrono>
#include <utility>

#include DAQLING_MODULE_HEADER

using namespace std::chrono_literals;

/*
 * Defines a hidden symbol that holds the module's logger instance.
 * All log entries made in the module will be done through this instance.
 * Must be hidden from the symbol table so that it is not aliased to daqling's own when the module
 * is dynamically loaded.
 */
namespace daqling {
namespace core {
extern "C" {
// forward-declare to satisfy -Werror=missing-declarations
DAQProcess *daqling_module_create(const std::string &name);
void daqling_module_delete(DAQProcess * /*module*/);
}

DAQProcess *daqling_module_create(const std::string &name) {

  auto module = new DAQLING_MODULE_NAME(name);
  return static_cast<DAQProcess *>(module);
}

void daqling_module_delete(DAQProcess *module) {
  while (module->running()) {
    // DAQProcess::m_runner_thread has yet to join,
    // we cannot safely destruct resources in derived module that m_runner_thread may be using.
    std::this_thread::sleep_for(1ms); // TODO: use a cv instead
  }
  delete module;
}
} // namespace core
} // namespace daqling
