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
#include "Core/Sender.hpp"
#include "nlohmann/json.hpp"

namespace daqling {
namespace connection {
class LocalSender : public daqling::core::Sender {
public:
  LocalSender(uint chid, const nlohmann::json &j = NULL);
  bool send(DataTypeWrapper &bin) override;
  bool sleep_send(DataTypeWrapper &bin) override;
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