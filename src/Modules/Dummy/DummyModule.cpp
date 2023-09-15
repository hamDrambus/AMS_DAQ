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

#include "DummyModule.hpp"
#include "Utils/Ers.hpp"
using namespace daqling::core;
using namespace daqling::module;
DummyModule::DummyModule(const std::string &n) : DAQProcess(n) { ERS_INFO(""); }

DummyModule::~DummyModule() { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void DummyModule::configure() {
  daqling::core::DAQProcess::configure();
  ERS_INFO("");

  registerCommand("foobar", "foobarring", "foobarred", &DummyModule::foobar, this, _1);
}

void DummyModule::start(unsigned run_num) {
  daqling::core::DAQProcess::start(run_num);
  ERS_INFO("");
}

void DummyModule::stop() {
  daqling::core::DAQProcess::stop();
  ERS_INFO("");
}

void DummyModule::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
  }
  ERS_INFO("Runner stopped");
}

void DummyModule::foobar(const std::string &arg) {
  ERS_INFO("Inside custom command. Got argument: " << arg);
}