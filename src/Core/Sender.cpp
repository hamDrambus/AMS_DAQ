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

#include "Sender.hpp"
#include "Utils/Ers.hpp"
using namespace daqling::core;

Sender::Sender(uint chid) : m_chid(chid) {
  m_msg_handled.store(0);
  m_pcq_size.store(0);
}
bool Sender::start() {
  ERS_DEBUG(0, "Default start method.");
  return true;
}
bool Sender::stop() {

  ERS_DEBUG(0, "Default stop method.");
  return true;
}
bool Sender::sleep_send(DataTypeWrapper &bin) {
  if (send(bin)) {
    return true;
  }
  std::this_thread::sleep_for((std::chrono::milliseconds(m_sleep_duration)));
  return false;
}
void Sender::set_sleep_duration(uint ms) { m_sleep_duration = ms; }
std::atomic<size_t> &Sender::getMsgsHandled() { return m_msg_handled; }
std::atomic<size_t> &Sender::getPcqSize() { return m_pcq_size; }

#include <chrono>
#include <ctime>
#include <utility>
using namespace std::chrono_literals;
// Queue sender
QueueSender::QueueSender(uint chid, std::unique_ptr<Queue> ptr,
                         const std::shared_ptr<Sender> &chained_ptr)
    : Sender(chid) {
  m_queue = std::move(ptr);
  setChainedSender(chained_ptr);
}
bool QueueSender::send(DataTypeWrapper &bin) { return m_queue->write(bin); }
bool QueueSender::sleep_send(DataTypeWrapper &bin) { return m_queue->sleep_write(bin); }
void QueueSender::set_sleep_duration(uint ms) {
  m_sleep_duration = ms;
  if (m_queue) {
    m_queue->set_sleep_duration(ms);
  }
}
bool QueueSender::start() {
  m_stop_handler.store(false);
  return (addSendHandler() && m_chained_sender->start());
}
bool QueueSender::stop() {
  m_stop_handler.store(true);
  bool retval = m_chained_sender->stop();
  m_handler_thread.join();
  return retval;
}
bool QueueSender::addSendHandler() {
  ERS_INFO(" [SERVER] SendHandler for channel [" << m_chid << "] starting...");
  m_handler_thread = std::thread([&]() {
    while (!m_stop_handler) {
      if (m_queue->sizeGuess() != 0) {
        DataTypeWrapper m_container;
        if (m_queue->sleep_read(m_container)) {
          m_chained_sender->send(m_container);
        }
      } else {
        std::this_thread::sleep_for(1ms);
      }
      m_pcq_size.store(m_queue->sizeGuess());
      if (m_queue->sizeGuess() > m_queue->capacity() * 0.9) {
        ERS_WARNING("SERVER -> queue population: " << m_queue->sizeGuess());
      }
    }
    ERS_INFO(" joining channel [" << m_chid << "] handler.");
  });
  return true;
}

void QueueSender::setChainedSender(std::shared_ptr<daqling::core::Sender> ptr) {
  m_chained_sender = std::move(ptr);
  m_chained_sender->set_sleep_duration(1);
}
std::atomic<size_t> &QueueSender::getMsgsHandled() { return m_chained_sender->getMsgsHandled(); }