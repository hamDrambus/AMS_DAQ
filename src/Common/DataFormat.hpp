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
#include <cstddef>
#include <cstdint>
#include <cstring>
struct header_t {
  uint16_t payload_size;
  uint16_t source_id;
  uint32_t seq_number;
  uint64_t timestamp;
} __attribute__((__packed__));

struct data_t {
  header_t header{};
  char payload[24000]{};
  inline size_t size() { return sizeof(header) + header.payload_size; }
  inline void *data() { return this; }
  data_t() = default;
  data_t(const void *data, const size_t size) { memcpy(this, data, size); }
  // const void* data() const{return this;};
} __attribute__((__packed__));

static_assert(std::is_trivially_copyable_v<data_t> == true);

#include "DataType.hpp"
