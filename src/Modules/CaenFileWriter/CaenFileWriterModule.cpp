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


#include <utility>
#include <sstream>
#include <set>
#include <cstdlib>
#include <fmt/core.h>
#include <fmt/format.h>
#include <filesystem>
#include "Utils/Common.hpp"
#include "Utils/Ers.hpp"
#include "CaenFileWriterModule.hpp"

namespace fs = std::filesystem;
using namespace std::chrono_literals;
namespace daqutils = daqling::utilities;
using namespace daqling::module;

CaenFileWriterModule::WriteState CaenFileWriterModule::FileGenerator::next()
{
  auto failed_return = [&]() {
    m_state.filenames.clear();
    return m_state;
  };

  auto success_return = [&] (std::vector<std::string> &fnames) {
    m_state.error_code = WriteState::ErrorCode::None;
    m_state.filenames = fnames;
    return m_state;
  };

  std::string timestamp, date, year, month, day, time, hour, minute, second, device;
  std::time_t t = std::time(nullptr);
  tm* loctime = std::localtime(&t);
  char tstr[32];
  if (std::strftime(tstr, sizeof(tstr), "%Y %m %d %H %M %S", loctime) == 0u)
    throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Failed to format timestamp");
  std::istringstream ss(tstr);
  ss >> year >> month >> day >> hour >> minute >> second;
  date = year + "-" + month + "-" + day;
  time = hour + ":" + minute + ":" + second;
  timestamp = date + "-" + time;
  device = m_settings.device_name;

  std::vector<std::string> fnames_pattern;
  std::vector<std::string> channels;

  switch (m_settings.file_splitting)
  {
    case Settings::FileSplitting::FilePerDevice:
      fnames_pattern.resize(1, m_settings.filename_pattern);
      channels.resize(1, "");
      break;
    case Settings::FileSplitting::FilePerDeviceHead:
      fnames_pattern.resize(2, m_settings.filename_pattern);
      channels = {"head", ""};
      break;
    case Settings::FileSplitting::FilePerChannel:
      fnames_pattern.resize(m_state.caen_channels.size(), m_settings.filename_pattern);
      channels.reserve(m_state.caen_channels.size());
      for (const auto& ch : m_state.caen_channels)
        channels.push_back(std::to_string(ch));
      break;
    case Settings::FileSplitting::FilePerChannelHead:
      fnames_pattern.resize(m_state.caen_channels.size()+1, m_settings.filename_pattern);
      channels.reserve(m_state.caen_channels.size()+1);
      channels.push_back("head");
      for (const auto& ch : m_state.caen_channels)
        channels.push_back(std::to_string(ch));
      break;
    default:
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileSplitting cases was not implemented.");
  }

  char* cstr = getenv("DAQLING_LOCATION");
  std::string daqling_str = cstr == nullptr ? "./" : cstr;
  ERS_DEBUG(0, " Environment: {daqling_dir}="<<daqling_str);

  std::vector<std::string> fnames(fnames_pattern.size());
  while (true) {
    for (std::size_t i = 0, i_end_ = fnames_pattern.size(); i!=i_end_; ++i) {
      try {
        fnames[i] = fmt::format(fnames_pattern[i],
                    fmt::arg("daqling_dir", daqling_str),
                    fmt::arg("timestamp", timestamp),
                    fmt::arg("date", date),
                    fmt::arg("year", year),
                    fmt::arg("month", month),
                    fmt::arg("day", day),
                    fmt::arg("time", time),
                    fmt::arg("hour", hour),
                    fmt::arg("minute", minute),
                    fmt::arg("second", second),
                    fmt::arg("run", m_state.run_number),
                    fmt::arg("ch", channels[i]),
                    fmt::arg("filenum", m_state.filenum),
                    fmt::arg("device", m_settings.device_name));
      }
      catch (const fmt::format_error &e) {
        throw FormatError(ERS_HERE, fnames_pattern[i], e.what());
      }
    }
    ++m_state.filenum;
    if (!check_name_conflicts(fnames))
      break;
    if (m_state.error_code == WriteState::ErrorCode::FilenameConflictGenerated)
      return failed_return();
    if (m_settings.when_name_conflict == Settings::NameConflictBehavior::Overwrite)
      break;
    if (m_settings.when_name_conflict == Settings::NameConflictBehavior::Stop)
      return failed_return();
    if (!check_all_names_changed(fnames))
      return failed_return();
  }
  return success_return(fnames);
}

