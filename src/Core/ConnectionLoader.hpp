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

#include "Core/Receiver.hpp"
#include "Core/Sender.hpp"
#include "Utils/Ers.hpp"
#include "Utils/Singleton.hpp"
#include "nlohmann/json.hpp"
#include <dlfcn.h>
#include <string>
#include <optional>

namespace daqling {
#include "DynamicLinkIssues.hpp"
ERS_DECLARE_ISSUE(core, ConnectionLoaderIssue, "", ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(core, UnrecognizedConnectionType, core::ConnectionLoaderIssue,
                       "Unrecognized connection type: " << type, ERS_EMPTY, ((const char *)type))
ERS_DECLARE_ISSUE_BASE(core, UnrecognizedQueueType, core::ConnectionLoaderIssue,
                       "Unrecognized queue type: " << type, ERS_EMPTY, ((const char *)type))
namespace core {

class Queue;
class Sender;
class Receiver;
class ConnectionLoader : public daqling::utilities::Singleton<ConnectionLoader> {
public:
  /**
   * @brief Dynamically loads connection library.
   * @param name name of connection to load.
   */
  bool loadConnection(const std::string &name);
  /**
   * @brief Dynamically loads queue library.
   * @param name name of queue to load.
   */
  bool loadQueue(const std::string &name);
  /**
   * @brief Gets a shared pointer to a DAQling Sender instance.
   * @param s type of sender.
   * @param chid chid of sender.
   * @param json json configuration for creating Sender.
   */
  std::shared_ptr<daqling::core::Sender> getSender(const std::string & /*s*/, const uint &chid,
                                                   const nlohmann::json &json);
  /**
   * @brief Gets a shared pointer to a DAQling Receiver instance.
   * @param s type of receiver.
   * @param chid chid of receiver.
   * @param json json configuration for creating receiver.
   */
  std::shared_ptr<daqling::core::Receiver> getReceiver(const std::string & /*s*/, const uint &chid,
                                                       const nlohmann::json &json);

  /**
   * @brief Adds function for creating Sender to Sender map.
   * @param T type of sender to add.
   * @param s name identifying sender type.
   */
  template <class T> void addSender(const std::string &s) {
    m_senderMap[s] = [](const uint &chid, const nlohmann::json &json) {
      return std::make_shared<T>(chid, json);
    };
  }

  /**
   * @brief Adds function for creating Receiver to Receover map.
   * @param T type of Receiver to add.
   * @param s name identifying Receiver type.
   */
  template <class T> void addReceiver(const std::string &s) {
    m_receiverMap[s] = [](const uint &chid, const nlohmann::json &json) {
      return std::make_shared<T>(chid, json);
    };
  }

  /**
   * @brief Adds function for creating Queue to Queue map.
   * @param T type of queue to add.
   * @param s name identifying queue type.
   */
  template <class T> void addQueue(const std::string &s) {
    m_queueMap[s] = [](const nlohmann::json &json) { return std::make_unique<T>(json); };
  }
  // static ConnectionLoader& instance();
  std::shared_ptr<daqling::core::Queue> getQueue(const std::string &type,
                                                 const nlohmann::json &json) {

    if (m_queueMap.find(type) == m_queueMap.end()) {
      loadQueue(type);
      if (m_queueMap.find(type) == m_queueMap.end()) {
        throw UnrecognizedQueueType(ERS_HERE, type.c_str());
      }
    }
    return m_queueMap[type](json);
  }

private:
  /**
   * @brief Creates and returns a QueueSender instance.
   * @param sen Sender to chain to the queue.
   * @param chid channel id of the QueueSender.
   * @param json json configuration of QueueSender. Including type of queue.
   */
  std::shared_ptr<daqling::core::Sender>
  getQueueSender(std::shared_ptr<daqling::core::Sender> /*sen*/, const uint &chid,
                 const nlohmann::json &json);
  /**
   * @brief Creates and returns a QueueReceiver instance.
   * @param sen Receiver to chain to the queue.
   * @param chid channel id of the QueueReceiver.
   * @param json json configuration of QueueReceiver. Including type of queue.
   */
  std::shared_ptr<daqling::core::Receiver>
  getQueueReceiver(std::shared_ptr<daqling::core::Receiver> /*rec*/, const uint &chid,
                   const nlohmann::json &json);
  /**
   * @brief Dynamically loads library. Helper function for loadQueue and loadConnection.
   * @param name Name of library to load.
   */
  bool load(const std::string &name);
  /**
   * @brief handle to dl library.
   */
  std::optional<void *> m_handle;
  /**
   * @brief Map with functions for creating Sender types.
   */
  std::unordered_map<std::string, std::function<std::shared_ptr<daqling::core::Sender>(
                                      const uint &, const nlohmann::json &)>>
      m_senderMap;
  /**
   * @brief Map with functions for creating Receiver types.
   */
  std::unordered_map<std::string, std::function<std::shared_ptr<daqling::core::Receiver>(
                                      const uint &, const nlohmann::json &)>>
      m_receiverMap;
  /**
   * @brief Map with functions for creating Queue types.
   */
  std::unordered_map<std::string,
                     std::function<std::unique_ptr<daqling::core::Queue>(const nlohmann::json &)>>
      m_queueMap;
};

} // namespace core
} // namespace daqling
