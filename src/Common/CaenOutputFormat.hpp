/**
 * Data format for readout of CAEN ADC boards to be sent over module connections.
 * Has some header info and adjustable number of channels. Each channel is two vectors
 * of values vs time data. The size of vectors is allowed to be different between channels.
 * The data type of vector is parameterized with templates, allowing double for precision and
 * integer (channel) values for lesser memory/transfer load. 
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

template <class T>
struct caen_output_data {
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
  // Are these implementable? What to do with void *data()? Should new pointer to memory be created?
  // Or should I just go for new DataType with class serialization?
  caen_output_data(const void *data, const size_t size) { memcpy(this, data, size); }
  inline size_t size() { return sizeof(header) + header.payload_size; }
  inline void *data() { return this; }
};