bool CaenFileWriterModule::FileGenerator::check_name_conflicts(const std::vector<std::string> &fnames)
{
  if (std::set<std::string>(fnames.begin(), fnames.end()).size() != fnames.size()) {
    m_state.error_code = WriteState::ErrorCode::FilenameConflictGenerated;
    ERS_DEBUG(0, "Proposed filenames are not unique: "<<list_files(fnames));
    return true;
  }
  for (const auto & fn : fnames) {
    fs::file_status status = fs::status(fn);
    if (fs::exists(status)) {
      m_state.error_code = WriteState::ErrorCode::FilenameConflictFilesystem;
      ERS_DEBUG(0, "Proposed file name \""<<fn<<"\" already exists on filesystem");
      return true;
    }
  }
  return false;
}

bool CaenFileWriterModule::FileGenerator::check_all_names_changed(const std::vector<std::string> &fnames)
{
  std::vector<std::string> all_names = fnames;
  all_names.insert(all_names.end(), m_state.filenames.begin(), m_state.filenames.end());
  if (std::set<std::string>(all_names.begin(), all_names.end()).size() != all_names.size()) {
    m_state.error_code = WriteState::ErrorCode::FilenamesNotChanged;
    ERS_DEBUG(0, "Some of proposed file names have not changed from the previous set of files.\n"
                  "Make sure that file name pattern includes {filenum}");
    return false;
  }
  return true;
}

std::vector<std::ofstream> CaenFileWriterModule::open_ofstreams(WriteState& state, const Settings &settings, bool append)
{
  std::vector<std::ofstream> ret;
  for (const auto & fn : state.filenames) {
    fs::path file_path(fn);
    fs::create_directories(file_path.parent_path());
    std::ofstream str(fn, (settings.file_format == Settings::FileFormat::Binary ? std::ios::binary : std::ios::out) |
                          (append ? std::ios::app : std::ios::out));
    if (!str.is_open()) {
      state.error_code = WriteState::ErrorCode::FilestreamsNotOpened;
      ret.clear();
      ERS_WARNING("Could not open file '"
                << fn
                << "'. The event will not be written and output will proceed to next files.");
      return ret;
    }
    ret.push_back(std::move(str));
  }
  return ret;
}


CaenFileWriterModule::CaenFileWriterModule(const std::string &n) : DAQProcess(n), m_stopWriters{false} {
  // Set up static resources...
  std::ios_base::sync_with_stdio(false);
}

