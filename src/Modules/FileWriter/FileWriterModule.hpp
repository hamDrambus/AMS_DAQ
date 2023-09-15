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

#include "Core/DAQProcess.hpp"
#include "Utils/Binary.hpp"
#include "Utils/ReusableThread.hpp"
#include "folly/ProducerConsumerQueue.h"
#include <fstream>
#include <map>
#include <tuple>

namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(module, InvalidFileName, "Invalid File name pattern", ERS_EMPTY)

ERS_DECLARE_ISSUE(module, OfstreamFail, "std::ofstream::fail()", ERS_EMPTY)
}
/**
 * Module for writing your acquired data to file.
 */
class FileWriterModule : public daqling::core::DAQProcess {
public:
  FileWriterModule(const std::string & /*n*/);

  void configure() override;
  void start(unsigned run_num) override;
  void stop() override;
  void runner() noexcept override;

  void monitor_runner();

private:
  struct ThreadContext {
    ThreadContext(std::array<unsigned int, 2> tids) : consumer(tids[0]), producer(tids[1]) {}
    daqling::utilities::ReusableThread consumer;
    daqling::utilities::ReusableThread producer;
  };
  using PayloadQueue = folly::ProducerConsumerQueue<SharedDataType<daqling::utilities::Binary>>;
  using Context = std::tuple<PayloadQueue, ThreadContext>;

  struct Metrics {
    std::atomic<size_t> bytes_written = 0;
    std::atomic<size_t> payload_queue_size = 0;
    std::atomic<size_t> payload_size = 0;
  };

  size_t m_buffer_size{};
  std::string m_pattern;
  std::atomic<bool> m_start_completed{};

  /**
   * Output file generator with a printf-like filename pattern.
   */
  class FileGenerator {
  public:
    FileGenerator(std::string pattern, const uint64_t chid, const unsigned run_number)
        : m_pattern(std::move(pattern)), m_chid(chid), m_run_number(run_number) {}

    /**
     * Generates the next output file in the sequence.
     *
     * @warning Silently overwrites files if they already exists
     * @warning Silently overwrites previous output files if specified pattern does not generate
     * unique file names.
     */
    std::ofstream next();

    /**
     * Returns whether `pattern` yields unique output files on rotation.
     * Effectively checks whether the pattern contains %n.
     */
    static bool yields_unique(const std::string &pattern);

  private:
    const std::string m_pattern;
    const uint64_t m_chid;
    unsigned m_filenum = 0;
    const unsigned m_run_number;
  };

  // Configs
  size_t m_max_filesize{};
  uint64_t m_channels = 0;

  // Thread control
  std::atomic<bool> m_stopWriters;

  // Metrics
  mutable std::map<uint64_t, Metrics> m_channelMetrics;

  // Internals
  void flusher(uint64_t chid, PayloadQueue &pq, size_t max_buffer_size, FileGenerator fg) const;
  std::map<uint64_t, Context> m_channelContexts;
  std::thread m_monitor_thread;
};
