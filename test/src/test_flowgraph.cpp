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

#include "Utils/Logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "tbb/flow_graph.h"

using namespace tbb::flow;
using logger = daqling::utilities::Logger;

struct body {
  std::string my_name;
  body(const char *name) : my_name(name) {}
  void operator()(continue_msg) const { printf("%s\n", my_name.c_str()); }
};

int main(int, char **) {
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto logger = std::make_shared<spdlog::logger>("my_logger", sink);
  logger::set_instance(logger);

  INFO("WOOF WOOF -> Test basic TBB FlowGraph");

  graph g;
  continue_node<continue_msg> hello(g, [](const continue_msg &) { INFO("Hello"); });
  continue_node<continue_msg> world(g, [](const continue_msg &) { INFO("World"); });
  make_edge(hello, world);
  hello.try_put(continue_msg());
  g.wait_for_all();

  graph g2;

  broadcast_node<continue_msg> start(g2);
  continue_node<continue_msg> a(g2, body("A"));
  continue_node<continue_msg> b(g2, body("B"));
  continue_node<continue_msg> c(g2, body("C"));
  continue_node<continue_msg> d(g2, body("D"));
  continue_node<continue_msg> e(g2, body("E"));

  make_edge(start, a);
  make_edge(start, b);
  make_edge(a, c);
  make_edge(b, c);
  make_edge(c, d);
  make_edge(a, e);

  INFO("WOOF WOOF -> Launching dependency graph...");
  for (int i = 0; i < 3; ++i) {
    start.try_put(continue_msg());
    g2.wait_for_all();
    INFO("   -> Next round...");
  }

  INFO("WOOF WOOF -> Cleaning house...");
  return 0;
}