void CaenFileWriterModule::configure() {
  DAQProcess::configure();
  // Read out required and optional configurations
  Settings default_settings;
  std::string str = getModuleSettings().value("file_format", "default");
  default_settings.file_format = Settings::file_format_from_string(str, true);
  str = getModuleSettings().value("when_name_conflict", "default");
  default_settings.when_name_conflict = Settings::filename_conflict_from_string(str, true);
  str = getModuleSettings().value("when_finished_run", "default");
  default_settings.when_finished_run = Settings::when_finished_run_from_string(str, true);
  str = getModuleSettings().value("when_stopped_writing", "default");
  default_settings.when_stopped_writing = Settings::when_stopped_from_string(str, true);
  str = getModuleSettings().value("file_splitting", "default");
  default_settings.file_splitting = Settings::file_splitting_from_string(str, true);
  default_settings.max_total_events = getModuleSettings().value("max_total_events", SIZE_MAX);
  default_settings.max_events_per_file = getModuleSettings().value("max_events_per_file", 1000u);
  default_settings.filename_pattern = getModuleSettings().value("filename_pattern",
                                "CAEN_{date}/{device}_run{run:02d}_ch{ch}_f{filenum:03d}.dat");

  if (getModuleSettings().contains("inputs")) {
    auto recvs = getModuleSettings()["inputs"];
    for (auto& [key, elem] : recvs.items()) {
      uint64_t ch = elem.at("chid");
      m_channelSettings[ch] = default_settings;
      if (elem.contains("device_name"))
        m_channelSettings[ch].device_name = elem.at("device_name");
      else
        m_channelSettings[ch].device_name = "dev" + std::to_string(ch);
      if (elem.contains("file_format"))
        m_channelSettings[ch].file_format = Settings::file_format_from_string(elem["file_format"], false);
      if (elem.contains("when_name_conflict"))
        m_channelSettings[ch].when_name_conflict = Settings::filename_conflict_from_string(elem["when_name_conflict"], false);
      if (elem.contains("when_finished_run"))
        m_channelSettings[ch].when_finished_run = Settings::when_finished_run_from_string(elem["when_finished_run"], false);
      if (elem.contains("when_stopped_writing"))
        m_channelSettings[ch].when_stopped_writing = Settings::when_stopped_from_string(elem["when_stopped_writing"], false);
      if (elem.contains("file_splitting"))
        m_channelSettings[ch].file_splitting = Settings::file_splitting_from_string(elem["file_splitting"], false);
      
      m_channelSettings[ch].max_total_events = elem.value("max_total_events", default_settings.max_total_events);
      m_channelSettings[ch].max_events_per_file = elem.value("max_events_per_file", default_settings.max_events_per_file);
      m_channelSettings[ch].filename_pattern = elem.value("filename_pattern", default_settings.filename_pattern);
    }
  }

  m_channels = m_config.getNumReceiverConnections(m_name);
  for (uint64_t chid = 0; chid < m_channels; ++chid) {
    // Construct default settings for missing chid.
    if (m_channelSettings.find(chid)==m_channelSettings.end()) {
      m_channelSettings[chid] = default_settings;
      m_channelSettings[chid].device_name = "dev" + std::to_string(chid);
    }
    // Contruct variables for metrics and writer states
    m_channelMetrics[chid];
    const auto & [ it, success ] = m_channelStates.emplace(chid, chid);
    assert(success);
  }

  ERS_DEBUG(0, "setup finished");

  if (m_statistics) {
    // Register statistical variables
    for (auto & [ chid, metrics ] : m_channelMetrics) {
      m_statistics->registerMetric<std::atomic<size_t>>(&metrics.bytes_written,
                                                        "BytesWritten_chid" + std::to_string(chid),
                                                        daqling::core::metrics::RATE);
      m_statistics->registerMetric<std::atomic<size_t>>(
          &metrics.payload_queue_size, "PayloadQueueSize_chid" + std::to_string(chid),
          daqling::core::metrics::LAST_VALUE);
      m_statistics->registerMetric<std::atomic<size_t>>(&metrics.payload_size,
                                                        "PayloadSize_chid" + std::to_string(chid),
                                                        daqling::core::metrics::AVERAGE);
    }
    ERS_DEBUG(0, "Metrics are setup");
  }
}

