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

namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(module, BrokenSequenceNumber,
                  "Sequence number for channel " << ch << " is broken! Previous = " << prev_seq
                                                 << " while current = " << seq_number,
                  ((unsigned)ch)((unsigned)prev_seq)((unsigned)seq_number))
}

class EventBuilderModule : public daqling::core::DAQProcess {

  unsigned m_nreceivers;
  std::atomic<size_t> m_eventmap_size;
  std::atomic<size_t> m_complete_ev_size_guess;

public:
  EventBuilderModule(const std::string & /*n*/);

  void configure() override;
  void start(unsigned run_num) override;
  void stop() override;

  void runner() noexcept override;
};
