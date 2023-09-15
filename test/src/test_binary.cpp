#include "Utils/Binary.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using daqling::utilities::Binary;

int main(int /*unused*/, char * /*unused*/ []) {
  // ctors, operator==, operator!=
  {
    const Binary ba;
    assert(ba.size() == 0);
    assert(ba == ba);

    const std::vector<uint8_t> bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    Binary bb{bytes.data(), bytes.size()};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    assert(std::strncmp(reinterpret_cast<const char *>(bytes.data()), bb.data<char *>(),
                        bytes.size()) == 0);
    const Binary bc{std::move(bb)};
    assert(bb.data() == nullptr); // NOLINT(misc-use-after-move)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    assert(std::strncmp(reinterpret_cast<const char *>(bytes.data()), bc.data<const char *>(),
                        bytes.size()) == 0);

    assert(ba != bc);
  }

  // operator=, operator+=
  {
    Binary ba;
    const char *str = "some string";
    const Binary bb{str, std::strlen(str)};
    ba = bb;
    assert(ba == bb);

    Binary bc;
    bc += bb;
    bc += bb;
    assert(!bc.error());
    assert(std::strncmp("some stringsome string", bc.data<char *>(), std::strlen(str) * 2) == 0);

    ba = std::move(bc);
    assert(std::strncmp("some stringsome string", ba.data<char *>(), std::strlen(str) * 2) == 0);
    assert(bc.data() == nullptr); // NOLINT(misc-use-after-move)
  }
}
