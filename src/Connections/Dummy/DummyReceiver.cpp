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

#include "DummyReceiver.hpp"
#include "Utils/ConnectionMacros.hpp"
#include "Utils/Ers.hpp"

using namespace daqling::connection;

REGISTER_RECEIVER(DummyReceiver)
DummyReceiver::DummyReceiver(uint chid, const nlohmann::json &j) : daqling::core::Receiver(chid) {
  if (j.empty()) {
  }
}
bool DummyReceiver::receive(DataTypeWrapper &bin) {
  bin.getDataTypePtr()->detach();
  ERS_DEBUG(0, "Hello from DummyReceiver protocolHandler");
  return true;
}