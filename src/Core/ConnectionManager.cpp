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

#include "ConnectionManager.hpp"

#include "ConnectionLoader.hpp"
#include "Utils/Binary.hpp"
#include "Utils/Ers.hpp"
#include <utility>

using namespace daqling::core;

bool ConnectionManager::addReceiverChannel(const std::string &key, const nlohmann::json &j) {
  auto &cl = daqling::core::ConnectionLoader::instance();
  std::string type;
  uint chid;
  try {
    chid = j.at("chid").get<uint>();
    type = j.at("type").get<std::string>();
  } catch (const std::exception &e) {
    throw CannotGetChidAndType(ERS_HERE, e.what());
    return false;
  }

  if (m_sub_managers[key]->m_receivers.find(chid) != m_sub_managers[key]->m_receivers.end()) {
    ERS_INFO(" Socket for channel already exists... Won't add this channel again.");
    return false;
  }

  try {
    m_sub_managers[key]->m_receivers[chid] = cl.getReceiver(type, chid, j);
    ERS_INFO(" Adding RECEIVER channel for: [" << chid << "] type: " << type);
  } catch (ers::Issue &i) {
    throw CannotAddChannel(ERS_HERE, "Caught Issue", i);
    return false;
  }
  m_sub_managers[key]->m_receivers[chid]->set_sleep_duration(1);
  m_receiver_channels++;
  m_sub_managers[key]->m_receiver_channels++;
  return true;
}

bool ConnectionManager::addSenderChannel(const std::string &key, const nlohmann::json &j) {
  auto &cl = daqling::core::ConnectionLoader::instance();
  uint chid;
  std::string type;
  try {
    chid = j.at("chid").get<uint>();
    type = j.at("type").get<std::string>();
  } catch (const std::exception &e) {
    throw CannotGetChidAndType(ERS_HERE, e.what());
    return false;
  }

  if (m_sub_managers[key]->m_senders.find(chid) != m_sub_managers[key]->m_senders.end()) {
    ERS_INFO(" Socket for channel already exists... Won't add this channel again.");
    return false;
  }

  try {
    m_sub_managers[key]->m_senders[chid] = cl.getSender(type, chid, j);
    ERS_INFO(" Adding SENDER channel for: [" << chid << "] type: " << type);
  } catch (ers::Issue &i) {
    throw CannotAddChannel(ERS_HERE, "Caught issue", i);
    return false;
  }
  m_sub_managers[key]->m_senders[chid]->set_sleep_duration(1);
  m_sender_channels++;
  m_sub_managers[key]->m_sender_channels++;
  return true;
}

bool ConnectionManager::start(const std::string &name) {
  if (m_sub_managers.find(name) != m_sub_managers.end()) {
    auto subManager = m_sub_managers[name];
    ERS_INFO("starting submanager with name: " << name);
    for (auto const &it : subManager->m_receivers) //([first: chn, second:dir])
    {
      it.second->start();
    }
    for (auto const &it : subManager->m_senders) //([first: chn, second:dir])
    {

      it.second->start();
    }
    return true;
  }
  return false; // TODO: put some meaning or return void
}

bool ConnectionManager::stop(const std::string &name) {
  if (m_sub_managers.find(name) != m_sub_managers.end()) {
    auto subManager = m_sub_managers[name];
    for (auto const &it : subManager->m_receivers) //([first: chn, second:dir])
    {
      it.second->stop();
    }
    for (auto const &it : subManager->m_senders) //([first: chn, second:dir])
    {
      it.second->stop();
    }
  }
  return true; // TODO: put some meaning or return void
}
bool ConnectionManager::removeChannel(const std::string &name) {
  if (m_sub_managers.find(name) != m_sub_managers.end()) {
    auto subManager = m_sub_managers[name];
    while (!subManager->getReceiverMap().empty()) {
      for (auto & [ ch, receiver ] : subManager->getReceiverMap()) {
        subManager->removeReceiverChannel(ch);
        m_receiver_channels--;
      }
    }
    while (!subManager->getSenderMap().empty()) {
      for (auto & [ ch, sender ] : subManager->getSenderMap()) {
        subManager->removeSenderChannel(ch);
        m_sender_channels--;
      }
    }
  }
  return !((m_sender_channels != 0u) || (m_receiver_channels != 0u));
}
std::shared_ptr<daqling::utilities::Resource> ConnectionManager::getLocalResource(unsigned id) {
  auto &rf = ResourceFactory::instance();
  return rf.getResource(id);
}
ConnectionSubManager &ConnectionManager::addSubManager(std::string key) {
  if (m_sub_managers.find(key) == m_sub_managers.end()) {
    m_sub_managers[key] = std::make_shared<ConnectionSubManager>(key);
    ERS_INFO("Creating sub manager for: " << key);
  } else {
    ERS_WARNING("Sub-manager with module name " << key << " Already exists");
  }
  return *m_sub_managers[key].get();
}

void ConnectionSubManager::set_receiver_sleep_duration(const unsigned &chn, uint ms) {
  m_receivers[chn]->set_sleep_duration(ms);
}
void ConnectionSubManager::set_sender_sleep_duration(const unsigned &chn, uint ms) {
  m_senders[chn]->set_sleep_duration(ms);
}
bool ConnectionSubManager::removeSenderChannel(unsigned chn) {
  m_senders.erase(chn);
  m_sender_channels--;
  return true;
}
bool ConnectionSubManager::removeReceiverChannel(unsigned chn) {
  m_receivers.erase(chn);
  m_receiver_channels--;
  return true;
}
ConnectionSubManager::ConnectionSubManager(std::string name) : m_name(std::move(name)) {}
