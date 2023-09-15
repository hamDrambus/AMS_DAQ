/**
 * Copyright (C) 2021 CERN
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

ERS_DECLARE_ISSUE(module, ErsTestIssue, "ERSTestIssue Here with msg: " << baseMsg,
                  ((const char *)baseMsg))

ERS_DECLARE_ISSUE_BASE(module, ErsTestInherittedIssue, module::ErsTestIssue,
                       "ERSTestInherritedIssue Here with msg: " << inhMsg, ((const char *)baseMsg),
                       ((const char *)inhMsg))
}

class ErsTestModule : public daqling::core::DAQProcess {

public:
  ErsTestModule(const std::string & /*n*/);

  void configure() override; // optional (configuration can be handled in the constructor)
  void start(unsigned run_num) override;
  void stop() override;

  void runner() noexcept override;
};
