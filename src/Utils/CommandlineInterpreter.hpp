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

#include "Utils/Ers.hpp"
#include <map>
#include <string>

namespace daqling {
ERS_DECLARE_ISSUE(utilities,                                             // namespace
                  CommandLineIssue,                                      // Issue type
                  "Failed to interpret commandline arguments: " << what, // Message
                  ((const char *)what))                                  // Attributes
namespace utilities {
/*
 * CommandlineInterpreter
 * Description: Responsible for interpreting commandline arguments to the daqling.cpp executable.
 * Date: July 2021
 */
struct CommandlineInterpreter {

  /**
   * @brief Parses the commandline arguments, into a CommandlineInterpreter object.
   * @param argc number of arguments
   * @param argv arguments.
   * @return CommandlineInterpreter object containing parsed arguments.
   */
  static CommandlineInterpreter parse(int argc, char **argv) {
    CommandlineInterpreter output;
    auto map = CommandlineInterpreter::map(argc, argv);
    if (map.count("help") != 0u or argc == 1) {
      INFO("Usage: daqling --port <port> [--name <name>] [--core_lvl <core_lvl>] [--module_lvl "
           "<module_lvl>] [--connection_lvl <connection_lvl>]");
      return output;
    }
    try {
      output.core_lvl = map["core_lvl"];
      output.name = map["name"];
      output.module_lvl = map["module_lvl"];
      output.connection_lvl = map["connection_lvl"];
      output.port = std::stoul(map["port"]);
      output.success = true;
    } catch (const std::exception &e) {
      throw CommandLineIssue(ERS_HERE, e.what());
    }

    return output;
  }
  /**
   * @brief Maps all arguments into key-value pairs.
   * @param argc number of arguments
   * @param argv arguments.
   * @return Map containing argument pairs.
   */
  static std::map<std::string, std::string> map(int argc, char **argv) {
    std::map<std::string, std::string> map;
    std::string flag;
    for (int i = 0; i < argc; ++i) {
      std::string str = argv[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (str.substr(0, 2) == "--") {
        flag = str.substr(2, std::string::npos);
        map[flag] = "";
      } else if (!flag.empty()) {
        map[flag] = argv[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        flag.erase();
      }
    }
    if (map.find("core_lvl") == map.end()) {
      map["core_lvl"] = "DEBUG";
    }
    if (map.find("module_lvl") == map.end()) {
      map["module_lvl"] = "DEBUG";
    }
    if (map.find("connection_lvl") == map.end()) {
      map["connection_lvl"] = "DEBUG";
    }
    return map;
  }

  std::string name;
  std::string core_lvl;
  std::string module_lvl;
  std::string connection_lvl;
  unsigned port{0};
  bool success{false};
};
} // namespace utilities
} // namespace daqling