void CaenFileWriterModule::start(unsigned run_num) {
  while (!m_stop_completed)
    std::this_thread::sleep_for(1ms);
  m_start_completed.store(false);

  DAQProcess::start(run_num);
  m_stopWriters.store(false);
  unsigned int threadid = 11111;       // XXX: magic
  constexpr size_t queue_size = 100;   // XXX: magic

  for (uint64_t chid = 0; chid < m_channels; ++chid) {
    // For each connection channel, construct a context of a payload queue, a consumer thread, and a producer
    // thread, settings and writer state.
    m_channelStates.at(chid).new_run(run_num, m_channelSettings[chid]);
    std::array<unsigned int, 2> tids = {threadid++, threadid++};
    const auto & [ it, success ] =
        m_channelContexts.emplace(std::piecewise_construct,
        std::forward_as_tuple(chid),
        std::forward_as_tuple(queue_size, std::move(tids), m_channelStates.at(chid), m_channelSettings[chid])
        );
    assert(success);

    // Start the context's consumer thread.
    it->second.consumer.set_work(&CaenFileWriterModule::flusher, this, it->first, std::ref(it->second));
  }
  assert(m_channelContexts.size() == m_channels);

  m_monitor_thread = std::thread(&CaenFileWriterModule::monitor_runner, this);

  m_start_completed.store(true);
}

void CaenFileWriterModule::stop() {
  while (!m_start_completed)
    std::this_thread::sleep_for(1ms);
  m_stop_completed.store(false);

  DAQProcess::stop();
  m_stopWriters.store(true);
  for (auto & [ chid, ctx ] : m_channelContexts) {
    ERS_DEBUG(0, " stopping context[" << chid << "]");
    while (!ctx.consumer.get_readiness())
      std::this_thread::sleep_for(1ms);
    m_channelStates.at(chid) = ctx.write_state;
  }
  m_channelContexts.clear();

  if (m_monitor_thread.joinable())
    m_monitor_thread.join();
  m_stop_completed.store(true);
}

void CaenFileWriterModule::unconfigure() {
  for (auto & [key, state] : m_channelStates)
    state.finish_run(m_channelSettings.at(key));
  m_channelSettings.clear();
  m_channelMetrics.clear();
  m_channelSettings.clear();
  DAQProcess::unconfigure();
}

void CaenFileWriterModule::runner() noexcept {
  ERS_DEBUG(0, " Running...");

  while (!m_start_completed)
    std::this_thread::sleep_for(1ms);

  // Start the producer thread of each context
  for (auto &it : m_channelContexts) {
    it.second.producer.set_work([&]() {
      addTag();
      auto &pq = it.second.queue;
      while (m_run) {
        DataFragment<EventDataType> pl;
        while (!m_connections.sleep_receive(it.first, pl) && m_run) {
          if (m_statistics) {
            m_channelMetrics.at(it.first).payload_queue_size = pq.sizeGuess();
          }
        }
        if (m_run) {
          size_t size = pl.size();
          // ERS_DEBUG(0, " Received " << size << "B payload on channel: " << it.first);
          SharedDataType<EventDataType> pl_shared(std::move(pl));
          pl_shared.make_shared();
          while (!pq.write(pl_shared) && m_run) {
          } // try until successful append
          if (m_statistics) {
            m_channelMetrics.at(it.first).payload_size = size;
          }
        }
      }
    });
  }

  while (m_run)
    std::this_thread::sleep_for(1ms);

  ERS_DEBUG(0, " Runner stopped");
}

void CaenFileWriterModule::write_event_single_file_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  std::ofstream & out = streams[0];
  out<<data->event_number<<"\n";
  out<<data->timestamp<<"\n";
  //out<<data->device<<"\n";
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    out<<data->ch_data[i].channel<<"\n";
    if (!is_short)
      for (std::size_t j = 0, j_end_ = data->ch_data[i].xs.size(); j!= j_end_; ++j)
        out<<data->ch_data[i].xs[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    for (std::size_t j = 0, j_end_ = data->ch_data[i].ys.size(); j!= j_end_; ++j)
      out<<data->ch_data[i].ys[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
  }
  out<<"\n";
}

