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

#include "MetricsSimulatorModule.hpp"
#include "Core/Statistics.hpp"
#include "Utils/Ers.hpp"

MetricsSimulatorModule::MetricsSimulatorModule(const std::string &n) : DAQProcess(n) {
  ERS_INFO("");
}

void MetricsSimulatorModule::start(unsigned run_num) {
  daqling::core::DAQProcess::start(run_num);
  m_metric1 = 0;
  m_metric2 = 0;
  m_metric3 = 0;
  m_metric4 = false;
  m_metric5 = 0;
  m_metric6 = 0;
  m_metric7 = 0;
  m_metric8 = 0;
  m_metric9 = false;
  m_metric10 = 0;
  m_statistics->registerMetric<std::atomic<int>>(&m_metric6, "RandomMetric1-int",
                                                 daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<float>>(&m_metric2, "RandomMetric2-float",
                                                   daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<double>>(&m_metric3, "RandomMetric3-dobule",
                                                    daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<bool>>(&m_metric4, "RandomMetric4-bool",
                                                  daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<size_t>>(&m_metric5, "RandomMetric5-size_t",
                                                    daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<int>>(&m_metric6, "RandomMetric5-int_average",
                                                 daqling::core::metrics::AVERAGE);
  m_statistics->registerMetric<std::atomic<float>>(&m_metric2, "RandomMetric7-float_rate",
                                                   daqling::core::metrics::RATE);
  m_statistics->registerMetric<std::atomic<double>>(&m_metric3, "RandomMetric8-double_average",
                                                    daqling::core::metrics::AVERAGE);
  m_statistics->registerMetric<std::atomic<bool>>(&m_metric9, "RandomMetric9-bool",
                                                  daqling::core::metrics::LAST_VALUE);
  m_statistics->registerMetric<std::atomic<size_t>>(&m_metric5, "RandomMetric10-size_t_rate",
                                                    daqling::core::metrics::RATE);
  ERS_INFO("");
}

void MetricsSimulatorModule::stop() {
  daqling::core::DAQProcess::stop();
  ERS_INFO("");
}

void MetricsSimulatorModule::runner() noexcept {
  ERS_INFO("Running...");
  // downcast is fine here, but this RNG should be modified to use std random.
  srand(static_cast<unsigned int>(time(nullptr)));
  while (m_run) {
    m_metric1 += 4;
    m_metric6 = rand() % 10 + 1;
    m_metric2 = 100.5 * static_cast<float>(rand()) / RAND_MAX;
    m_metric7 = static_cast<float>(rand()) / RAND_MAX;
    m_metric3 = 0.5 * static_cast<double>(rand()) / RAND_MAX;
    m_metric8 = 10. * static_cast<double>(rand()) / RAND_MAX;
    if (m_metric4) {
      m_metric4 = false;
    } else {
      m_metric4 = true;
    }
    if (m_metric9) {
      m_metric9 = false;
    } else {
      m_metric9 = true;
    }
    m_metric5 += 50;
    m_metric10 = static_cast<uint64_t>(rand() % 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  ERS_INFO("Runner stopped");
}
