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
#include "nlohmann/json.hpp"
#include <ers/Issue.h>
#include <utility>
#include <zmq.hpp>

class zmq_sink {

public:
  zmq_sink(std::string name) : m_name{std::move(name)} {
    m_context = std::make_unique<zmq::context_t>(1);
    m_socket = std::make_unique<zmq::socket_t>(*(m_context.get()), ZMQ_PUB);
    m_socket->connect("tcp://localhost:6542");
  }
  void log(const ers::Issue &issue) {
    std::lock_guard<std::mutex> lock(_mutex);
    // send
    sink_it_(issue);
  }

  ~zmq_sink() {
    m_socket->close();
    m_context->close();
  }
  zmq_sink(zmq_sink const &) = delete;            // Copy construct
  zmq_sink(zmq_sink &&) = delete;                 // Move construct
  zmq_sink &operator=(zmq_sink const &) = delete; // Copy assign
  zmq_sink &operator=(zmq_sink &&) = delete;      // Move assign

private:
  std::unique_ptr<zmq::context_t> m_context;
  std::unique_ptr<zmq::socket_t> m_socket;
  std::string m_name;
  std::mutex _mutex;

private:
  void sink_it_(const ers::Issue &issue) {
    // header
    zmq::message_t topic(m_name.size());
    memcpy(topic.data(), m_name.data(), m_name.size());
    m_socket->send(topic, ZMQ_SNDMORE);

    // content
    // create json object from issue:
    nlohmann::json json_;
    json_["time"] = issue.time<std::chrono::milliseconds>("%Y-%m-%d %H:%M:%S");
    json_["package"] = issue.context().package_name();
    json_["severity"] = to_string(issue.severity());
    json_["fileName"] = std::string(issue.context().file_name())
                            .substr(std::string(issue.context().file_name()).find_last_of('/') + 1);
    json_["lineNumber"] = issue.context().line_number();
    json_["function"] = issue.context().function_name();
    json_["message"] = issue.message();

    std::string str = json_.dump();
    zmq::message_t message(str.size());
    memcpy(message.data(), str.data(), str.size());
    m_socket->send(message);
  }
};