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

#include "ErsTestModule.hpp"
#include "Utils/Ers.hpp"

using namespace daqling::core;
using namespace daqling::module;

ErsTestModule::ErsTestModule(const std::string &n) : DAQProcess(n) { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void ErsTestModule::configure() {
  daqling::core::DAQProcess::configure();

  ERS_INFO("");
}

void ErsTestModule::start(unsigned run_num) {
  daqling::core::DAQProcess::start(run_num);
  ErsTestIssue i(ERS_HERE, "Hi");
  ers::info(i);
  ERS_WARNING("ErsTestModule Package Name: " << i.context().package_name());

  ERS_INFO("Package name:" << i.context().package_name());
}

void ErsTestModule::stop() {
  daqling::core::DAQProcess::stop();
  ERS_INFO("");
  ers::warning(ErsTestInherittedIssue(ERS_HERE, "fish", "salmon"));
}

void ErsTestModule::runner() noexcept {
  ERS_INFO("Running...");
  ERS_DEBUG(0, "Hi from debug");
  while (m_run) {
    ers::error(ErsTestInherittedIssue(ERS_HERE, "fish", "salmon"));
  }
  ERS_INFO("Runner stopped");
}
