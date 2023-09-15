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
 * @file DAQProcess.hpp
 * @brief Base class for Modules loaded via the ModuleLoader
 * @date 2019-02-20
 */

#ifndef DAQLING_CORE_DAQPROCESS_HPP
#define DAQLING_CORE_DAQPROCESS_HPP

#include "ConnectionManager.hpp"
#include "Statistics.hpp"
#include "Utils/Ers.hpp"
#include "Utils/ThreadTagger.hpp"
namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(core, DAQProcessIssue, "", ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(core, CannotSetupStats, core::DAQProcessIssue,
                       "Connection setup failed for Statistics publishing!", ERS_EMPTY, ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(core, CannotGetStatPointer, core::DAQProcessIssue,
                       "Cannot get stat pointer! cause: " << ewhat, ERS_EMPTY,
                       ((const char *)ewhat))
namespace core {

using namespace std::placeholders;
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class DAQProcess {
public:
  DAQProcess(const std::string &name)
      : m_name(name),
        m_connections(daqling::core::ConnectionManager::instance().addSubManager(name)) {
    addTag();
  }

  virtual ~DAQProcess() = default;
  nlohmann::json &getModuleSettings() {
    return daqling::core::Configuration::instance().getModuleSettings(getName());
  }
  std::string getName() { return m_name; }
  /* use virtual otherwise linker will try to perform static linkage */
  virtual void configure() {
    // assign submanager
    setupStatistics();
    if (m_statistics->isStatsOn()) {
      m_statistics->start();
    }
  };
  virtual void unconfigure() { m_statistics->unsetStatsConnection(); };
  virtual void down() { m_statistics->unsetStatsConnection(); };

  virtual void start(unsigned run_num) {
    m_run_number = run_num;
    ERS_DEBUG(0, "run number " << m_run_number);
    m_run = true;
    m_runner_thread = std::thread(&DAQProcess::runner_wrapper, this);
  };

  virtual void stop() {
    m_run = false;
    if (running()) {
      m_runner_thread.join();
    }
  };

  virtual void runner() noexcept = 0;

  /**
   * Returns whether a command was found in register.
   */
  bool isCommandRegistered(const std::string &key) noexcept {
    if (auto cmd = m_commands.find(key); cmd != m_commands.end()) {
      return true;
    }
    ERS_WARNING("No command '" << key << "' registered");
    return false;
  }

  /**
   * Runs a registered custom command named `key`.
   */
  void command(const std::string &key, const std::string &arg) {
    auto cmd = m_commands.find(key);
    std::get<0>(cmd->second)(arg);
  }

  std::string getCommandTransitionState(const std::string &key) {
    auto cmd = m_commands.find(key);
    return std::get<1>(cmd->second);
  }

  std::string getCommandTargetState(const std::string &key) {
    auto cmd = m_commands.find(key);
    return std::get<2>(cmd->second);
  }

  /**
   * Creates the Statistics class, and registers metrics for all connections.
   */
  bool setupStatistics() {
    try {

      m_statistics = std::make_unique<Statistics>(m_config.getMetricsSettings());

      for (auto & [ ch, receiver ] : m_connections.getReceiverMap()) {
        m_statistics->registerMetric<std::atomic<size_t>>(
            &receiver->getPcqSize(), "ReceiverCh" + std::to_string(ch) + "-QueueSizeGuess",
            daqling::core::metrics::LAST_VALUE);
        m_statistics->registerMetric<std::atomic<size_t>>(
            &receiver->getMsgsHandled(), "ReceiverCh" + std::to_string(ch) + "-NumMessages",
            daqling::core::metrics::RATE);
      }
      for (auto & [ ch, sender ] : m_connections.getSenderMap()) {
        m_statistics->registerMetric<std::atomic<size_t>>(
            &sender->getPcqSize(), "SenderCh" + std::to_string(ch) + "-QueueSizeGuess",
            daqling::core::metrics::LAST_VALUE);
        m_statistics->registerMetric<std::atomic<size_t>>(
            &sender->getMsgsHandled(), "SenderCh" + std::to_string(ch) + "-NumMessages",
            daqling::core::metrics::RATE);
      }
    } catch (const ers::Issue &i) {
      throw CannotSetupStats(ERS_HERE, i);
    }

    return true;
  }

  bool running() const { return m_runner_thread.joinable(); }

protected:
  std::string m_name;
  void runner_wrapper() {
    addTag();
    runner();
  }
  void addTag() const { daqling::utilities::ThreadTagger::instance().writeTag(m_name); }
  /**
   * Registers a custom command under the name `cmd`.
   * Returns whether the command was inserted (false meaning that command `cmd` already exists)
   */
  template <typename Function, typename... Args>
  bool registerCommand(const std::string &cmd, const std::string &transition_state,
                       const std::string &target_state, Function &&f, Args &&... args) {
    return m_commands
        .emplace(cmd, std::make_tuple(std::bind(f, args...), transition_state, target_state))
        .second;
  }

  // ZMQ ConnectionManager
  daqling::core::ConnectionSubManager &m_connections;
  // JSON Configuration map
  daqling::core::Configuration &m_config = daqling::core::Configuration::instance();

  // Stats
  bool m_stats_on{};
  std::shared_ptr<Statistics> m_statistics;

  std::atomic<bool> m_run{};
  std::thread m_runner_thread;
  unsigned m_run_number{};

private:
  std::map<const std::string, std::tuple<std::function<void(const std::string &)>,
                                         const std::string, const std::string>>
      m_commands;
};
} // namespace core
} // namespace daqling

#endif // DAQLING_CORE_DAQPROCESS_HPP
