/**
 * Abstract class (interface) for data types with serialization.
 * Serialization is the transfer of classes to raw byte (binary) array and back.
 * Initially DAQling supported trivial data types only (e.g. int, double, char payload[24000])
 * which do not require serialization as the pointer to structs containing such data can
 * work as byte array which fully represents both the data and structure state.
 * 
 * But if one wants to tranfer for example std:vector<double>, then the serialization is required.
 * 
 */

#pragma once

#include <optional>
#include "Utils/Binary.hpp"

using Binary = daqling::utilities::Binary;
using std::size_t;

class SerializableFormat {
protected:
  Binary m_byte_buffer;

public:
  SerializableFormat() = default;
  virtual ~SerializableFormat() = default;
  
  /// @brief Move constructor
  SerializableFormat(SerializableFormat &&rhs) noexcept : m_byte_buffer{std::move(rhs.m_byte_buffer)} {};

  /// @brief Move assignment
  SerializableFormat &operator=(SerializableFormat &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    m_byte_buffer = std::move(rhs.m_byte_buffer);
    return *this;
  }
  
  /// @brief Copy constructor
  SerializableFormat(const SerializableFormat &rhs) {
    m_byte_buffer = rhs.m_byte_buffer;
  }

  /// @brief Copy assignment
  SerializableFormat &operator=(const SerializableFormat &rhs) {
    if (this == &rhs) {
      return *this;
    }
    m_byte_buffer = rhs.m_byte_buffer;
    return *this;
  }

  /// @brief Deserialize function used in data reconstruction from (const void *data, const size_t size).
  /// This reconstruction occurs when recieving data from connections. Also refer to DataTypeWrapper.
  /// @param data
  /// @param size
  /// @return True on success
  bool deserialize (const void *data, const size_t size) {
    m_byte_buffer = Binary(data, size);

    if (m_byte_buffer.error()) { // Failed to allocate memory
      m_byte_buffer.clear();
      return false;
    }
    return deserialize();
  }

  // Clears all data. Necessary in case serialization falied and we want a clear state.
  virtual void clear(void) noexcept = 0;

  inline size_t size() { return m_byte_buffer.size(); }
  inline void *data() { return m_byte_buffer.data(); }

  /// @brief Synchronizes byte buffer and data by serializing the data to the buffer
  /// @return True on success
  bool serialize(void) {
    m_byte_buffer.reset();
    m_byte_buffer.reserve(serialize_size_hint());
    std::size_t buffer_pos = 0;
    buffer_pos = serialize(m_byte_buffer, buffer_pos);
    if (buffer_pos == 0) // Error in serializtion (likely failed to allocate memory)
      m_byte_buffer.clear();
    else
      m_byte_buffer.resize(buffer_pos); // Trims excess bytes if any are present
    return buffer_pos != 0;
  }

  /// @brief Synchronizes byte buffer and data by de-serializing the data using the buffer
  /// @return True on success
  bool deserialize (void) {
    std::size_t buffer_pos = 0;
    buffer_pos = deserialize(m_byte_buffer, buffer_pos);
    if (buffer_pos == 0) // Error in de-serializtion (likely failed to allocate memory)
      clear();
    return buffer_pos != 0;
  }

protected:
  /// Used to reserve byte buffer beforehand. Otherwise,
  /// necessary memory is allocated during serialization.
  virtual size_t serialize_size_hint() const {
      return 0;
  };

  /// @brief serializes this class into resizable byte buffer Binary starting with byte dest
  /// @param dataBuffer is the byte buffer
  /// @param dest is the starting byte in the buffer (allows appending data)
  /// @return new serialization position (position after the last written byte) on success and 0 on fail.
  virtual size_t serialize(Binary & dataBuffer, size_t dest) const = 0;

  /// @brief de-serializes this class from resizable byte buffer Binary starting with byte dest
  /// @param dataBuffer is the byte buffer
  /// @param dest is the starting byte in the buffer (allows reading after previous data)
  /// @return new de-serialization position (position after the last read byte) on success and 0 on fail.
  virtual size_t deserialize(const Binary & dataBuffer, size_t dest) = 0;
};
