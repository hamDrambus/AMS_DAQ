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

#include "Core/Queue.hpp"
#include "folly/ProducerConsumerQueue.h"
#include "nlohmann/json.hpp"

namespace daqling {
namespace queue {
class FollyProducerConsumer : public daqling::core::Queue {
public:
  // using MessageQueue<T> = folly::ProducerConsumerQueue<T>;
  FollyProducerConsumer(const nlohmann::json &j);
  bool read(DataTypeWrapper & /*bin*/) override;
  bool write(DataTypeWrapper & /*bin*/) override;
  uint sizeGuess() override;
  uint capacity() override;

protected:
private:
  folly::ProducerConsumerQueue<DataTypeWrapper> m_queue;
};
} // namespace queue
} // namespace daqling