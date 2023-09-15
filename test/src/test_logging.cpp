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

#include "Core/ConnectionManager.hpp"
#include "Utils/Ers.hpp"

int main(int /*unused*/, char ** /*unused*/) {

  ERS_INFO("WOOF WOOF");
  ERS_WARNING("Ugh!" << 12345 << "bof bof" << '\n');

  ERS_INFO("Testing ConnectionManager.hpp");
  std::ignore = daqling::core::ConnectionManager::instance();

  ERS_WARNING("About to die...");
  return 0;
}
