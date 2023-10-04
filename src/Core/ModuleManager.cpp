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

/// \cond
#include <algorithm>
#include <chrono>
#include <ctime>
#include <future>
#include <iomanip>
#include <thread>
#include <utility>

/// \endcond

#include "Command.hpp"
#include "ModuleManager.hpp"

using namespace daqling::core;
using namespace std::chrono_literals;
daqling::utilities::ThreadTagger &ModuleManager::m_tagger =
    daqling::utilities::ThreadTagger::instance();
template <typename T> bool ModuleManager::for_each_module(T &mod_func) {
  std::vector<std::future<bool>> vec;
  for (auto &item : m_modloaders) {
    const std::string &key = item.first;
    daqling::core::module_info &mod = item.second;
    vec.push_back(std::async(mod_func, std::ref(key), std::ref(mod)));
  }
  bool retval = true;
  for (auto &item : vec) {
    if (!item.get()) {
      retval = false;
    }
  }
  return retval;
}

ModuleManager::ModuleManager() = default;

ModuleManager::~ModuleManager() { m_modloaders.clear(); }
void ModuleManager::setTag(std::string tag) { m_tagger.writeTag(std::move(tag)); }

bool ModuleManager::load(const std::string &name, const std::string &type) {
  setTag("core");
  bool success = false;
  if (m_modloaders.find(name) == m_modloaders.end()) {
    ERS_INFO("Loading Module " << name << "with type: " << type);
    m_modloaders[name].m_module_loader = std::make_unique<ModuleLoader>();
    m_modloaders[name].m_module_type = type;
    m_modloaders[name].m_module_name = name;
    if (m_modloaders[name].m_module_loader->load(name, type)) {
      m_modloaders[name].m_module_status = "booted";
      success = true;
    } else {
      ERS_ERROR("Failed to load module: " << name); // throw issue instead
      success = false;
    }
  } else {
    ERS_WARNING("Module[" << name << "] is already loaded!");
    success = false;
  }
  return success;
}

bool ModuleManager::unload(std::unordered_set<std::string> modargs) {
  setTag("core");
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          if (mod.m_module_loader->unload()) {
            mod.m_module_loader.release();
            ModuleManager::instance().m_modloaders.erase(key);
            return true;
          }
          return false;
          ERS_WARNING("Failed to unload module " << key);
        }
        return true;
      },
      std::move(modargs));
  return for_each_module(mod_func);
}
void ModuleManager::configure(std::unordered_set<std::string> modargs) {
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Configuring modules with name: " << mod);
    }
  } else {
    ERS_INFO("No modules eligible for configure.");
    return;
  }
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "configuring";
          setTag(key);
          mod.m_module_loader->configure();
          mod.m_module_status = "ready";
          return true;
        }
        return false;
      },
      modargs);
  for_each_module(mod_func);
}

void ModuleManager::start(unsigned run_num, std::unordered_set<std::string> modargs) {
  // change to take list as argument instead
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Starting modules with name: " << mod);
    }
  } else {
    ERS_INFO("No modules eligible for start.");
    return;
  }
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs,
         unsigned run_num) {
        auto &cm = daqling::core::ConnectionManager::instance();
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "starting";
          setTag(key);
          if (!cm.start(mod.m_module_name)) {
            ERS_WARNING("Could not find submanager");
          }
          mod.m_module_loader->start(run_num);
          mod.m_module_status = "running";
          return true;
        }
        return false;
      },
      modargs, run_num);
  for_each_module(mod_func);
}

void ModuleManager::stop(std::unordered_set<std::string> modargs) {
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Stopping modules with name: " << mod);
    }
  } else {
    ERS_INFO("No modules eligible for stop.");
    return;
  }
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          auto &cm = daqling::core::ConnectionManager::instance();
          mod.m_module_status = "stopping";
          setTag(key);
          mod.m_module_loader->stop();
          cm.stop(mod.m_module_name);
          mod.m_module_status = "ready";
        }
        return true;
      },
      modargs);
  for_each_module(mod_func);
}

