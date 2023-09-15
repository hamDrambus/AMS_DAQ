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

#include "Core/DAQProcess.hpp"

class MetricsSimulatorModule : public daqling::core::DAQProcess {
public:
  MetricsSimulatorModule(const std::string & /*n*/);

  void start(unsigned run_num) override;
  void stop() override;

  void runner() noexcept override;

protected:
  std::atomic<int> m_metric1{};
  std::atomic<float> m_metric2{};
  std::atomic<double> m_metric3{};
  std::atomic<bool> m_metric4{};
  std::atomic<size_t> m_metric5{};
  std::atomic<int> m_metric6{};
  std::atomic<float> m_metric7{};
  std::atomic<double> m_metric8{};
  std::atomic<bool> m_metric9{};
  std::atomic<size_t> m_metric10{};
};
