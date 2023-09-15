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

#include <thread>

#include "Command.hpp"
#include "Configuration.hpp"
#include "ConnectionLoader.hpp"
#include "ConnectionManager.hpp"
#include "ModuleManager.hpp"

using namespace daqling::core;
using namespace std::chrono_literals;

class status : public xmlrpc_c::method {
public:
  status() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    auto &plugin = daqling::core::ModuleManager::instance();
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    try {
      // auto &command = daqling::core::Command::instance();
      auto resVec = plugin.getIndividualStates();
      std::vector<xmlrpc_c::value> rpc_vec;
      rpc_vec.reserve(resVec.size());
      for (const auto &item : resVec) {
        rpc_vec.push_back(xmlrpc_c::value_string(item));
      }
      *retvalP = xmlrpc_c::value_array(rpc_vec);
    } catch (ers::Issue &i) {
      ers::error(CommandIssue(ERS_HERE, i));
      std::string error_response = "Failure: " + std::string(i.message());
      *retvalP = xmlrpc_c::value_string(error_response);
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
  };
};

class configure : public xmlrpc_c::method {
public:
  configure() = default;
  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    std::string argument = paramList.getString(0);
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 1, paramList.size());
    auto &cm = daqling::core::ConnectionManager::instance();
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      if (plugin.getStatesAsString() != "booted") {
        throw InvalidCommand(ERS_HERE);
      }
      auto &cfg = Configuration::instance();
      auto &rf = ResourceFactory::instance();
      cfg.load(argument);
      ERS_DEBUG(0, "Get config: " << argument);

      // Add all local resources.
      for (const auto &resourceConfig : cfg.getResources()) {
        rf.createResource(resourceConfig);
      }
      for (auto moduleConfigs : cfg.getModules()) {
        ERS_DEBUG(0, "module configs: " << moduleConfigs);
        auto type = moduleConfigs.at("type").get<std::string>();
        auto name = moduleConfigs.at("name").get<std::string>();
        ERS_DEBUG(0, "Loading type: " << type);
        try {
          plugin.load(name, type);

        } catch (ers::Issue &i) {
          throw CannotLoadPlugin(ERS_HERE, type.c_str(), i);
        }
        auto rcvs = moduleConfigs["connections"]["receivers"];
        ERS_DEBUG(0, "receivers empty " << rcvs.empty());
        for (auto &it : rcvs) {
          ERS_DEBUG(0, "key" << it);
          try {
            cm.addReceiverChannel(name, it);
          } catch (ers::Issue &i) {
            // couldn't add receiver issue
            throw AddChannelFailed(ERS_HERE, it["chid"].get<uint>(), i);
          }
        }
        auto sndrs = moduleConfigs["connections"]["senders"];
        ERS_DEBUG(0, "senders empty " << sndrs.empty());
        for (auto &it : sndrs) {
          ERS_DEBUG(0, "key" << it);
          try {
            cm.addSenderChannel(name, it);
          } catch (ers::Issue &i) {
            // couldn't add receiver issue
            throw AddChannelFailed(ERS_HERE, it["chid"].get<uint>(), i);
          }
        }
      }

      // only apply command to modules in a state that allows the command.
      auto modules_affected = plugin.getModulesEligibleForCommand(types_affected, "booted");
      plugin.configure(modules_affected);
      response = "Success";
    } catch (ers::Issue &i) {
      ers::error(CommandIssue(ERS_HERE, i));
      response = "Failure: " + std::string(i.message());
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

class unconfigure : public xmlrpc_c::method {
public:
  unconfigure() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      auto modules_affected = plugin.getModulesEligibleForCommand(types_affected, "ready");
      if (modules_affected.empty()) {
        throw InvalidCommand(ERS_HERE);
      }
      plugin.unconfigure(modules_affected);
      plugin.unload(modules_affected);
      response = "Success";
    } catch (ers::Issue &i) {
      response = "Failure: " + std::string(i.message());
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

class start : public xmlrpc_c::method {
public:
  start() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    const auto run_num = static_cast<unsigned>(paramList.getInt(0));
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 1, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      // only apply command to modules in a state that allows the command.
      auto modules_affected = plugin.getModulesEligibleForCommand(
          types_affected, std::unordered_set<std::string>{"ready", "running"});
      if (modules_affected.empty()) {
        throw InvalidCommand(ERS_HERE);
      }
      plugin.start(run_num, modules_affected);
      response = "Success";
    } catch (ers::Issue &i) {
      response = "Failure: " + std::string(i.message());
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

class stop : public xmlrpc_c::method {
public:
  stop() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      // only apply command to modules in a state that allows the command.
      auto modules_affected = plugin.getModulesEligibleForCommand(types_affected, "running");
      if (modules_affected.empty()) {
        throw InvalidCommand(ERS_HERE);
      }
      plugin.stop(modules_affected);
      response = "Success";
    } catch (ers::Issue &i) {
      response = "Failure";
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

class down : public xmlrpc_c::method {
public:
  down() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    auto &command = daqling::core::Command::instance();
    try {
      // only apply command to modules in a state that allows the command.
      auto modules_affected = plugin.getModulesEligibleForCommand(
          types_affected, std::unordered_set<std::string>{"booted", "ready", "running"});
      if (modules_affected.empty() && plugin.getStatesAsString() != "booted") {
        throw InvalidCommand(ERS_HERE);
      }
      auto modules_to_stop = plugin.getModulesEligibleForCommand(types_affected, "running");
      if (!modules_to_stop.empty()) {
        plugin.stop(modules_to_stop);
      }
      auto modules_to_unconfigure = plugin.getModulesEligibleForCommand(types_affected, "ready");
      if (!modules_to_unconfigure.empty()) {
        plugin.unconfigure(modules_to_unconfigure);
      }
      if (plugin.getStatesAsString() == "booted") {
        command.stop_and_notify();
      }
      response = "Success";
    } catch (ers::Issue &i) {
      response = "Failure: " + std::string(i.message());
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

class custom : public xmlrpc_c::method {
public:
  custom() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    std::string response;
    const std::string command_name = paramList.getString(0);
    const std::string argument = paramList.getString(1);
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 2, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    // get modules in types_affected, which has command.
    // apply command to these modules only.
    try {
      auto modules_affected = plugin.CommandRegistered(command_name, types_affected);
      if (!modules_affected.empty()) {
        plugin.command(command_name, argument, modules_affected);
        response = "Success";
      } else {
        throw UnregisteredCommand(ERS_HERE, command_name.c_str());
      }
    } catch (ers::Issue &i) {
      response = "Failure: " + std::string(i.message());
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_string(response);
  };
};

/* 
*  Here commands called from python are registered to xmlrpc server.
*  Each command class (status, down, ...) will eventually call corresponding DAQProcess's
*  function via daqling::core::ModuleManager proxy. Only custom commands require registration
*  in DAQling by calling DAQProcess::registerCommand. Any registration is avoided if new
*  inheritor from xmlrpc_c::method class is created and corresponding functions are added to
*  daqling::core::ModuleManager, daqling::core::ModuleLoader and daqling::core::DAQProcess.
*  
*  If one wants to get some data returned from xmlrpc calls, then new xmlrpc_c::method class
*  must be created.
*  TODO: change return of custom command to bytestream?
*/

void daqling::core::Command::setupServer(unsigned port) {
  try {
    m_method_pointers.insert(std::make_pair("status", new status));
    m_method_pointers.insert(std::make_pair("down", new down));
    m_method_pointers.insert(std::make_pair("configure", new configure));
    m_method_pointers.insert(std::make_pair("unconfigure", new unconfigure));
    m_method_pointers.insert(std::make_pair("start", new start));
    m_method_pointers.insert(std::make_pair("stop", new stop));
    m_method_pointers.insert(std::make_pair("custom", new custom));
    for (auto const & [ name, pointer ] : m_method_pointers) {
      m_registry.addMethod(name, pointer);
    }

    m_server_p = std::make_unique<xmlrpc_c::serverAbyss>(
        xmlrpc_c::serverAbyss::constrOpt().registryP(&m_registry).portNumber(port));
    m_cmd_handler = std::thread([&]() { m_server_p->run(); });
    ERS_INFO("Server set up on port: " << port);
    auto &plugin = daqling::core::ModuleManager::instance();
    plugin.AddModules();
  } catch (ers::Issue &i) {
    ers::error(CommandIssue(ERS_HERE, i));
  } catch (std::exception const &e) {
    ers::fatal(UnknownException(ERS_HERE, e.what()));
  }
}
