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
#ifndef ERS_HPP
#define ERS_HPP

#include <ers/ers.h>

/** \def ERS_WARNING( message ) This macro sends the message to the ers::warning stream.
 */
#define ERS_WARNING(message)                                                                       \
  do {                                                                                             \
    ERS_REPORT_IMPL(ers::warning, ers::Message, message, ERS_EMPTY);                               \
  } while (0)

#define ERS_ERROR(message)                                                                         \
  do {                                                                                             \
    ERS_REPORT_IMPL(ers::error, ers::Message, message, ERS_EMPTY);                                 \
  } while (0)

#define DEBUG(message)                                                                             \
  do {                                                                                             \
    ERS_DEBUG(0, message);                                                                         \
  } while (0)

#define INFO(message)                                                                              \
  do {                                                                                             \
    ERS_INFO(message);                                                                             \
  } while (0)

#define LOG(message)                                                                               \
  do {                                                                                             \
    ERS_LOG(message);                                                                              \
  } while (0)

#define WARNING(message)                                                                           \
  do {                                                                                             \
    ERS_WARNING(message);                                                                          \
  } while (0)

#define ERROR(message)                                                                             \
  do {                                                                                             \
    ERS_ERROR(message);                                                                            \
  } while (0)

#endif