void CaenFileWriterModule::write_event_single_file_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  std::ofstream & out = streams[0];
  out<<data->event_number<<data->timestamp;
  // Not writing device name here.
  out<<data->ch_data.size();
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    out<<data->ch_data[i].channel;
    std::size_t xs_sz = data->ch_data[i].xs.size(), ys_sz = data->ch_data[i].ys.size();
    if (!is_short) {
      out<<xs_sz;
      out.write(reinterpret_cast<const char *>(data->ch_data[i].xs.data()), static_cast<std::streamsize>(sizeof(EventPointType)*xs_sz));
    }
    out<<ys_sz;
    out.write(reinterpret_cast<const char *>(data->ch_data[i].ys.data()), static_cast<std::streamsize>(sizeof(EventPointType)*ys_sz));
  }
}

void CaenFileWriterModule::write_event_single_file_head_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  std::ofstream & out_head = streams[0];
  std::ofstream & out = streams[1];
  out_head<<data->event_number<<"\n";
  out_head<<data->timestamp<<"\n";
  out_head<<data->device<<"\n";
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    out<<data->ch_data[i].channel<<"\n";
    if (!is_short)
      for (std::size_t j = 0, j_end_ = data->ch_data[i].xs.size(); j!= j_end_; ++j)
        out<<data->ch_data[i].xs[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    for (std::size_t j = 0, j_end_ = data->ch_data[i].ys.size(); j!= j_end_; ++j)
      out<<data->ch_data[i].ys[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
  }
  out<<"\n";
}

void CaenFileWriterModule::write_event_single_file_head_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  std::ofstream & out_head = streams[0];
  std::ofstream & out = streams[1];
  out_head<<data->event_number<<data->timestamp;
  // Not writing device name here.
  out<<data->ch_data.size();
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    out<<data->ch_data[i].channel;
    std::size_t xs_sz = data->ch_data[i].xs.size(), ys_sz = data->ch_data[i].ys.size();
    if (!is_short) {
      out<<xs_sz;
      out.write(reinterpret_cast<const char *>(data->ch_data[i].xs.data()), static_cast<std::streamsize>(sizeof(EventPointType)*xs_sz));
    }
    out<<ys_sz;
    out.write(reinterpret_cast<const char *>(data->ch_data[i].ys.data()), static_cast<std::streamsize>(sizeof(EventPointType)*ys_sz));
  }
}

void CaenFileWriterModule::write_event_per_channel_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  // Number of channels equals number of streams.
  for (std::size_t i = 0, i_end_ = streams.size(); i != i_end_; ++i) {
    streams[i]<<data->event_number<<"\n";
    streams[i]<<data->timestamp<<"\n";
    streams[i]<<data->device<<"\n";
    // streams[i]<<data->ch_data[i].channel<<"\n";
    if (!is_short)
      for (std::size_t j = 0, j_end_ = data->ch_data[i].xs.size(); j!= j_end_; ++j)
        streams[i]<<data->ch_data[i].xs[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    for (std::size_t j = 0, j_end_ = data->ch_data[i].ys.size(); j!= j_end_; ++j)
      streams[i]<<data->ch_data[i].ys[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    streams[i]<<"\n";
  }
}

void CaenFileWriterModule::write_event_per_channel_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  // Number of channels equals number of streams.
  for (std::size_t i = 0, i_end_ = streams.size(); i != i_end_; ++i) {
    streams[i]<<data->event_number<<data->timestamp;
    // Not writing device name and channel here.
    std::size_t xs_sz = data->ch_data[i].xs.size(), ys_sz = data->ch_data[i].ys.size();
    if (!is_short) {
      streams[i]<<xs_sz;
      streams[i].write(reinterpret_cast<const char *>(data->ch_data[i].xs.data()), static_cast<std::streamsize>(sizeof(EventPointType)*xs_sz));
    }
    streams[i]<<ys_sz;
    streams[i].write(reinterpret_cast<const char *>(data->ch_data[i].ys.data()), static_cast<std::streamsize>(sizeof(EventPointType)*ys_sz));
  }
}