bool ModuleManager::command(const std::string &cmd, const std::string &arg,
                            std::unordered_set<std::string> modargs) {
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Sending command " << cmd << " for modules with name: " << mod);
    }
  } else {
    ERS_INFO("No modules eligible for command " << arg << ".");
    return false;
  }
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs,
         const std::string &cmd, const std::string &arg) {
        if (modargs.find(key) != modargs.end()) {
          ERS_INFO("setting state for " << key << " to "
                                        << mod.m_module_loader->getCommandTransitionState(cmd));
          mod.m_module_status = mod.m_module_loader->getCommandTransitionState(cmd);
          setTag(key);
          mod.m_module_loader->command(cmd, arg);
          ERS_INFO("setting state for " << key << " to "
                                        << mod.m_module_loader->getCommandTargetState(cmd));
          mod.m_module_status = mod.m_module_loader->getCommandTargetState(cmd);
        }
        return true;
      },
      modargs, cmd, arg);
  return for_each_module(mod_func);
}

std::unordered_set<std::string>
ModuleManager::CommandRegistered(const std::string &com, std::unordered_set<std::string> modargs) {
  std::unordered_set<std::string> ret_set;
  for (auto const & [ key, mod ] : m_modloaders) {
    if (modargs.empty() || modargs.find(mod.m_module_type) != modargs.end()) {
      if (mod.m_module_loader->isCommandRegistered(com)) {
        ret_set.insert(key);
      }
    }
  }
  return ret_set;
}
void ModuleManager::unconfigure(std::unordered_set<std::string> modargs) {
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Unconfiguring modules with name: " << mod);
    }
  } else {
    ERS_INFO("No modules eligible for unconfigure.");
    return;
  }
  auto mod_func = ModuleFunction(
      [](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        auto &cm = daqling::core::ConnectionManager::instance();
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "unconfiguring";
          setTag(key);
          mod.m_module_loader->unconfigure();
          cm.removeChannel(mod.m_module_name);
          mod.m_module_status = "booted";
        }
        return true;
      },
      modargs);
  for_each_module(mod_func);
}
std::string ModuleManager::getState(const std::string &modarg) {
  std::string state = "booted";
  if (m_modloaders.find(modarg) != m_modloaders.end()) {
    state = m_modloaders[modarg].m_module_status;
  }
  return state;
}
std::string ModuleManager::getStatesAsString() {
  std::vector<std::string> vec;
  for (auto const & [ key, mod ] : m_modloaders) {
    vec.push_back(mod.m_module_status);
  }
  std::string state = "booted";
  bool first = true;
  for (const auto &item : vec) {
    if (item != state) {
      if (first) {
        state = item;
        first = false;
      } else {
        state = "inconsistent";
        break;
      }
    }
  }
  return state;
}

nlohmann::json ModuleManager::getIndividualStates() {
  nlohmann::json ret;
  ret["state"] = getStatesAsString(); // Overall manager state
  for (auto const & [ key, mod ] : m_modloaders) {
    ret[key]["state"] = mod.m_module_status;
    // TODO: get status from module itself.
    // TODO: get additional (optional) info such as error message.
  }
  return ret;
}
std::unordered_set<std::string>
ModuleManager::getModulesEligibleForCommand(std::unordered_set<std::string> modargs,
                                            std::unordered_set<std::string> states) {
  std::unordered_set<std::string> res_set;
  for (auto const & [ key, mod ] : m_modloaders) {
    if (states.find(mod.m_module_status) != states.end() &&
        (modargs.empty() || modargs.find(mod.m_module_type) != modargs.end())) {
      res_set.insert(key);
    }
  }
  return res_set;
}
std::unordered_set<std::string>
ModuleManager::getModulesEligibleForCommand(std::unordered_set<std::string> modargs,
                                            const std::string &state) {
  std::unordered_set<std::string> res_set;
  for (auto const & [ key, mod ] : m_modloaders) {
    if (state == mod.m_module_status &&
        (modargs.empty() || modargs.find(mod.m_module_type) != modargs.end())) {
      res_set.insert(key);
    }
  }
  return res_set;
}

void ModuleManager::AddModules() {}
