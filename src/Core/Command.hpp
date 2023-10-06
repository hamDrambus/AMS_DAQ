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

#ifndef DAQLING_CORE_COMMAND_HPP
#define DAQLING_CORE_COMMAND_HPP

#include <optional>
#include "Utils/Ers.hpp"
#include "Utils/ReusableThread.hpp"
#include "Utils/Singleton.hpp"
#include <unordered_set>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include <nlohmann/json.hpp>
namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(core, CommandIssue, "", ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(core, UnknownException, core::CommandIssue,
                       "Command received unknown exception with content: " << what, ERS_EMPTY,
                       ((const char *)what))

ERS_DECLARE_ISSUE_BASE(core, CannotLoadPlugin, core::CommandIssue,
                       "Module load failure: Could not load plugin of type: " << type, ERS_EMPTY,
                       ((const char *)type))

ERS_DECLARE_ISSUE_BASE(core, AddChannelFailed, core::CommandIssue,
                       "Failed to add channel... chid: " << chid, ERS_EMPTY, ((unsigned)chid))

ERS_DECLARE_ISSUE_BASE(core, InvalidCommand, core::CommandIssue, "Invalid Command.", ERS_EMPTY,
                       ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(core, InvalidState, core::CommandIssue, "Invalid state " << state << " for command "<< command, ERS_EMPTY,
                       ((std::string) state) ((std::string) command) )

ERS_DECLARE_ISSUE_BASE(core, UnregisteredCommand, core::CommandIssue,
                       "Unregistered command: " << CommandName, ERS_EMPTY,
                       ((const char *)CommandName))

namespace core {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Command : public daqling::utilities::Singleton<Command> {
public:
  Command() : m_should_stop{false}, m_command{""}, m_argument{""}, m_response{""} {}
  ~Command() {
    m_server_p->terminate();
    m_cmd_handler.join();
  }

  void setupServer(unsigned port);

  bool getShouldStop() { return m_should_stop; }
  std::mutex *getMutex() { return &m_mtx; }
  std::condition_variable *getCondVar() { return &m_cv; }

  void stop_and_notify() {
    ERS_DEBUG(0, "Shutting down...");
    std::lock_guard<std::mutex> lk(m_mtx);
    m_should_stop = true;
    m_cv.notify_one();
  }
  static std::unordered_set<std::string>
  paramListToUnordered_set(xmlrpc_c::paramList const &paramList, unsigned start, unsigned end) {
    std::unordered_set<std::string> types_affected;
    for (unsigned i = start; i < end; i++) {
      types_affected.insert(paramList.getString(i));
    }
    return types_affected;
  }

  // Some commands consist of actually several commands (calls) for module manager.
  // For example, 'unconfigure' consisits of unconfiguring and unloading the module.
  // Each step returns its own json responce. This function merges two responces into a single
  // one which will be returned by XML-RPC (to python). It handles all cases of potential
  // fails. Only commands which have empty response (including in modules) should be merged!
  // If responces are not empty (e.g. both arguments have errors) responce is overwritten to merge error.
  static nlohmann::json mergeCommandResponses (const nlohmann::json &resp1, const nlohmann::json &resp2);

private:
  bool m_should_stop;
  std::string m_command;
  std::string m_argument;
  std::string m_response;
  std::mutex m_mtx;
  std::mutex m_state_mtx;
  std::condition_variable m_cv;
  std::thread m_cmd_handler;
  xmlrpc_c::registry m_registry;
  std::map<std::string, xmlrpc_c::methodPtr> m_method_pointers;
  std::unique_ptr<xmlrpc_c::serverAbyss> m_server_p;
};

} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_COMMAND_HPP
