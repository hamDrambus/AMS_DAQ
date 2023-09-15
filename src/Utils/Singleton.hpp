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

#ifndef DAQLING_UTILITIES_SINGLETON_HPP
#define DAQLING_UTILITIES_SINGLETON_HPP

/*
 * Types
 * Description:
 *   Singleton template from CodeReviews
 *   https://codereview.stackexchange.com/questions/173929/modern-c-singleton-template
 * Date: March 2019
 */

#include <memory>
#include <mutex>
namespace daqling {
namespace utilities {

template <typename T> class Singleton { // NOLINT(cppcoreguidelines-special-member-functions)
public:
  static T &instance();
  // Prevent copying and moving.
  Singleton(Singleton const &) = delete;            // Copy construct
  Singleton(Singleton &&) = delete;                 // Move construct
  Singleton &operator=(Singleton const &) = delete; // Copy assign
  Singleton &operator=(Singleton &&) = delete;      // Move assign

protected:
  static std::mutex s_mutex;
  Singleton() = default;
};
template <typename T> std::mutex Singleton<T>::s_mutex;

template <typename T> T &Singleton<T>::instance() {
  std::scoped_lock lock(s_mutex);
  static const std::unique_ptr<T> instance{new T{}};
  return *instance;
}

} // namespace utilities
} // namespace daqling

#endif // DAQLING_UTILITIES_SINGLETON_HPP
