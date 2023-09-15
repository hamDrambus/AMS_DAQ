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
#include "Singleton.hpp"
// #include <boost/thread/shared_mutex.hpp>
#include <map>
#include <shared_mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <utility>
namespace daqling {
namespace utilities {

/*
 * ThreadTagger
 * Description: Singleton class responsible for tagging threads with the name of the module they
 * belong to, and storing these in a map. The ERS DaqlingStandardStream can then retrieve these tags
 * via the readTag method, and add the tag to the log messages. Date: July 2021
 */
class ThreadTagger : public Singleton<ThreadTagger> {
private:
  /**
   * @brief Map storing the tags. Key: thread id
   */
  std::map<pid_t, std::string> m_tags_map;
  /**
   * @brief Reader/Writer lock controlling access to the tags map.
   */
  std::shared_mutex m_reader_writer_lock;

public:
  /**
   * @brief Returns the tag of the current thread.
   */
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::string readTagCurrentThread() { return readTag(syscall(SYS_gettid)); }
  /**
   * @brief Returns the tag of the thread with the id pid
   * @param pid id of the thread to read tag of.
   * @return tag corresponding to pid.
   */
  std::string readTag(pid_t pid) {
    std::shared_lock<std::shared_mutex> lock(m_reader_writer_lock);
    if (m_tags_map.find(pid) != m_tags_map.end()) {
      return m_tags_map[pid];
    }
    // potentially percolate up through thread tree?
    return "unknown";
  }
  /**
   * @brief Adds tag to current thread
   * @param tag tag to add to the current thread.
   */
  void writeTag(std::string tag) {
    // get pid
    std::unique_lock<std::shared_mutex> lock(m_reader_writer_lock);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    m_tags_map[static_cast<pid_t>(syscall(SYS_gettid))] = std::move(tag);
  }
};

} // namespace utilities
} // namespace daqling