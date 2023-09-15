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

#include <chrono>
#include <iostream>
#include <thread>

#include <zmq.hpp>

struct data_t {
  uint8_t tag;
  uint32_t blab : 24;
  uint32_t whatever;
} __attribute__((__packed__));

int main(int argc, char *argv[]) {
  zmq::context_t context(1);
  zmq::socket_t subscriber(context, ZMQ_SUB);

  if (argc == 2) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    subscriber.connect(argv[1]);
  } else {
    subscriber.connect("tcp://localhost:5556");
  }

  uint8_t tag = 126;
  subscriber.setsockopt(ZMQ_SUBSCRIBE, &tag, sizeof(tag));
  //   subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  std::cout << "sock opt" << std::endl;

  while (true) {
    zmq::message_t msg;
    std::cout << "-----------\nReceived " << subscriber.recv(&msg) << std::endl;
    std::cout << "-> size " << msg.size() << std::endl;

    data_t d = {0, 0, 0};
    // memcpy(&d, msg.data(), msg.size());
    d = *static_cast<data_t *>(msg.data());

    std::cout << static_cast<int>(d.tag) << std::endl;
    std::cout << d.whatever << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
