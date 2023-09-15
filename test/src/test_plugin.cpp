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

#include <chrono>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <thread>

#include "Core/DAQProcess.hpp"
#include "Utils/Ers.hpp"

using namespace std::chrono_literals;

using CreateFunc = daqling::core::DAQProcess *();
using DestroyFunc = void(daqling::core::DAQProcess *);

int main(int argc, char **argv) {

  if (argc == 1) {
    ERS_WARNING("No plugin name entered");
    return 1;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  ERS_INFO("Loading " << argv[1]);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::string pluginName = "lib/lib" + std::string(argv[1]) + ".so";
  void *handle = dlopen(pluginName.c_str(), RTLD_NOW);
  if (handle == nullptr) {
    ERS_WARNING("Plugin name not valid");
    return 1;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto create = reinterpret_cast<CreateFunc *>(dlsym(handle, "daqling_module_create"));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto destroy = reinterpret_cast<DestroyFunc *>(dlsym(handle, "daqling_module_create"));

  auto *dp = create();

  destroy(dp);
}
