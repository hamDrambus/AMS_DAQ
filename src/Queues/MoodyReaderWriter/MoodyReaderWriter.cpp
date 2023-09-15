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

#include "MoodyReaderWriter.hpp"
#include "Utils/ConnectionMacros.hpp"

using namespace daqling::queue;
REGISTER_QUEUE(MoodyReaderWriter)
MoodyReaderWriter::MoodyReaderWriter(const nlohmann::json &j)
    : m_queue(j.at("queue_size").get<unsigned int>()) {
  m_capacity = j.at("queue_size").get<unsigned int>();
}
bool MoodyReaderWriter::read(DataTypeWrapper &bin) { return m_queue.try_dequeue(bin); }
// bool MoodyReaderWriter::sleep_read(daqling::utilities::Binary& bin)
// {
//     return m_queue.wait_dequeue_timed(bin,std::chrono::milliseconds(m_read_sleep_duration));
// }
bool MoodyReaderWriter::write(DataTypeWrapper &bin) { return m_queue.emplace(std::move(bin)); }
uint MoodyReaderWriter::sizeGuess() { return m_queue.size_approx(); }
uint MoodyReaderWriter::capacity() { return m_capacity; }