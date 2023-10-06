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

nlohmann::json ModuleManager::unload(std::unordered_set<std::string> modargs) {
  nlohmann::json ret;
  setTag("core");
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          if (mod.m_module_loader->unload()) {
            mod.m_module_loader.release();
            ModuleManager::instance().m_modloaders.erase(key);
            ret["modules"][mod.m_module_name]["status"] = "Success";
            ret["modules"][mod.m_module_name]["response"] = "";
            return true;
          }
          ret["modules"][mod.m_module_name]["status"] = "Error";
          ret["modules"][mod.m_module_name]["response"] = std::string("Failed to unload module ") + key;
          ERS_WARNING("Failed to unload module " << key);
          return false;
        }
        return true;
      },
      modargs);
  if (for_each_module(mod_func)) {
    ret["status"] = "Success";
    ret["response"] = "";
  } else {
    ret["status"] = "Error";
    ret["response"] = "";
  }
  return ret;
}

nlohmann::json ModuleManager::configure(std::unordered_set<std::string> modargs) {
  nlohmann::json ret;
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Configuring modules with name: " << mod);
    }
  } else {
    ret["status"] = "Error";
    ret["response"] = "No modules eligible for configure.";
    ret["modules"] = {};
    ERS_INFO("No modules eligible for configure.");
    return ret;
  }
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "configuring";
          setTag(key);
          mod.m_module_loader->configure(); // TODO: add possibility for failing configure()
          mod.m_module_status = "ready";
          ret["modules"][mod.m_module_name]["status"] = "Success";
          ret["modules"][mod.m_module_name]["response"] = "";
          return true;
        }
        return false;
      },
      modargs);
  for_each_module(mod_func);
  ret["status"] = "Success";
  ret["response"] = "";
  if (!ret.contains("modules")) {
    ret["response"] = "Warning: modules status was not written.";
  }
  return ret;
}

nlohmann::json ModuleManager::start(unsigned run_num, std::unordered_set<std::string> modargs) {
  // change to take list as argument instead
  nlohmann::json ret;
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Starting modules with name: " << mod);
    }
  } else {
    ret["status"] = "Error";
    ret["response"] = "No modules eligible for start.";
    ret["modules"] = {};
    ERS_INFO("No modules eligible for start.");
    return ret;
  }
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs,
         unsigned run_num) {
        auto &cm = daqling::core::ConnectionManager::instance();
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "starting";
          setTag(key);
          if (!cm.start(mod.m_module_name)) {
            // TODO: should return error?
            ERS_WARNING("Could not find submanager");
          }
          mod.m_module_loader->start(run_num); // TODO: add handling of failed start
          mod.m_module_status = "running";
          ret["modules"][mod.m_module_name]["status"] = "Success";
          ret["modules"][mod.m_module_name]["response"] = "";
          return true;
        }
        return false;
      },
      modargs, run_num);
  for_each_module(mod_func);
  ret["status"] = "Success";
  ret["response"] = "";
  return ret;
}

nlohmann::json ModuleManager::stop(std::unordered_set<std::string> modargs) {
  nlohmann::json ret;
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Stopping modules with name: " << mod);
    }
  } else {
    ret["status"] = "Error";
    ret["response"] = "No modules eligible for stop.";
    ret["modules"] = {};
    ERS_INFO("No modules eligible for stop.");
    return ret;
  }
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        if (modargs.find(key) != modargs.end()) {
          auto &cm = daqling::core::ConnectionManager::instance();
          mod.m_module_status = "stopping";
          setTag(key);
          mod.m_module_loader->stop(); // TODO: add handling failed stop
          cm.stop(mod.m_module_name);
          mod.m_module_status = "ready";
          ret["modules"][mod.m_module_name]["status"] = "Success";
          ret["modules"][mod.m_module_name]["response"] = "";
        }
        return true;
      },
      modargs);
  for_each_module(mod_func);
  ret["status"] = "Success";
  ret["response"] = "";
  return ret;
}

nlohmann::json ModuleManager::command(const std::string &cmd, const std::string &arg,
                            std::unordered_set<std::string> modargs) {
  nlohmann::json ret;
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Sending command " << cmd << " for modules with name: " << mod);
    }
  } else {
    ret["status"] = "Error";
    ret["response"] = std::string("No modules eligible for command ") + arg + ".";
    ret["modules"] = {};
    ERS_INFO("No modules eligible for command " << arg << ".");
    return ret;
  }
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs,
         const std::string &cmd, const std::string &arg) {
        if (modargs.find(key) != modargs.end()) {
          ERS_INFO("setting state for " << key << " to "
                                        << mod.m_module_loader->getCommandTransitionState(cmd));
          mod.m_module_status = mod.m_module_loader->getCommandTransitionState(cmd);
          setTag(key);
          mod.m_module_loader->command(cmd, arg); // TODO: handle failed execition
          ERS_INFO("setting state for " << key << " to "
                                        << mod.m_module_loader->getCommandTargetState(cmd));
          mod.m_module_status = mod.m_module_loader->getCommandTargetState(cmd);
          ret["modules"][mod.m_module_name]["status"] = "Success";
          ret["modules"][mod.m_module_name]["response"] = "";
        }
        return true;
      },
      modargs, cmd, arg);
  if (for_each_module(mod_func)) {
    ret["status"] = "Success";
    ret["response"] = "";
  } else {
    ret["status"] = "Error";
    ret["response"] = "";
  }
  return ret;
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

nlohmann::json ModuleManager::unconfigure(std::unordered_set<std::string> modargs) {
  nlohmann::json ret;
  setTag("core");
  if (!modargs.empty()) {
    for (const auto &mod : modargs) {
      ERS_INFO("Unconfiguring modules with name: " << mod);
    }
  } else {
    ret["status"] = "Error";
    ret["response"] = "No modules eligible for configure.";
    ret["modules"] = {};
    ERS_INFO("No modules eligible for unconfigure.");
    return ret;
  }
  auto mod_func = ModuleFunction(
      [&ret](const std::string &key, module_info &mod, std::unordered_set<std::string> modargs) {
        auto &cm = daqling::core::ConnectionManager::instance();
        if (modargs.find(key) != modargs.end()) {
          mod.m_module_status = "unconfiguring";
          setTag(key);
          mod.m_module_loader->unconfigure(); // TODO: add handling of unsuccessful unconfigure()
          cm.removeChannel(mod.m_module_name);
          mod.m_module_status = "booted";
          ret["modules"][mod.m_module_name]["status"] = "Success";
          ret["modules"][mod.m_module_name]["response"] = "";
        }
        return true;
      },
      modargs);
  for_each_module(mod_func);
  ret["status"] = "Success";
  ret["response"] = "";
  return ret;
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
  ret["response"] = getStatesAsString(); // Overall manager state
  for (auto const & [ key, mod ] : m_modloaders) {
    ret["modules"][key]["status"] = "Success";
    ret["modules"][key]["response"] = mod.m_module_status;
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
