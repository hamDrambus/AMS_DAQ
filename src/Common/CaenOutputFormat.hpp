/**
 * Data format for readout of CAEN ADC boards to be sent over module connections.
 * Has some header info and adjustable number of channels. Each channel is two vectors
 * of values vs time data. The size of vectors is allowed to be different between channels.
 * The data type of vector is parameterized with templates, allowing double for precision and
 * integer (channel) values for lesser memory/transfer load. The templatestype can even be some
 * simple (i.e. copyable with memcpy) structs.
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "Utils/Ers.hpp"
#include "SerializableFormat.hpp"

using Binary = daqling::utilities::Binary;
using std::size_t;

template <class T>
class caen_output_data final : public SerializableFormat {
  // Want fast serialization by memcpy-ing vectors to void* buffer
  static_assert(std::is_trivially_copyable_v<T> == true, "caen_output_data: template parameter must be copyable with memcpy.");
  // vector<bool> is a special case in terms of memory allocation
  static_assert(std::is_same_v<T, bool> == false, "caen_output_data: template parameter can not be bool.");

public:
  using SerializableFormat::serialize;
  using SerializableFormat::deserialize;

  struct channel_data {
    uint16_t channel;
    std::vector<T> ys;
    std::vector<T> xs;
  };
  uint32_t event_number; // since device start
  uint64_t timestamp;
  std::string device;
  std::vector<channel_data> ch_data;
  caen_output_data() = default;
  
  caen_output_data(const caen_output_data &) = default;
  caen_output_data(caen_output_data &&) noexcept = default;
  caen_output_data &operator=(const caen_output_data &) = default;
  caen_output_data &operator=(caen_output_data &&) noexcept = default;

  // Must be implemented for trasfering by DAQling
  caen_output_data(const void *data, const size_t size) : SerializableFormat() {
    deserialize(data, size);
  }

  void clear(void) noexcept override {
    ch_data.clear();
    device.clear();
    timestamp = 0;
  }
  
protected:
  size_t serialize(Binary & dataBuffer, size_t dest) const override {
    size_t current_pos = dest;
    size_t sz;
    try {
      current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&event_number), sizeof(uint32_t));
      current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&timestamp), sizeof(uint64_t));
      
      // serialize string:
      sz = device.length();
      current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&sz), sizeof(size_t));
      current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(device.data()), sz * sizeof(char));

      // serialize per-channel data:
      sz = ch_data.size();
      current_pos = dataBuffer.memwrite(current_pos, &sz, sizeof(size_t));
      for (std::size_t i = 0, i_end_ = sz; i!=i_end_; ++i) {
        std::size_t xs_sz = ch_data[i].xs.size(), ys_sz = ch_data[i].ys.size();
        current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&ch_data[i].channel), sizeof(uint16_t));
        // serialize xs
        current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&xs_sz), sizeof(size_t));
        current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(ch_data[i].xs.data()), sizeof(T)*xs_sz);
        // serialize ys
        current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(&ys_sz), sizeof(size_t));
        current_pos = dataBuffer.memwrite(current_pos, reinterpret_cast<const void *>(ch_data[i].ys.data()), sizeof(T)*ys_sz);
      }
      return current_pos;
    } catch (const std::exception & e) {
      ERS_WARNING(std::string("Serialization failed after byte #") + std::to_string(current_pos)
            + ".\n" + e.what() + "\nData will not be sent.");
      dataBuffer.clear();
      return 0;
    }
  }

  // If actual data size (signature) does not change, then no memory re-allocation will occur
  // and only data copying will be done from byte buffer to this class.
  size_t deserialize(const Binary & dataBuffer, std::size_t dest) override {
    size_t current_pos = dest;
    size_t sz = 0;
    ERS_INFO("De-seriazing event, byte size = "<< dataBuffer.size());
    try {
      current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&event_number), sizeof(uint32_t));
      current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&timestamp), sizeof(uint64_t));
      ERS_INFO("De-seriazed event #" << event_number << " timestamp = " << timestamp);
      
      // de-serialize string:
      current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&sz), sizeof(size_t));
      device.resize(sz);
      current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(device.data()), sz*sizeof(char));

      // de-serialize per-channel data:
      current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&sz), sizeof(size_t));
      ch_data.resize(sz);
      for (std::size_t i = 0, i_end_ = sz; i!=i_end_; ++i) {
        std::size_t xs_sz = 0, ys_sz = 0;
        current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&ch_data[i].channel), sizeof(uint16_t));
        // de-serialize xs
        current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&xs_sz), sizeof(size_t));
        ERS_INFO("De-seriazed xs n_samples =" << xs_sz);
        ch_data[i].xs.resize(xs_sz);
        current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(ch_data[i].xs.data()), sizeof(T)*xs_sz);
        // de-serialize ys
        current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(&ys_sz), sizeof(size_t));
        ERS_INFO("De-seriazed ys n_samples =" << ys_sz);
        ch_data[i].ys.resize(ys_sz);
        current_pos = dataBuffer.memread(current_pos, reinterpret_cast<void *>(ch_data[i].ys.data()), sizeof(T)*ys_sz);
      }
      return current_pos;
    } catch (const std::exception & e) {
      ERS_WARNING(std::string("De-serialization failed after byte #") + std::to_string(current_pos)
            + ".\n" + e.what() + "\nData will not be recieved.");
      clear();
      return 0;
    }
  }
};
