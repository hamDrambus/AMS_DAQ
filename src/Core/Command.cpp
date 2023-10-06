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

// TODO: add checks that nlohmann::json result is not empty when sending it to retvalP

class status : public xmlrpc_c::method {
public:
  status() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    auto &plugin = daqling::core::ModuleManager::instance();
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    try {
      nlohmann::json result = plugin.getIndividualStates();
      result["status"] = "Success";
      *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
    } catch (ers::Issue &i) {
      ers::error(CommandIssue(ERS_HERE, i));
      nlohmann::json result = {{"status", "Error"}, {"response", std::string(i.message())}};
      *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
    } catch (std::exception const &e) {
      nlohmann::json result = {{"status", "FatalError"}, {"response", std::string(e.what())}};
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
  };
};

class configure : public xmlrpc_c::method {
public:
  configure() = default;
  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
    std::string argument = paramList.getString(0);
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 1, paramList.size());
    auto &cm = daqling::core::ConnectionManager::instance();
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      // TODO: remove this condition. Each module should decide itself whether it is configurable
      if (plugin.getStatesAsString() != "booted") {
        throw InvalidState(ERS_HERE, plugin.getStatesAsString(), std::string("configure"));
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
      result = plugin.configure(modules_affected);
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
  };
};

class unconfigure : public xmlrpc_c::method {
public:
  unconfigure() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      auto modules_affected = plugin.getModulesEligibleForCommand(types_affected, "ready");
      if (modules_affected.empty()) {
        throw InvalidCommand(ERS_HERE);
      }
      nlohmann::json result1 = plugin.unconfigure(modules_affected);
      nlohmann::json result2 = plugin.unload(modules_affected);
      result = Command::mergeCommandResponses(result1, result2);
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
  };
};

class start : public xmlrpc_c::method {
public:
  start() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
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
      result = plugin.start(run_num, modules_affected);
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
  };
};

class stop : public xmlrpc_c::method {
public:
  stop() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    try {
      // only apply command to modules in a state that allows the command.
      auto modules_affected = plugin.getModulesEligibleForCommand(types_affected, "running");
      if (modules_affected.empty()) {
        throw InvalidCommand(ERS_HERE);
      }
      result = plugin.stop(modules_affected);
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
  };
};

class down : public xmlrpc_c::method {
public:
  down() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
    std::unordered_set<std::string> types_affected =
        daqling::core::Command::paramListToUnordered_set(paramList, 0, paramList.size());
    auto &plugin = daqling::core::ModuleManager::instance();
    auto &command = daqling::core::Command::instance();
    try {
      // only apply command to modules in a state that allows the command.
      // TODO: remove this condition? Each module should decide itself whether it accepts given command.
      auto modules_affected = plugin.getModulesEligibleForCommand(
          types_affected, std::unordered_set<std::string>{"booted", "ready", "running"});
      if (modules_affected.empty() && plugin.getStatesAsString() != "booted") {
        throw InvalidState(ERS_HERE, plugin.getStatesAsString(), std::string("down"));
      }
      auto modules_to_stop = plugin.getModulesEligibleForCommand(types_affected, "running");
      nlohmann::json res1, res2;
      if (!modules_to_stop.empty()) {
        res1 = plugin.stop(modules_to_stop);
      }
      auto modules_to_unconfigure = plugin.getModulesEligibleForCommand(types_affected, "ready");
      if (!modules_to_unconfigure.empty()) {
        res2 = plugin.unconfigure(modules_to_unconfigure);
      }
      if (plugin.getStatesAsString() == "booted") {
        command.stop_and_notify();
        // TODO: handle else condition
        // TODO: stop_and_notify() should also return json
      }
      result = Command::mergeCommandResponses(res1, res2);
      if (result.empty()) {
        result["status"] = "Success";
        result["response"] = "";
        for (auto mod : modules_to_stop) {
          result["modules"][mod]["status"] = "Success";
          result["modules"][mod]["response"] = "";
        }
      }
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
  };
};

class custom : public xmlrpc_c::method {
public:
  custom() = default;

