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

#include <chrono>
#include <utility>

#include "Common/DataFormat.hpp"
#include "EventBuilderModule.hpp"
#include "Utils/Ers.hpp"
#include "folly/ProducerConsumerQueue.h"

using namespace std::chrono_literals;
using namespace daqling::module;
using Binary = DataFragment<daqling::utilities::Binary>;
EventBuilderModule::EventBuilderModule(const std::string &n)
    : DAQProcess(n), m_eventmap_size{0}, m_complete_ev_size_guess{0} {
  ERS_DEBUG(0, "With config: " << getModuleSettings());
  m_nreceivers = m_config.getNumReceiverConnections(m_name);
}

void EventBuilderModule::configure() {
  DAQProcess::configure();

  if (m_statistics) {
    // Register statistical variables
    m_statistics->registerMetric<std::atomic<size_t>>(&m_eventmap_size, "EventMap-Size",
                                                      daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric<std::atomic<size_t>>(&m_complete_ev_size_guess,
                                                      "CompleteEventQueue-SizeGuess",
                                                      daqling::core::metrics::LAST_VALUE);
  }
}

void EventBuilderModule::start(unsigned run_num) { DAQProcess::start(run_num); }

void EventBuilderModule::stop() { DAQProcess::stop(); }

void EventBuilderModule::runner() noexcept {
  ERS_DEBUG(0, "Running...");

  std::unordered_map<uint32_t, std::vector<Binary>> events;
  folly::ProducerConsumerQueue<unsigned> complete_seq(1000);
  std::mutex mtx;

  std::thread consumer{[&]() {
    addTag();
    while (m_run || !complete_seq.isEmpty()) { // finish to process complete events
      ERS_DEBUG(0, "Consumer thread Running...");
      unsigned seq;
      while (!complete_seq.read(seq) && m_run) {
        std::this_thread::sleep_for(1ms);
      }
      if (!m_run) {
        return;
      }
      Binary out(new daqling::utilities::Binary());
      std::unique_lock<std::mutex> lck(mtx);
      for (auto &c : events[seq]) {
        out += c;
      }
      events.erase(seq);
      if (m_statistics) {
        m_eventmap_size = events.size();
        m_complete_ev_size_guess = complete_seq.sizeGuess();
      }
      lck.unlock();
      while (!m_connections.sleep_send(0, out) && m_run) {
        ERS_WARNING("send() failed. Trying again");
      }
      ERS_DEBUG(0, "Sent msg.");
    }
  }};
  // store previous sequence number per channel
  std::vector<unsigned> prev_seq = {0};
  prev_seq.reserve(m_nreceivers);

  while (m_run) {
    for (unsigned ch = 0; ch < m_nreceivers; ch++) {
      Binary b;
      if (m_connections.sleep_receive(ch, b)) {
        ERS_DEBUG(0, "Received msg.");
        unsigned seq_number;
        auto *d = static_cast<data_t *>(b.data());
        seq_number = d->header.seq_number;
        // check sequence number
        if (prev_seq[ch] + 1 != seq_number && seq_number != 0) {
          ers::fatal(BrokenSequenceNumber(ERS_HERE, ch, prev_seq[ch], seq_number));
        }
        prev_seq[ch] = seq_number;
        std::unique_lock<std::mutex> lck(mtx);
        events[seq_number].push_back(std::move(b));
        if (events[seq_number].size() == m_nreceivers) {
          complete_seq.write(seq_number);
        }
        lck.unlock();
      }
    }
  }
  consumer.join();
  ERS_DEBUG(0, "Runner stopped");
}
