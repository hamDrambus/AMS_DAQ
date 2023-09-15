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
class LocalReceiver : public daqling::core::Receiver {
public:
  LocalReceiver(uint chid, const nlohmann::json &j = NULL);
  bool receive(DataTypeWrapper &bin) override;
  bool sleep_receive(DataTypeWrapper &bin) override;
  void set_sleep_duration(uint ms) override;
  std::atomic<size_t> &getPcqSize() override {
    m_pcq_size = m_queue->sizeGuess();
    return m_pcq_size;
  }

protected:
  std::shared_ptr<daqling::core::Queue> m_queue;
};
} // namespace connection
} // namespace daqling