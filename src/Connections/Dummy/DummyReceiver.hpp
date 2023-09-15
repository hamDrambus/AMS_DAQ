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
#include "Core/Receiver.hpp"
#include "nlohmann/json.hpp"

namespace daqling {
namespace connection {
class DummyReceiver : public daqling::core::Receiver {
public:
  DummyReceiver(uint chid, const nlohmann::json &j = NULL);

protected:
  bool receive(DataTypeWrapper &bin) override;
};
} // namespace connection
} // namespace daqling