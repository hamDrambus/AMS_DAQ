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
#include <ers/OutputStream.h>

#include "Utils/ZmqSink.hpp"

namespace ers {

class ZMQSinkStream : public OutputStream {
public:
  explicit ZMQSinkStream(const std::string &format);

  void write(const Issue &issue) override;

private:
  zmq_sink _sink;
  std::string _name;
};

} // namespace ers
