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

#include "nlohmann/json.hpp"
#include <iostream>

using nlohmann::json;

struct process {
  std::string name;
  std::string type;
};

std::vector<process> processes;

// void to_json(json& j, const std::vector<process>& c) {
//   j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
// }

static void from_json(const json &j, std::vector<process> &ps) {
  std::cout << "range-based for" << std::endl;
  for (auto &element : j) {
    process p;
    std::cout << element << '\n';
    element.at("name").get_to(p.name);
    element.at("type").get_to(p.type);
    ps.push_back(p);
  }
}

int main(int /*unused*/, char const * /*unused*/ []) {
  json j = {{{"name", "Luke"}, {"type", "Jedi"}},
            {{"name", "Han"}, {"type", "Mercenary"}},
            {{"name", "Leia"}, {"type", "Princess"}}};

  std::cout << "size " << j.size() << std::endl;
  std::cout << "is_array " << j.is_array() << std::endl;
  std::cout << j.dump() << std::endl;

  std::vector<process> ps;
  from_json(j, ps);
  std::cout << "Let's check what's in the struct!" << std::endl;
  for (auto &element : ps) {
    std::cout << "===============" << std::endl;
    std::cout << element.name << std::endl;
    std::cout << element.type << std::endl;
  }
  return 0;
}
