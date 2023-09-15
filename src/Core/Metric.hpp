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

#ifndef METRIC_HPP
#define METRIC_HPP

#include "Utils/Ers.hpp"
#include <chrono>
#include <string>

namespace daqling {
namespace core {
namespace metrics {
enum metric_type { LAST_VALUE, ACCUMULATE, AVERAGE, RATE };
enum variable_type { FLOAT, INT, DOUBLE, BOOL, SIZE };

} // namespace metrics

class Metric_base { // NOLINT(cppcoreguidelines-special-member-functions)  virtual destructor
  friend class Statistics;

public:
  Metric_base(std::string name, metrics::metric_type mtype, metrics::variable_type vtype)
      : m_name(std::move(name)), m_mtype(mtype), m_vtype(vtype) {
    m_timestamp = std::chrono::system_clock::now();
  }
  virtual ~Metric_base() = default;

protected:
  std::string m_name;
  std::chrono::time_point<std::chrono::system_clock> m_timestamp; // timestamp of last measurement
  metrics::metric_type m_mtype;                                   // defined metric types:
  metrics::variable_type m_vtype;
};

template <class T, class U> class Metric : public Metric_base {
  friend class Statistics;

public:
  Metric(T *pointer, std::string name, metrics::metric_type mtype, metrics::variable_type vtype)
      : Metric_base(name, mtype, vtype), m_metrics_ptr(pointer) {}

protected:
  std::vector<U> m_values;
  T *m_metrics_ptr;
};

} // namespace core
} // namespace daqling

#endif // METRIC_HPP
