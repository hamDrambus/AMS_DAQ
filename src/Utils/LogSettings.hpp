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

#pragma once
#include "Utils/CommandlineInterpreter.hpp"
#include <string>
#include <tuple>
#include <vector>
namespace daqling {
namespace utilities {
/*
 * LogSettings
 * Description: Sets up the ERS logging streams used by daqling.
 * Date: July 2021
 */
struct LogSettings {

  /**
   * @brief Sets up the streams based on the commandline arguments.
   * @param input commandlineInterpreter object, with configurations.
   */
  static void setupLogs(const CommandlineInterpreter &input) {
    std::vector<std::string> const table = {"DEBUG", "LOG", "INFO", "WARNING", "ERROR"};
    std::map<std::string, std::vector<std::string>::const_iterator> areas;

    areas["core"] = find(table.begin(), table.end(), input.core_lvl);
    areas["module"] = find(table.begin(), table.end(), input.module_lvl);
    areas["connection"] = find(table.begin(), table.end(), input.connection_lvl);
    // set up the streams:
    for (auto level = table.begin(); level != table.end(); ++level) {
      std::string stream = (static_cast<std::string>("TDAQ_ERS_")) + (*level);
      std::string configs = ""; // NOLINT  sets configs=configs for some reason
      bool any = false;
      bool all = true;
      for (auto i : areas) {
        ERS_ASSERT(i.second != table.end());
        if (level < i.second) {
          if (configs.empty()) {
            configs += "filter(!" + i.first;
            all = false;
          } else {
            configs += ",!" + i.first;
          }
        } else {
          any = true;
        }
      }
      if (!any) {
        configs = "null";
      } else {
        if (!all) {
          configs += "),";
        }
        if (*level == "WARNING" || *level == "ERROR") {
          configs += "throttle,dlstderr";
        } else {
          configs += "dlstdout";
        }
        configs += ",ZMQSink(" + input.name + ")";
      }
      setenv(stream.c_str(), configs.c_str(), 0);
    }
    // Setup fatal stream to terminate
    setenv("TDAQ_ERS_FATAL", "dlstderr,exit", 0);
  }
};

} // namespace utilities
} // namespace daqling