  void execute(xmlrpc_c::paramList const &paramList, xmlrpc_c::value *const retvalP) override {
    nlohmann::json result;
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
        result = plugin.command(command_name, argument, modules_affected);
      } else {
        throw UnregisteredCommand(ERS_HERE, command_name.c_str());
      }
    } catch (ers::Issue &i) {
      result["status"] = "Error";
      result["response"] = i.message();
      ers::error(CommandIssue(ERS_HERE, i));
    } catch (std::exception const &e) {
      result["status"] = "FatalError";
      result["response"] = e.what();
      ers::fatal(UnknownException(ERS_HERE, e.what()));
    }
    *retvalP = xmlrpc_c::value_bytestring(nlohmann::json::to_bson(result));
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
*  Each command returns json file (in bson binary format) consisting of overall execution
*  status ("Success", "Error" or "FatalError"), module manager's response and detailed
*  per-module execution status with thier individual responce. Response format depends
*  on the executed command. The resulting file will look something like this:
{
  "status": "Success",
  "response": "Manager's response as string",
  "modules": [
    {
      "modulename1" : {
        "status": "Success",
        "response": "Module string response"
      }
    },
    {
      "modulename2" : {
        "status": "Success",
        "response": {
          "field1" : "Some complex response",
          "field2" : 0.502,
          ...
        }
      }
    }
  ]
}
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

nlohmann::json daqling::core::Command::mergeCommandResponses (const nlohmann::json &resp1, const nlohmann::json &resp2) {
  nlohmann::json ret;
  if (resp1.empty()) {
    ret = resp2;
    return ret;
  }
  if (resp2.empty()) {
    ret = resp1;
    return ret;
  }
  try {
    // returns 0 if equal, 1 if the first argument is greater (more important) then the second one
    // and -1 otherwise
    auto compare_by_status = [](const nlohmann::json &resp1, const nlohmann::json &resp2) {
      if (!resp1.contains("status") && !resp2.contains("status"))
        return 0;
      if (!resp2.contains("status"))
        return 1;
      if (!resp1.contains("status"))
        return -1;
      if (resp1["status"] == resp2["status"])
        return 0;
      if (resp1["status"] == "FatalError")
        return 1;
      if (resp2["status"] == "FatalError")
        return -1;
      if (resp1["status"] == "Error")
        return 1;
      if (resp2["status"] == "Error")
        return -1;
      return 2;
    };

    // Handle module manager status and response
    int relation = compare_by_status(resp1, resp2);
    switch (relation) {
      case 0: {
        ret["status"] = resp2["status"];
        if (!resp1.contains("response") || resp1["response"] == "") {
          ret["response"] = resp2["response"];
        } else if (!resp2.contains("response") || resp2["response"] == "") {
          ret["response"] = resp1["response"];
        } else {
          ret["status"] = "Error";
          ret["response"] = "Could not merge command responces.";
        }
        break;
      }
      case -1: {
        ret["status"] = resp2["status"];
        ret["response"] = resp2["response"];
        break;
      }
      case 1: {
        ret["status"] = resp1["status"];
        ret["response"] = resp1["response"];
        break;
      }
      default: {
        ret["status"] = "Error";
        ret["response"] = "Merging error during handling command responces by their importance.";
      }
    }

    // Handle statuses and responses of each module
    if (!resp1.contains("modules")) {
      ret["modules"] = resp2["modules"];
      return ret;
    }
    if (!resp2.contains("modules")) {
      ret["modules"] = resp1["modules"];
      return ret;
    }
    bool modules_error = false;
    ret["modules"] = resp1["modules"];
    for (auto& [key, value] : resp2["modules"].items()) {
      if (ret["modules"].contains(key)) {
        int relation = compare_by_status(ret["modules"][key], value);
        switch (relation) {
          case 0: {
            if (!ret["modules"][key].contains("response") || ret["modules"][key]["response"] == "") {
              ret["modules"][key]["response"] = value["response"];
            } else if (!value.contains("response") || value["response"] == "") {
              break;
            } else {
              modules_error = true;
              ret["modules"][key]["status"] = "Error";
              ret["modules"][key]["response"] = "Could not merge command responces.";
            }
            break;
          }
          case -1: {
            ret["modules"][key] = value;
            break;
          }
          case 1: {
            break;
          }
          default: {
            modules_error = true;
            ret["modules"][key]["status"] = "Error";
            ret["modules"][key]["response"] = "Merging error during handling command responces by their importance.";
          }
        }
      } else {
        ret["modules"][key] = value;
      }
    }
    if (modules_error) {
      ret["status"] = "Error";
      ret["response"] = "Error during merging modules responses.";
    }
  } catch (std::exception const &e) {
    ret["status"] = "FatalError";
    ret["response"] = e.what();
  }
  return ret;
}
