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

#include "CaenDummyModule.hpp"
#include "Common/DataFormat.hpp"
#include "Common/SerializableFormat.hpp"
#include "Utils/Ers.hpp"
#include <random>
#include <utility>

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace daqling::module;

CaenDummyModule::CaenDummyModule(const std::string &n) : DAQProcess(n) {
  ERS_DEBUG(0, "With config: " << getModuleSettings());

  m_adc_max_amplitude = getModuleSettings()["adc_max_amplitude"];
  m_n_samples_min = getModuleSettings()["n_samples"]["min"];
  m_n_samples_max = getModuleSettings()["n_samples"]["max"];

  // Parameters of simulated gaus signal + background
  m_baseline_amplitude = getModuleSettings()["baseline"]["amplitude"];
  m_noise_amplitude = getModuleSettings()["baseline"]["noise_amplitude"];
  m_signal_probability = getModuleSettings()["signal"]["probability"];
  m_signal_amplitude = getModuleSettings()["signal"]["amplitude"];
  m_signal_amplitude_jitter = getModuleSettings()["signal"]["amplitude_jitter"];
  m_signal_duration = getModuleSettings()["signal"]["duration"];
  m_signal_duration_jitter = getModuleSettings()["signal"]["duration_jitter"];
  m_signal_position = getModuleSettings()["signal"]["position"];
  m_signal_position_jitter = getModuleSettings()["signal"]["position_jitter"];

  m_delay_us = std::chrono::microseconds(getModuleSettings()["delay_us"]);;
  m_pause = false;

  registerCommand("pause", "pausing", "paused", &CaenDummyModule::pause, this);
  registerCommand("resume", "resuming", "running", &CaenDummyModule::resume, this);
}

void CaenDummyModule::configure() {
  DAQProcess::configure();
  try {
    m_event_data = std::make_shared<EventType>();
    m_event_data->device = m_name;
  } catch (const std::bad_alloc &) {
    throw MemoryAllocationFailure(ERS_HERE);
  } catch (const std::exception& e) {
    throw UnexpectedFailure(ERS_HERE, e.what());
  }
  std::this_thread::sleep_for(2s); // some sleep to demonstrate transition states
}

void CaenDummyModule::pause() { m_pause = true; }

void CaenDummyModule::resume() { m_pause = false; }

void CaenDummyModule::start(unsigned run_num) { DAQProcess::start(run_num); }

void CaenDummyModule::stop() { DAQProcess::stop(); }

void CaenDummyModule::runner() noexcept {
  if (!m_event_data) {
    ERS_DEBUG(0, "Run abourted due to null data pointer.");
    ers::error(UnexpectedFailure(ERS_HERE, "Run abourted due to null data pointer."));
    return;
  } else {
    m_event_data->event_number = 0;
  }
  microseconds timestamp{};

  std::random_device rd;
  std::mt19937 gen(rd());
  
  ERS_DEBUG(0, "Running...");
  while (m_run) {
    if (m_pause) {
      ERS_INFO("Paused at event number " << m_event_data->event_number);
      while (m_pause && m_run) {
        std::this_thread::sleep_for(10ms);
      }
    }
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    m_event_data->timestamp = static_cast<uint64_t>(timestamp.count());
    try {
      generate_signal(gen);
    } catch (const std::bad_alloc &) {
      ERS_WARNING("Generation of event failed due to failed memory allocation. Skipping.");
      ers::error(MemoryAllocationFailure(ERS_HERE));
    } catch (const std::exception& e) {
      ERS_ERROR("Generation of event failed due unexpected exception. Stopping run.");
      m_run = false;
      ers::error(UnexpectedFailure(ERS_HERE, e.what()));
    }

    ERS_DEBUG(0, "event number " << m_event_data->event_number << " | timestamp " << std::hex << "0x"
                                    << m_event_data->timestamp << std::dec);

    // Passing shared pointers so that resources are persistent and are not re-allocated
    // every time the data is sent
    SharedDataType<EventType> data_massage(m_event_data);
    ERS_INFO("Sending event #" << m_event_data->event_number << " timestamp = " << m_event_data->timestamp);
    m_event_data->serialize();
    ERS_INFO("Serialized event, total size = " << m_event_data->size() << ", n_samples = " << m_event_data->ch_data[0].xs.size());
    while ((!m_connections.sleep_send(0, data_massage)) && m_run) {
      ERS_WARNING("put() failed. Trying again");
    };

    ++m_event_data->event_number;
    if (m_event_data->event_number == UINT32_MAX) {
      m_run = false;
      ers::fatal(EventLimitReached(ERS_HERE));
    }
    std::this_thread::sleep_for(m_delay_us);
  }
  ERS_DEBUG(0, "Runner stopped");
}

template<typename UniformRandomNumberGenerator>
void CaenDummyModule::generate_signal(UniformRandomNumberGenerator & urng) {
  std::uniform_int_distribution<> distr_n_samples(static_cast<int>(m_n_samples_min),
      static_cast<int>(m_n_samples_max));
  std::normal_distribution<> distr_baseline(static_cast<double>(m_baseline_amplitude),
      static_cast<double>(m_noise_amplitude));
  std::bernoulli_distribution distr_signal_present(m_signal_probability);
  std::normal_distribution<> distr_signal_position(static_cast<double>(m_signal_position),
      static_cast<double>(m_signal_position_jitter));
  std::normal_distribution<> distr_signal_amplitude(static_cast<double>(m_signal_amplitude),
      static_cast<double>(m_signal_amplitude_jitter));
  std::normal_distribution<> distr_signal_duration(static_cast<double>(m_signal_duration),
      static_cast<double>(m_signal_duration_jitter));

  const uint16_t n_samples_size = static_cast<uint16_t>(distr_n_samples(urng));
  const bool has_signal = distr_signal_present(urng);
  const double signal_position = distr_signal_position(urng);
  const double signal_duration = distr_signal_duration(urng);
  const double signal_amplitude = distr_signal_amplitude(urng);

  m_event_data->ch_data.resize(2);
  m_event_data->ch_data[0].channel = 0;
  m_event_data->ch_data[0].xs.resize(n_samples_size);
  m_event_data->ch_data[0].ys.resize(n_samples_size);
  m_event_data->ch_data[1].channel = 1;
  m_event_data->ch_data[1].xs.resize(n_samples_size);
  m_event_data->ch_data[1].ys.resize(n_samples_size);
  for (std::size_t i = 0; i!=n_samples_size; ++i) {
    m_event_data->ch_data[0].xs[i] = i;
    m_event_data->ch_data[1].xs[i] = i;

    m_event_data->ch_data[0].ys[i] = distr_baseline(urng) +
        (has_signal ? std::round(signal_amplitude * std::exp(-std::pow((i - signal_position)/signal_duration, 2))) : 0);
    m_event_data->ch_data[1].ys[i] = distr_baseline(urng);
  }
}
