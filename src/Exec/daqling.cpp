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

#include "Core/Core.hpp"
#include "Utils/Binary.hpp"
#include "Utils/CommandlineInterpreter.hpp"
#include "Utils/Ers.hpp"
#include "Utils/LogSettings.hpp"

int main(int argc, char **argv) {

  auto args = daqling::utilities::CommandlineInterpreter::parse(argc, argv);
  if (!args.success) {
    return EXIT_FAILURE;
  }

  daqling::utilities::LogSettings::setupLogs(args);
  // get port from options:
  unsigned port = args.port;
  ERS_DEBUG(0, "Setup commandserver with port: " << port);
  daqling::core::Core c(port);
  c.setupCommandServer();

  std::mutex *mtx = c.getMutex();
  std::condition_variable *cv = c.getCondVar();
  std::unique_lock<std::mutex> lk(*mtx);

  cv->wait(lk, [&] { return c.getShouldStop(); });
  lk.unlock();

  return EXIT_SUCCESS;
}
