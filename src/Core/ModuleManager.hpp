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

/**
 * @file ModuleManager.hpp
 * @brief Loads module (shared object) of type DAQProcess
 */

#ifndef DAQLING_CORE_ModuleManager_HPP
#define DAQLING_CORE_ModuleManager_HPP

/// \cond
#include <dlfcn.h>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
/// \endcond
#include <nlohmann/json.hpp>

#include "DAQProcess.hpp"
#include "ModuleLoader.hpp"
#include "Utils/ThreadTagger.hpp"
namespace daqling {
ERS_DECLARE_ISSUE(core, ModuleIssue, "", ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(core, InconsistentStates, core::ModuleIssue,
                       "Module states are inconsistent!", ERS_EMPTY, ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(core, InvalidModuleName, core::ModuleIssue,
                       "Couldn't find module with name: " << name, ERS_EMPTY, ((const char *)name))
namespace core {
struct module_info {
  std::unique_ptr<ModuleLoader> m_module_loader;
  std::string m_module_status = "booted";
  std::string m_module_type;
  std::string m_module_name;
};

template <typename Function, typename... FunctionArgs> class ModuleFunction {
public:
  ModuleFunction(Function func, std::unordered_set<std::string> modargs,
                 FunctionArgs... parameters) {
    m_func = std::bind(func, std::placeholders::_1, std::placeholders::_2, modargs, parameters...);
  }
  bool operator()(const std::string &str, module_info &mod) const { return m_func(str, mod); }

private:
  std::function<bool(const std::string, module_info &)> m_func;
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ModuleManager : public daqling::utilities::Singleton<ModuleManager> {
private:
  std::map<std::string, module_info> m_modloaders;
  template <typename T> bool for_each_module(T & /*mod_func*/);
  // bool m_loaded;
  static daqling::utilities::ThreadTagger &m_tagger;
  static void setTag(std::string tag);

public:
  ModuleManager();
  ~ModuleManager();
  /**
   * Adds module_info structs for every module.
   * Sets module_status to "added", and provides modile_name.
   */
  void AddModules();
  /**
   * Tries to load a module of name `name`, with number 'no'.
   * Returns whether the operation succeeded.
   */

  bool load(const std::string &name, const std::string &type);

  // bool loadAll(const std::vector<std::string>& names);

  /**
   * Unloads the loaded module.
   * Returns whether the operation succeeded.
   */
  bool unload(std::unordered_set<std::string> modargs);

  /**
   * Configures the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void configure(std::unordered_set<std::string> modargs);
  /**
   * Unconfigures the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void unconfigure(std::unordered_set<std::string> modargs);

  /**
   * Starts the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void start(unsigned run_num, std::unordered_set<std::string> modargs);

  /**
   * Stops the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void stop(std::unordered_set<std::string> modargs);

  /**
   * Executes a custom module command `cmd` if registered.
   *
   * Returns whether specified command was executed.
   */
  bool command(const std::string &cmd, const std::string &arg,
               std::unordered_set<std::string> modargs);

  // std::string
  // getState(std::unordered_set<std::string> modargs = std::unordered_set<std::string>());
  std::string getState(const std::string &modarg);
  std::string getStatesAsString();
  nlohmann::json getIndividualStates();
  std::unordered_set<std::string>
  getModulesEligibleForCommand(std::unordered_set<std::string> modargs,
                               std::unordered_set<std::string> states);
  std::unordered_set<std::string>
  getModulesEligibleForCommand(std::unordered_set<std::string> modargs, const std::string &state);
  std::string getCommandTargetState(const std::string &, std::unordered_set<std::string> modarg);

  std::string getCommandTransitionState(const std::string &,
                                        std::unordered_set<std::string> modarg);

  // bool getLoaded() { return m_loaded; }
  std::unordered_set<std::string> CommandRegistered(const std::string & /*com*/,
                                                    std::unordered_set<std::string> modargs);
};

} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_ModuleManager_HPP
