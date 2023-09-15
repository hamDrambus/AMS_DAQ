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
 * @file ModuleLoader.hpp
 * @brief Loads module (shared object) of type DAQProcess
 */

#ifndef DAQLING_CORE_PLUGINMANAGER_HPP
#define DAQLING_CORE_PLUGINMANAGER_HPP

#include "DAQProcess.hpp"
#include "Utils/Ers.hpp"
#include <dlfcn.h>
#include <string>

namespace daqling {
#include "DynamicLinkIssues.hpp"
namespace core {

class ModuleLoader {
private:
  using CreateFunc = DAQProcess *(std::string);
  using DeleteFunc = void(DAQProcess *);

  CreateFunc *m_create;
  DeleteFunc *m_delete;

  std::optional<DAQProcess *> m_dp;
  std::optional<void *> m_handle;
  bool m_loaded;

  /**
   * Resolves a symbol from the loaded shared module.
   * Throws std::runtime_error if the symbol cannot be resolved.
   */
  template <typename FuncSig> FuncSig *resolve(const char *symbol) {
    ERS_PRECONDITION(*m_handle != nullptr);
    dlerror(); // discard any previous errors

    char *error;
    void *handle = dlsym(*m_handle, symbol);
    error = dlerror();
    if (error) {
      throw CannotResolveModule(ERS_HERE, symbol, error);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<FuncSig *>(handle);
  }

public:
  ModuleLoader();
  ~ModuleLoader();
  ModuleLoader(ModuleLoader const &) = delete;            // Copy construct
  ModuleLoader(ModuleLoader &&) = delete;                 // Move construct
  ModuleLoader &operator=(ModuleLoader const &) = delete; // Copy assign
  ModuleLoader &operator=(ModuleLoader &&) = delete;      // Move assign
  /**
   * Tries to load a module of name `name`, with number 'no'.
   * Returns whether the operation succeeded.
   */
  bool load(const std::string &name, const std::string &type);

  /**
   * Unloads the loaded module.
   * Returns whether the operation succeeded.
   */
  bool unload();

  /**
   * Configures the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void configure() {
    ERS_PRECONDITION(m_loaded);
    m_dp.value()->configure();
  };

  /**
   * Starts the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void start(unsigned run_num) {
    ERS_PRECONDITION(m_loaded);
    m_dp.value()->start(run_num);
  };

  /**
   * Stops the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void stop() {
    ERS_PRECONDITION(m_loaded);
    m_dp.value()->stop();
  };
  /**
   * Unconfigs the loaded module.
   *
   * @warning May only be called after a successful `load`.
   */
  void unconfigure() {
    if (m_loaded) {
      m_dp.value()->unconfigure();
    }
  };

  /**
   * Returns whether specified command was executed.
   */

  bool isCommandRegistered(const std::string &key) {
    return m_dp.value()->isCommandRegistered(key);
  }

  /**
   * Executes a custom module command `cmd`.
   */
  void command(const std::string &cmd, const std::string &arg) { m_dp.value()->command(cmd, arg); }

  std::string getCommandTransitionState(const std::string &key) {
    return m_dp.value()->getCommandTransitionState(key);
  }

  std::string getCommandTargetState(const std::string &key) {
    return m_dp.value()->getCommandTargetState(key);
  }

  /**
   * Returns whether a module is loaded.
   */
  bool getLoaded() { return m_loaded; }
};

} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_PLUGINMANAGER_HPP
