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
#include "moody/readerwriterqueue.h"
#include "nlohmann/json.hpp"

namespace daqling {
namespace queue {
class MoodyReaderWriter : public daqling::core::Queue {
public:
  MoodyReaderWriter(const nlohmann::json &j);
  bool read(DataTypeWrapper & /*bin*/) override;
  bool write(DataTypeWrapper & /*bin*/) override;
  uint sizeGuess() override;
  uint capacity() override;

protected:
private:
  unsigned int m_capacity;
  moodycamel::ReaderWriterQueue<DataTypeWrapper> m_queue;
};
} // namespace queue
} // namespace daqling