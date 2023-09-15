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

#ifndef DAQLING_CORE_CONFIGURATION_HPP
#define DAQLING_CORE_CONFIGURATION_HPP

#include "Utils/Ers.hpp"
#include "Utils/Singleton.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <unordered_set>
#include <utility>
namespace daqling {
ERS_DECLARE_ISSUE(core, NoConfigsForName, "No configs for module with name: " << name,
                  ((const char *)name))
namespace core {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class Configuration : public daqling::utilities::Singleton<Configuration> {
public:
  Configuration() = default;
  ~Configuration() = default;

  void clear() { m_config.clear(); }
  std::string dump() { return m_config.dump(); }
  void load(const std::string &jsonStr) { m_config = nlohmann::json::parse(jsonStr); }
  nlohmann::json &getConfig() { return m_config; }
  nlohmann::json &getModuleSettings(const std::string &key) { return getModule(key)["settings"]; }
  nlohmann::json
  getModules(std::unordered_set<std::string> typearg = std::unordered_set<std::string>()) {
    if (typearg.empty()) {
      return m_config["modules"];
    }
    nlohmann::json retArray;
    for (auto item : m_config["modules"]) {
      if (typearg.find(item["type"].get<std::string>()) != typearg.end()) {
        retArray.push_back(item);
      }
    }
    return retArray;
  }
  nlohmann::json &getResources() { return m_config["resources"]; }
  nlohmann::json &getMetricsSettings() { return m_config["metrics_settings"]; }
  nlohmann::json &getConnections(const std::string &key) { return getModule(key)["connections"]; }
  unsigned getNumReceiverConnections(const std::string &key) {
    return getModule(key)["connections"]["receivers"].size();
  }
  unsigned getNumSenderConnections(const std::string &key) {
    return getModule(key)["connections"]["senders"].size();
  }

  std::string getName() { return m_config["name"]; };

  template <typename T> void set(const std::string &key, const T &value) { m_config[key] = value; }
  template <typename T> T get(const std::string &key) { return m_config[key]; }

private:
  nlohmann::json m_config;
  nlohmann::json &getModule(const std::string &key) {
    for (unsigned i = 0; i < m_config["modules"].size(); i++) {
      if (m_config["modules"][i]["name"].get<std::string>() == key) {
        return m_config["modules"][i];
      }
    }
    throw NoConfigsForName(ERS_HERE, key.c_str());
  }
};

} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_CONFIGURATION_HPP
