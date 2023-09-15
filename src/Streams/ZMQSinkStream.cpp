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

#include "ZMQSinkStream.hpp"
#include "Utils/Singleton.hpp"
#include <ers/StreamFactory.h>
#include <ers/internal/Util.h>

ERS_REGISTER_OUTPUT_STREAM(ers::ZMQSinkStream, "ZMQSink", format)

ers::ZMQSinkStream::ZMQSinkStream(const std::string &format) : _sink{zmq_sink(format)} {}

void ers::ZMQSinkStream::write(const ers::Issue &issue) {
  // sink
  _sink.log(issue);
  chained().write(issue);

} // send
