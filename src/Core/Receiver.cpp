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

#include "Receiver.hpp"
#include "Utils/Ers.hpp"
using namespace daqling::core;
Receiver::Receiver(uint chid) : m_chid(chid) {
  m_msg_handled.store(0);
  m_pcq_size.store(0);
}

bool Receiver::start() {
  ERS_DEBUG(0, "Default start method.");
  return true;
}
bool Receiver::stop() {
  ERS_DEBUG(0, "Default stop method.");
  return true;
}
bool Receiver::sleep_receive(DataTypeWrapper &bin) {
  if (receive(bin)) {
    return true;
  }
  std::this_thread::sleep_for((std::chrono::milliseconds(m_sleep_duration)));
  return false;
}
void Receiver::set_sleep_duration(uint ms) { m_sleep_duration = ms; }
std::atomic<size_t> &Receiver::getMsgsHandled() { return m_msg_handled; }
std::atomic<size_t> &Receiver::getPcqSize() { return m_pcq_size; }

#include <chrono>
#include <ctime>
#include <utility>
using namespace std::chrono_literals;
// Queue receiver
QueueReceiver::QueueReceiver(uint chid, std::unique_ptr<Queue> ptr,
                             const std::shared_ptr<Receiver> &chained_ptr)
    : Receiver(chid) {
  m_queue = std::move(ptr);
  setChainedReceiver(chained_ptr);
}
bool QueueReceiver::receive(DataTypeWrapper &bin) {
  if (m_queue->sizeGuess() != 0) {

    return m_queue->read(bin);
  }
  return false;
}
void QueueReceiver::set_sleep_duration(uint ms) {
  m_sleep_duration = ms;
  if (m_queue) {
    m_queue->set_sleep_duration(ms);
  }
}
bool QueueReceiver::sleep_receive(DataTypeWrapper &bin) {
  if (m_queue->sizeGuess() != 0) {

    return m_queue->sleep_read(bin);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(m_sleep_duration));
  return false;
}
bool QueueReceiver::start() {
  m_stop_handler.store(false);
  return (addReceiveHandler() && m_chained_receiver->start());
}
bool QueueReceiver::stop() {
  m_stop_handler.store(true);
  bool retval = m_chained_receiver->stop();
  m_handler_thread.join();
  return retval;
}
bool QueueReceiver::addReceiveHandler() {
  ERS_INFO(" [CLIENT] ReceiveHandler for channel [" << m_chid << "] starting...");
  m_handler_thread = std::thread([&]() {
    while (!m_stop_handler) {
      DataTypeWrapper m_container;
      if (m_chained_receiver->sleep_receive(m_container)) {
        while (!m_queue->sleep_write(m_container) && !m_stop_handler) {
          ERS_WARNING("Waiting queue to allow write");
        }
      }
      m_pcq_size.store(m_queue->sizeGuess());
      if (m_queue->sizeGuess() > m_queue->capacity() * 0.9) {
        ERS_WARNING("CLIENT -> queue population: " << m_queue->sizeGuess());
      }
    }
    ERS_INFO(" joining channel [" << m_chid << "] handler.");
  });
  return true;
}
void QueueReceiver::setChainedReceiver(std::shared_ptr<daqling::core::Receiver> ptr) {
  m_chained_receiver = std::move(ptr);
  m_chained_receiver->set_sleep_duration(1);
}
std::atomic<size_t> &QueueReceiver::getMsgsHandled() {
  return m_chained_receiver->getMsgsHandled();
}