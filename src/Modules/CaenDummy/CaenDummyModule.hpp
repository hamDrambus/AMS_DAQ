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
#include <chrono>
#include "Core/DAQProcess.hpp"
#include "Common/CaenOutputFormat.hpp"

namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(module, MemoryAllocationFailure,
                  "Failed to allocate memory! There is likely a memory leak", ERS_EMPTY)
ERS_DECLARE_ISSUE(module, UnexpectedFailure,
                  "Unexpected error was thrown: " << eWhat, ((const char *)eWhat))
ERS_DECLARE_ISSUE(module, EventLimitReached,
                  "Reached maximum event number! Stopping acquisition", ERS_EMPTY)
}
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class CaenDummyModule : public daqling::core::DAQProcess {
  void pause();
  void resume();

public:
  typedef caen_output_data<uint16_t> EventType;

  CaenDummyModule(const std::string & /*n*/);
  ~CaenDummyModule() override = default;

  void configure() override; // optional (configuration can be handled in the constructor)
  void start(unsigned run_num) override;
  void stop() override;
  void runner() noexcept override;

private:
  uint16_t m_adc_max_amplitude; // max adc amplitude in channels. Minimal amplitude is 0
  uint16_t m_n_samples_min; // Of course, in parctice the number of sampled points does not change from event to event
  uint16_t m_n_samples_max;

  // Parameters of simulated gaus signal + background
  uint16_t m_baseline_amplitude;
  uint16_t m_noise_amplitude;
  double m_signal_probability;
  uint16_t m_signal_amplitude;
  uint16_t m_signal_amplitude_jitter;
  uint16_t m_signal_duration;
  uint16_t m_signal_duration_jitter;
  uint16_t m_signal_position;
  uint16_t m_signal_position_jitter;

  std::chrono::microseconds m_delay_us{};
  bool m_pause;

  struct State {
    unsigned prev_run = 0;
    uint32_t event_number = 0;
  } m_state;

  std::shared_ptr<EventType> m_event_data;

  template<typename UniformRandomNumberGenerator>
  void generate_signal(UniformRandomNumberGenerator & urng);
};