void CaenFileWriterModule::write_event_per_channel_head_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  // Number of channels equals number of streams - 1.
  streams[0]<<data->event_number<<"\n";
  streams[0]<<data->timestamp<<"\n";
  streams[0]<<data->device<<"\n";
  streams[0]<<"\n";
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    if (!is_short)
      for (std::size_t j = 0, j_end_ = data->ch_data[i].xs.size(); j!= j_end_; ++j)
        streams[i+1]<<data->ch_data[i].xs[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    for (std::size_t j = 0, j_end_ = data->ch_data[i].ys.size(); j!= j_end_; ++j)
      streams[i+1]<<data->ch_data[i].ys[j]<< ((j == j_end_ - 1) ? "\n" : "\t");
    streams[i+1]<<"\n";
  }
}

void CaenFileWriterModule::write_event_per_channel_head_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short)
{
  // Number of channels equals number of streams - 1.
  streams[0]<<data->event_number<<data->timestamp;
  // Not writing device name here.
  for (std::size_t i = 0, i_end_ = data->ch_data.size(); i != i_end_; ++i) {
    std::size_t xs_sz = data->ch_data[i].xs.size(), ys_sz = data->ch_data[i].ys.size();
    if (!is_short) {
      streams[i+1]<<xs_sz;
      streams[i+1].write(reinterpret_cast<const char *>(data->ch_data[i].xs.data()), static_cast<std::streamsize>(sizeof(EventPointType)*xs_sz));
    }
    streams[i+1]<<ys_sz;
    streams[i+1].write(reinterpret_cast<const char *>(data->ch_data[i].ys.data()), static_cast<std::streamsize>(sizeof(EventPointType)*ys_sz));
  }
}

void CaenFileWriterModule::normalize_event(const std::vector<uint16_t>& channels, EventDataType* event)
{
  for (auto ch : channels) {
    bool insert = true;
    for (const auto& event_ch : event->ch_data) {
      if (event_ch.channel == ch) {
        insert = false;
        break;
      }
    } 
    if (insert)
      event->ch_data.emplace_back(ch);
  }
  std::remove_if(event->ch_data.begin(), event->ch_data.end(),
      [&channels](EventDataType::channel_data event_ch){
        bool erase = true;
        for (auto ch : channels) {
          if (ch == event_ch.channel) {
            erase = false;
            break;
          }
        }
        return erase;
      });
}

