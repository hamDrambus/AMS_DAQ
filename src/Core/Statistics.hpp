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

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "Configuration.hpp"
#include "Metric.hpp"
#include "Utils/Common.hpp"
#include "Utils/Ers.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <zmq.hpp>
#ifndef BUILD_WITHOUT_CPR
#include <cpr/cpr.h>
#endif

namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(core, StatisticIssue, "", ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(core, NoHTTPSupport, core::StatisticIssue,
                       "Failed to publish over HTTP. The library is not built with CURL support!",
                       ERS_EMPTY, ERS_EMPTY)
ERS_DECLARE_ISSUE_BASE(core, CannotAddStatsChannel, core::StatisticIssue,
                       "Failed to add stats channel! ZMQ returned: " << eWhat, ERS_EMPTY,
                       ((const char *)eWhat))
ERS_DECLARE_ISSUE_BASE(core, CannotSetupStatPublishing, core::StatisticIssue,
                       "Connection setup failed for Statistics publishing!", ERS_EMPTY, ERS_EMPTY)
namespace core {

class Statistics {
public:
  Statistics(nlohmann::json &j);
  ~Statistics();
  Statistics(Statistics const &) = delete;            // Copy construct
  Statistics(Statistics &&) = delete;                 // Move construct
  Statistics &operator=(Statistics const &) = delete; // Copy assign
  Statistics &operator=(Statistics &&) = delete;      // Move assign

  bool configure(unsigned interval);

  void setZMQpublishing(bool zmq_publisher) { m_zmq_publisher = zmq_publisher; }
  void setInfluxDBsending(bool influxDb) { m_influxDb = influxDb; }
  void setInfluxDBname(std::string influxDb_name) { m_influxDb_name = std::move(influxDb_name); }
  void setInfluxDBuri(std::string influxDb_uri) { m_influxDb_uri = std::move(influxDb_uri); }
  bool isStatsOn() { return m_stats_on; }
  void start();

  template <class T> void registerMetric(T *pointer, std::string name, metrics::metric_type mtype) {
    ERS_INFO("Registering metrics: " << name << " with module name: " << m_name);
    name = m_name + "-" + name;
    if (typeid(T) == typeid(std::atomic<int>)) {
      registerVariable<T, int>(pointer, name, mtype, metrics::INT);
    } else if (typeid(T) == typeid(std::atomic<float>)) {
      registerVariable<T, float>(pointer, name, mtype, metrics::FLOAT);
    } else if (typeid(T) == typeid(std::atomic<double>)) {
      registerVariable<T, double>(pointer, name, mtype, metrics::DOUBLE);
    } else if (typeid(T) == typeid(std::atomic<bool>)) {
      registerVariable<T, bool>(pointer, name, mtype, metrics::BOOL);
    } else if (typeid(T) == typeid(std::atomic<size_t>)) {
      registerVariable<T, size_t>(pointer, name, mtype, metrics::SIZE);
    } else {
      ERS_WARNING("Failed to register metric " << name
                                               << ": Unsupported metric type! Supported types:\n"
                                                  " - std::atomic<int>\n"
                                                  " - std::atomic<float>\n"
                                                  " - std::atomic<double>\n"
                                                  " - std::atomic<bool>\n"
                                                  " - std::atomic<size_t>");
    }
  }

  template <class T, class U>
  void registerVariable(T *pointer, std::string name, metrics::metric_type mtype,
                        metrics::variable_type vtype) {

    std::shared_ptr<Metric<T, U>> metric(new Metric<T, U>(pointer, name, mtype, vtype));
    std::shared_ptr<Metric_base> metric_base = std::dynamic_pointer_cast<Metric_base>(metric);
    std::lock_guard<std::mutex> lck(m_mtx);
    m_reg_metrics.push_back(metric_base);
  }

  template <class T, class U> void accumulateValue(Metric_base *m) {
    auto *metric = static_cast<Metric<T, U> *>(m);
    U value = *(metric->m_metrics_ptr);
    metric->m_values.push_back(value);
  }

  template <class T, class U> void accumulateValueAndReset(Metric_base *m) {
    auto *metric = static_cast<Metric<T, U> *>(m);
    U value = *(metric->m_metrics_ptr);
    *(metric->m_metrics_ptr) = 0.;
    metric->m_values.push_back(value);
  }

  void flushPublishValues() {
    if (m_zmq_publisher) {
      // Move rest of posting to new func
      if (influx_msg.str().empty()) {
        return;
      }
      zmq::message_t message(influx_msg.str().size() - 1);
      memcpy(message.data(), influx_msg.str().data(), influx_msg.str().size() - 1);
      ERS_DEBUG(0, " MSG " << influx_msg.str());
      bool rc = m_stat_socket->send(message);
      if (!rc) {
        ERS_WARNING("Failed to publish metrics");
      }
    }
    if (m_influxDb) {
#ifndef BUILD_WITHOUT_CPR

      std::int32_t status_code;
      std::string text;
      cpr::PostCallback(
          [&status_code, &text](cpr::Response r) {
            status_code = r.status_code;
            text = r.text;
          },
          cpr::Url{m_influxDb_uri + m_influxDb_name}, cpr::Body{influx_msg.str()},
          cpr::Header{{"Content-Type", "text/plain"}}, cpr::Timeout{1000});
      ERS_DEBUG(0, "InfluxDB response: " << status_code << "\t" << text);
#else
      throw NoHTTPSupport(ERS_HERE);
#endif
    }
    influx_msg.str("");
  }

  template <class T, class U> void publishValue(Metric_base *m) {
    ERS_DEBUG(0, "publish value");
    U value = 0;
    auto *metric = static_cast<Metric<T, U> *>(m);
    if (metric->m_mtype == metrics::AVERAGE) {
      U average = std::accumulate(metric->m_values.begin(), metric->m_values.end(), 0.0) /
                  metric->m_values.size();
      metric->m_values.clear();
      metric->m_values.shrink_to_fit();
      value = average;
    } else if (metric->m_mtype == metrics::ACCUMULATE) {
      U sum = std::accumulate(metric->m_values.begin(), metric->m_values.end(), 0.0);
      metric->m_values.clear();
      metric->m_values.shrink_to_fit();
      value = sum;
    } else if (metric->m_mtype == metrics::LAST_VALUE) {
      value = *(metric->m_metrics_ptr);
      metric->m_values.clear();
      metric->m_values.shrink_to_fit();
      metric->m_values.push_back(value);
    } else if (metric->m_mtype == metrics::RATE) {
      value = *(metric->m_metrics_ptr);
      U last_value = 0;
      if (metric->m_values.size() == 1) {
        last_value = metric->m_values[0];
      }
      metric->m_values.clear();
      metric->m_values.shrink_to_fit();
      metric->m_values.push_back(value);
      if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                metric->m_timestamp) !=
          std::chrono::milliseconds(0)) {
        value = (value - last_value) * 1000 /
                static_cast<U>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now() - metric->m_timestamp)
                                   .count());
      } else {
        ERS_DEBUG(
            0,
            "Too short time interval to calculate RATE! Extend delta_t parameter of your metric");
        return;
      }
    }

    metric->m_timestamp = std::chrono::system_clock::now();

    ERS_DEBUG(0, "Sending the metric: " << metric->m_name << " value: " << std::to_string(value)
                                        << " to influxDB");
    ERS_DEBUG(0, m_influxDb_uri + m_influxDb_name);
    influx_msg << metric->m_name << " value=" << value << " "
               << metric->m_timestamp.time_since_epoch().count() << "\n";
  }
  bool unsetStatsConnection();

private:
  // Publishing messages
  std::ostringstream influx_msg;

  // Thread control
  std::thread m_stat_thread;
  std::atomic<bool> m_stop_thread{};
  std::mutex m_mtx;

  // Config for data publishing
  std::atomic<bool> m_influxDb{};
  std::atomic<bool> m_zmq_publisher{};

  // Config for influxDB
  std::string m_influxDb_name;
  std::string m_influxDb_uri;

  // stats set to post
  bool m_stats_on;
  // zmq connection
  bool setupStatsConnection(uint8_t ioT, const std::string &connStr);
  // Publish socket ref for stats
  std::unique_ptr<zmq::socket_t> m_stat_socket;
  std::unique_ptr<zmq::context_t> m_stats_context;
  std::atomic<bool> m_is_stats_setup{};

  // Config
  unsigned m_interval;
  std::vector<std::shared_ptr<Metric_base>> m_reg_metrics;

  // Runner
  void CheckStatistics();

  daqling::core::Configuration &m_config = daqling::core::Configuration::instance();
  std::string m_name;
};

} // namespace core
} // namespace daqling

#endif // STATISTICS_HPP
