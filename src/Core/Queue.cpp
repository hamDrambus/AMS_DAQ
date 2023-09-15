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

#include "Queue.hpp"
#include <chrono>
#include <ctime>
#include <thread>

using namespace daqling::core;
using namespace std::chrono_literals;

bool Queue::sleep_read(DataTypeWrapper &bin) {
  if (read(bin)) {
    return true;
  }
  std::this_thread::sleep_for((std::chrono::milliseconds(m_read_sleep_duration)));
  return false;
}

bool Queue::sleep_write(DataTypeWrapper &bin) {
  if (write(bin)) {
    return true;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(m_write_sleep_duration));
  return false;
}
void Queue::set_sleep_duration(uint ms) { m_write_sleep_duration = m_read_sleep_duration = ms; }