void CaenFileWriterModule::flusher(uint64_t chid, Context &context) const {
  addTag();
  std::vector<std::ofstream> streams;

  const auto flush = [chid](EventDataType *data, const Settings& settings, std::vector<std::ofstream> &streams) {
    std::vector<std::size_t> bytes_written(streams.size(), 0);
    std::vector<std::streampos> pos1(streams.size());
    for (std::size_t i = 0, i_end_ = streams.size(); i!=i_end_; ++i)
      pos1[i] = streams[i].tellp();

    switch (settings.file_splitting) {
      case Settings::FileSplitting::FilePerDevice:
        switch (settings.file_format) {
          case Settings::FileFormat::Text:
            write_event_single_file_text(data, streams, false);
            break;
          case Settings::FileFormat::Binary:
            write_event_single_file_binary(data, streams, false);
            break;
          case Settings::FileFormat::TextShort:
            write_event_single_file_text(data, streams, true);
            break;
          case Settings::FileFormat::BinaryShort:
            write_event_single_file_binary(data, streams, true);
            break;
          default:
            ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileFormat cases was not implemented.");
            throw LogicFail(ERS_HERE);
        }
        streams[0].flush();
        if (streams[0].fail()) {
          ERS_WARNING(" Write operation for channel " << chid << " of event " << data->event_number
                                                      << " failed!");
          throw OfstreamFail(ERS_HERE);
        }
        break;
      case Settings::FileSplitting::FilePerDeviceHead:
        switch (settings.file_format) {
          case Settings::FileFormat::Text:
            write_event_single_file_head_text(data, streams, false);
            break;
          case Settings::FileFormat::Binary:
            write_event_single_file_head_binary(data, streams, false);
            break;
          case Settings::FileFormat::TextShort:
            write_event_single_file_head_text(data, streams, true);
            break;
          case Settings::FileFormat::BinaryShort:
            write_event_single_file_head_binary(data, streams, true);
            break;
          default:
            ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileFormat cases was not implemented.");
            throw LogicFail(ERS_HERE);
        }
        streams[0].flush();
        streams[1].flush();
        if (streams[0].fail() || streams[1].fail()) {
          ERS_WARNING(" Write operation for channel " << chid << " of event " << data->event_number
                                                      << " failed!");
          throw OfstreamFail(ERS_HERE);
        }
        break;
      case Settings::FileSplitting::FilePerChannel:
        switch (settings.file_format) {
          case Settings::FileFormat::Text:
            write_event_per_channel_text(data, streams, false);
            break;
          case Settings::FileFormat::Binary:
            write_event_per_channel_binary(data, streams, false);
            break;
          case Settings::FileFormat::TextShort:
            write_event_per_channel_text(data, streams, true);
            break;
          case Settings::FileFormat::BinaryShort:
            write_event_per_channel_binary(data, streams, true);
            break;
          default:
            ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileFormat cases was not implemented.");
            throw LogicFail(ERS_HERE);
        }
        for (std::size_t i = 0, i_end_ = streams.size(); i != i_end_; ++i) {
          streams[i].flush();
          if (streams[i].fail()) {
            ERS_WARNING(" Write operation for chid " << chid << " and channel " << i << " of event "
                                                     << data->event_number << " failed!");
            throw OfstreamFail(ERS_HERE);
          }
        }
        break;
      case Settings::FileSplitting::FilePerChannelHead:
        switch (settings.file_format) {
          case Settings::FileFormat::Text:
            write_event_per_channel_head_text(data, streams, false);
            break;
          case Settings::FileFormat::Binary:
            write_event_per_channel_head_binary(data, streams, false);
            break;
          case Settings::FileFormat::TextShort:
            write_event_per_channel_head_text(data, streams, true);
            break;
          case Settings::FileFormat::BinaryShort:
            write_event_per_channel_head_binary(data, streams, true);
            break;
          default:
            ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileFormat cases was not implemented.");
            throw LogicFail(ERS_HERE);
        }
        for (std::size_t i = 0, i_end_ = streams.size(); i != i_end_; ++i) {
          streams[i].flush();
          if (streams[i].fail()) {
            ERS_WARNING(" Write operation for chid " << chid << " and channel " << i << " of event "
                                                     << data->event_number << " failed!");
            throw OfstreamFail(ERS_HERE);
          }
        }
        break;
      default:
        ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::FileSplitting cases was not implemented.");
        throw LogicFail(ERS_HERE);
    }

    for (std::size_t i = 0, i_end_ = streams.size(); i!=i_end_; ++i)
      bytes_written[i] = static_cast<std::size_t>(streams[i].tellp()-pos1[i]);
    return bytes_written;
  };

  const Settings& settings = context.settings;
  WriteState& state = context.write_state;
  bool continuing_after_pause = !state.filenames.empty();
  while (!m_stopWriters) {
    while (context.queue.isEmpty() && !m_stopWriters) { // wait until we have something to write
      std::this_thread::sleep_for(1ms);
    };
    if (m_stopWriters) {
      streams.clear();
      switch (settings.when_stopped_writing) {
      case Settings::StopBehavior::Pause:
        ERS_DEBUG(0, " Paused writing. Files are closed.");
        break; // Filenames are not cleared.
      case Settings::StopBehavior::CloseFiles:
        state.filenames.clear();
        if (state.num_events_written > 0)
          ++state.filenum; // Go to next file if re-running this function
        ERS_DEBUG(0, " Stopped writing. Files are closed.");
        break;
      case Settings::StopBehavior::CloseAndTrim:
        if (state.num_events_written > 0 && state.num_events_written < settings.max_events_per_file) {
          for (const auto& fn : state.filenames) {
            fs::path file_path(fn);
            std::error_code ec;
            fs::remove(file_path, ec);
          }
          if (state.filenum > 0)
            --state.filenum;
          ERS_DEBUG(0, " Stopped writing. Last files are deleted.");
        }
        state.filenames.clear();
        ERS_DEBUG(0, " Stopped writing. Files are closed.");
        break;
      default:
        ERS_WARNING(std::string(__PRETTY_FUNCTION__) + " One of Settings::StopBehavior cases was not implemented.");
        throw LogicFail(ERS_HERE);
      }
      return;
    }

    auto payload = context.queue.frontPtr();
    EventDataType* event = payload->get();
    std::vector<std::size_t> bytes_written;
    std::size_t total_bytes_written = 0;

    if (nullptr == event || event->ch_data.empty()) // Ignore empty event
      goto skip;
    if (state.is_error()) // TODO: should stop the writer, but it is not possible currently.
      goto skip;
    // If first non-empty event, get channel list.
    // Otherwise, make sure event has only correct channels (same as first non-empty event).
    if (state.caen_channels.empty()) {
      state.caen_channels.reserve(event->ch_data.size());
      for (const auto& ch : event->ch_data)
        state.caen_channels.push_back(ch.channel);
    } else {
      normalize_event(state.caen_channels, event);
    }
    if (state.filenames.empty()) {
      FileGenerator gen(state, settings);
      state = gen.next();
      ERS_DEBUG(0, " Generated files: "<<list_files(state.filenames));
      if (state.is_error()) {
        // TODO: should stop writing. Note: can't call stop() as it will deadlock.
        // And in addition module manager won't be aware of state change.
        // For now, if writing is in error state, writing is simply skipped in this loop.
        goto skip;
      }
      // Non-error state here guarantees that file names are generated.
    }
    // If necessary, open streams. In case the writing was
    // stopped and continued, open streams in append mode.
    if (streams.empty()) {
      streams = open_ofstreams(state, settings, continuing_after_pause);
      if (continuing_after_pause)
        ERS_DEBUG(0, " Re-opened "<<streams.size()<<" file streams after pausing writing.");
      else
        ERS_DEBUG(0, " Opened "<<streams.size()<<" file streams.");
      continuing_after_pause = false;
      if (state.is_error()) {
        state.filenames.clear();
        goto skip;
      }
    }

    bytes_written = flush(event, context.settings, streams);
    ++state.num_events_written;
    ++state.num_total_events_written;
    state.last_event_written = event->event_number;

    if (state.num_events_written >= settings.max_events_per_file) { // Rotate output files
      ERS_DEBUG(0, " Rotating output files for channel " << chid);
      streams.clear();
      state.filenames.clear();
      state.num_events_written = 0;
    }
    if (state.num_total_events_written >= settings.max_total_events) {
      ERS_WARNING(" Reached maximum total events for channel " << chid);
      state.error_code = WriteState::ErrorCode::MaxTotalEventsReached;
      streams.clear();
      state.filenames.clear();
      // TODO: stop
    }
    
    for (const auto& b : bytes_written)
      total_bytes_written += b;
    ERS_DEBUG(0, " Wrote event #"<<event->event_number<<" ("<<total_bytes_written<<" bytes)");
    m_channelMetrics.at(chid).bytes_written += total_bytes_written;
  skip:
    // We are done with the payload; destruct it.
    context.queue.popFront();
  }
}

void CaenFileWriterModule::monitor_runner() {
  addTag();
  std::map<uint64_t, uint64_t> prev_value;
  while (m_run) {
    std::this_thread::sleep_for(1s);
    for (auto & [ chid, metrics ] : m_channelMetrics) {
      // ERS_DEBUG(0, "Bytes written (channel "
      //         << chid
      //         << "): " << static_cast<double>(metrics.bytes_written - prev_value[chid]) / 1000000
      //         << " MBytes/s");
      prev_value[chid] = metrics.bytes_written;
    }
  }
}
