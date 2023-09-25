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

#ifndef DAQLING_UTILITIES_BINARY_HPP
#define DAQLING_UTILITIES_BINARY_HPP

/*
 * Binary
 * Description:
 *   A convenient wrapper around a dynamically sized, continous memory area.
 *   Based on the idea of the void* wrapper from CORAL
 *   https://twiki.cern.ch/twiki/bin/view/Persistency/Coral
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iomanip>
#include <vector>
#include <optional>

namespace daqling {
namespace utilities {

class Binary {

public:
  using byte = uint8_t;
  enum class error_code {
    alloc,
    invalid_arg,
    overflow,
  };

  /// Default Constructor. Creates an empty BLOB
  Binary() noexcept = default;

  /// Destructor. Frees internally allocated memory, if any
  ~Binary() noexcept = default;

  /// Constructor initializing a BLOB with `size` bytes from `data`
  explicit Binary(const void *data, const size_t size) noexcept {
    if (data == nullptr) {
      m_error = error_code::invalid_arg;
      return;
    }

    try {
      m_data = std::vector<byte>(static_cast<const byte *>(data),
                                 // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                 static_cast<const byte *>(data) + size);
    } catch (const std::bad_alloc &) {
      m_error = error_code::alloc;
    }
  }

  /// Copy constructor
  explicit Binary(const Binary &rhs) noexcept {
    try {
      m_data = rhs.m_data;
    } catch (const std::bad_alloc &) {
      m_error = error_code::alloc;
    }
  }

  /// Move constructor
  explicit Binary(Binary &&rhs) noexcept : m_data{std::move(rhs.m_data)}, m_error{rhs.m_error} {}

  /// Assignment operator
  Binary &operator=(const Binary &rhs) noexcept {
    try {
      m_data = rhs.m_data;
    } catch (const std::bad_alloc &) {
      m_error = error_code::alloc;
    }

    return *this;
  }

  /// Move Assignment operator
  Binary &operator=(Binary &&rhs) noexcept {
    /*
     * Required identity test as `x = std::move(x)` is UB.
     * See <https://stackoverflow.com/a/24604504>
     */
    if (this != &rhs) {
      m_data = std::move(rhs.m_data);
      m_error = rhs.m_error;
    }

    return *this;
  }

  /// Appends the data of another blob
  Binary &operator+=(const Binary &rhs) noexcept {
    assert(!m_error);

    try {
      m_data.insert(m_data.end(), rhs.m_data.begin(), rhs.m_data.end());
    } catch (const std::bad_alloc &) {
      m_error = error_code::alloc;
    }
    return *this;
  }

  /// Equal operator. Compares the contents of the binary blocks
  bool operator==(const Binary &rhs) const noexcept {
    return (m_data == rhs.m_data) && (m_error == rhs.m_error);
  }

  /// Comparison operator
  inline bool operator!=(const Binary &rhs) const noexcept { return !this->operator==(rhs); }

  /// Returns the internally stored data
  template <typename T = void *> const T data() const noexcept {
    static_assert(std::is_pointer<T>(), "Type parameter must be a pointer type");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-*-cast)
    return reinterpret_cast<T>(const_cast<byte *>(m_data.data()));
  }

  /// Returns the internally stored data
  template <typename T = void *> T data() noexcept {
    static_assert(std::is_pointer<T>(), "Type parameter must be a pointer type");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<T>(m_data.data());
  }

  /// Current size of the blob
  size_t size() const noexcept { return m_data.size(); }

  /// Reserve memory for the blob
  void reserve(size_t size) noexcept { m_data.reserve(size); }

  /// Resize the memory to given size
  void resize(size_t size) noexcept {
    try {
      m_data.resize(size);
    } catch (const std::bad_alloc &) {
      m_error = error_code::alloc;
    }
  }

  /// Sets size to zero and resets the state
  void clear() noexcept {
    resize(0);
    reset();
  }

  /// Resets the state
  void reset() noexcept {
    m_error = {};
  }

  /// Returns whether or not the Binary is in a usable state or if an error occured
  [[nodiscard]] std::optional<error_code> error() const noexcept { return m_error; }

  /// Copy 'num' bytes from 'source' buffer to the blob starting with 'dest' byte.
  /// The blob will be expanded if necessary.
  /// Returns index of the byte after the last written byte (new effective size).
  size_t memwrite(size_t dest, const void* source, size_t num) {
    size_t new_sz = dest + num;
    if (new_sz < dest || new_sz < num)
      throw std::overflow_error(std::string(__PRETTY_FUNCTION__) + ":\n\tread index overflowed size_t. " +
          "Most likely the transfered data (byte buffer) is corrupted.");
    if (new_sz > m_data.size())
      m_data.resize(new_sz);
    std::memcpy(reinterpret_cast<char *>(m_data.data()) + dest, source, num);
    return new_sz;
  }

  /// Copy 'num' bytes from the blob starting with 'from' byte to 'dest' buffer.
  /// The destination must be correctly allocated.
  /// Returns new position from which further bytes should be read.
  size_t memread(size_t from, void* dest, size_t num) const {
    size_t new_sz = from + num;
    if (new_sz < from || new_sz < num)
      throw std::overflow_error(std::string(__PRETTY_FUNCTION__) + ":\n\tread index overflowed size_t. " +
          "Most likely the transfered data (byte buffer) is corrupted.");
    if (new_sz > m_data.size())
      throw std::out_of_range(std::string(__PRETTY_FUNCTION__) + ":\n\tread index went outside byte buffer size. " +
          "Most likely the transfered data (byte buffer) is corrupted.");
    std::memcpy(dest, reinterpret_cast<const char *>(m_data.data()) + from, num);
    return new_sz;
  }

private:
  /// The BLOB data buffer
  std::vector<byte> m_data;

  /// Error flag
  std::optional<error_code> m_error;
};
} // namespace utilities
} // namespace daqling

/// xxd(1)-like output representation of a `Binary`
inline std::ostream &operator<<(std::ostream &out, const daqling::utilities::Binary &rhs) {
  for (size_t i = 0; i < rhs.size(); i++) {
    const bool newline_prefix = i % 16 == 0;
    const bool newline = (i + 1) % 16 == 0;
    const bool seperate = (i + 1) % 2 == 0;
    const bool last_byte = i == rhs.size() - 1;

    auto c = rhs.data<uint8_t *>() + i;

    if (newline_prefix) {
      // Print offset
      out << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
    }

    // Print the byte in the byte line
    out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*c)
        << (seperate && !newline ? " " : "");

    if (newline || last_byte) {
      const auto str = std::invoke([c, i]() {
        std::locale loc("C");
        std::string str(c - i % 16, c + 1);

        // Replace unprintable characters with a '.'
        std::replace_if(str.begin(), str.end(), [&loc](auto c) { return !std::isprint(c, loc); },
                        '.');

        return str;
      });

      // Print the character string representation of the byte line
      const size_t blanks = 40                 // byte line length
                            - (i % 16 + 1) * 2 // space taken up by byte pairs
                            - (i % 16) / 2;    // space taken up by spacing between byte pairs
      out << std::string(blanks + 1, ' ') << str << '\n';
    }
  }

  return out;
}

#endif // DAQ_UTILITIES_BINARY_HPP
