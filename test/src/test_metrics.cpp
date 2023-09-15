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

#include "Core/Statistics.hpp"

#include "Utils/Ers.hpp"
#include "nlohmann/json.hpp"
#include "zmq.hpp"
#include <atomic>
#include <unistd.h>
using nlohmann::json;

int main(int /*unused*/, char ** /*unused*/) {

  std::atomic<float> buffer_occupation{};
  std::atomic<int> packets;
  buffer_occupation = 0.1;
  packets = 0;
  json j;
  j["stats_uri"] = "tcp://*:5556";
  daqling::core::Statistics stat(j);
  stat.start();

  stat.registerVariable<std::atomic<float>, float>(&buffer_occupation, "AverageBufferOccupation",
                                                   daqling::core::metrics::AVERAGE,
                                                   daqling::core::metrics::FLOAT);
  stat.registerVariable<std::atomic<float>, float>(&buffer_occupation, "BufferOccupation",
                                                   daqling::core::metrics::LAST_VALUE,
                                                   daqling::core::metrics::FLOAT);
  stat.registerVariable<std::atomic<int>, int>(
      &packets, "PacketsRate", daqling::core::metrics::RATE, daqling::core::metrics::INT);
  // stat.registerVariable("NumberOfPackets", &packets);
  while (true) {
    usleep(500000);
    packets += 2;
    buffer_occupation = buffer_occupation + 3.1;
    ERS_INFO("Buffer occupation: " << buffer_occupation);
  }

  return 0;
}
