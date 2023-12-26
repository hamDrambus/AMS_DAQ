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

#include <fstream>
#include <map>
#include <filesystem>
#include "Core/DAQProcess.hpp"
#include "Utils/Binary.hpp"
#include "Utils/ReusableThread.hpp"
#include "folly/ProducerConsumerQueue.h"
#include "Common/CaenOutputFormat.hpp"

namespace fs = std::filesystem;
namespace daqling {
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(module, FormatError, "Filename generation for name pattern \""<<fname<<"\" failed.\n\tReason: "<<format_error,
                  ((std::string)fname)((std::string)format_error))
ERS_DECLARE_ISSUE(module, NoFilePermission, "Permission for file \"" << fname <<"\" denied.", ((std::string)fname))
ERS_DECLARE_ISSUE(module, OfstreamFail, "std::ofstream::fail()", ERS_EMPTY)
ERS_DECLARE_ISSUE(module, LogicFail, "Logic failure: the unreachable code was reached.", ERS_EMPTY)
ERS_DECLARE_ISSUE(module, InvalidParameter, "Invalid parameter \""<< par <<"\" was provided for "<<target<<".",
                  ((std::string)par)((std::string)target))
}
/**
 * Module for writing CAEN-acquired data to files.
 * Each DAQling channel (device connected to this class) is written and processed independently.
 * Synchronizing all devices is the duty of event builder, not this class.
 */
class CaenFileWriterModule : public daqling::core::DAQProcess {
public:
  CaenFileWriterModule(const std::string & /*n*/);

  void configure() override;
  void start(unsigned run_num) override;
  void stop() override;
  void runner() noexcept override;
  void unconfigure() override;
  void monitor_runner();

private:
  // Settings are provided for each device connected to the file writer (for each DAQling channel).
  struct Settings {
      enum FileFormat {
        Binary,
        Text,
        BinaryShort, // Write only y data. Use only when x step is fixed.
        TextShort,   // Write only y data. Use only when x step is fixed.
      } file_format = BinaryShort;
      enum FileSplitting {
        FilePerDevice,      // All data from each connection (device) is congregated to single file.
        FilePerDeviceHead,  // Same as FilePerDevice, but header event info is written to a separate file.
        FilePerChannel,     // Data both from each connection (device) and from each channel in it is written to its own file.
        FilePerChannelHead, // Same as FilePerChannel, but header event info is also written to separate file.
      } file_splitting = FilePerDevice;
      enum StopBehavior {
        Pause,        // Take no action after stopping writing events.
        CloseFiles,   // After stopping writing events close all files and proceed to the next ones.
        CloseAndTrim, // Same as above but deletes files if they do not have maximum (full) amount of events.
      } when_stopped_writing = Pause;
      enum FinishedRunBehavior {
        IgnoreLast, // Simply procced to write to new files, ignoring what was already written
        ClearLast,  // Deletes last written files if they do not have maximum (full) amount of events.
      } when_finished_run = IgnoreLast;
      enum NameConflictBehavior {
        Overwrite,      // Silently overwrite old files.
        NextFilenames,  // Generate new filenames until name conflict is resolved. If not possible, then stop.
        Stop,           // Stop all data writing and (TODO) trasnition to 'configured' state (or to 'error' state?).
      } when_name_conflict = Stop;
    std::string device_name;
    std::string filename_pattern;
    size_t max_events_per_file = 10000;
    // Will stop writing after reaching this limit
    // TODO: should transition the module into 'configured' state.
    size_t max_total_events = SIZE_MAX;

    static FileFormat file_format_from_string(const std::string& str, bool use_default) {
      if (str == "Text" || str == "text" || str == "txt") {
        return Settings::FileFormat::Text;
      } else if (str == "Binary" || str == "binary" || str == "bin") {
        return Settings::FileFormat::Binary;
      } else if (str == "BinaryShort" || str == "binary short" || str == "bin short") {
        return Settings::FileFormat::BinaryShort;
      } else if (str == "TextShort" || str == "text short" || str == "txt short") {
        return Settings::FileFormat::TextShort;
      } else {
        if (use_default)
          return Settings::FileFormat::BinaryShort;
      }
      throw daqling::module::InvalidParameter(ERS_HERE, str, std::string("Settings::FileFormat"));
    }
    static FileSplitting file_splitting_from_string(const std::string& str, bool use_default) {
      if (str == "FilePerDevice" || str == "PerDevice" || str == "per device" || str == "single" || str == "single file") {
        return Settings::FileSplitting::FilePerDevice;
      } else if (str == "FilePerDeviceHead" || str == "PerDeviceHead"
                || str == "per device with head" || str == "single+head"
                || str == "single file with head") {
        return Settings::FileSplitting::FilePerDeviceHead;
      } else if (str == "FilePerChannel" || str == "PerChannel" || str == "per channel" || str == "channel") {
        return Settings::FileSplitting::FilePerChannel;
      } else if (str == "FilePerChannelHead" || str == "PerChannelHead" || str == "per channel with head" || str == "channel+head") {
        return Settings::FileSplitting::FilePerChannelHead;
      } else {
        if (use_default)
          return Settings::FileSplitting::FilePerDevice;
      }
      throw daqling::module::InvalidParameter(ERS_HERE, str, std::string("Settings::FileSplitting"));
    }
    static NameConflictBehavior filename_conflict_from_string(const std::string& str, bool use_default) {
      if (str == "NextFilenames" || str == "next file" || str == "Next" || str == "next" || str == "skip") {
        return Settings::NameConflictBehavior::NextFilenames;
      } else if (str == "Overwrite" || str == "overwrite") {
        return Settings::NameConflictBehavior::Overwrite;
      } else if (str == "Stop" || str == "stop" || str == "abort") {
        return Settings::NameConflictBehavior::Stop;
      } else {
        if (use_default)
          return Settings::NameConflictBehavior::Stop;
      }
      throw daqling::module::InvalidParameter(ERS_HERE, str, std::string("Settings::NameConflictBehavior"));
    }
    static StopBehavior when_stopped_from_string(const std::string& str, bool use_default) {
      if (str == "Pause" || str == "pause" || str == "pause writing") {
        return Settings::StopBehavior::Pause;
      } else if (str == "CloseFiles" || str == "close files" || str == "next files" || str == "next file") {
        return Settings::StopBehavior::CloseFiles;
      } else if (str == "CloseAndTrim" || str == "clear last files" || str == "clear incomplete") {
        return Settings::StopBehavior::CloseAndTrim;
      } else {
        if (use_default)
          return Settings::StopBehavior::Pause;
      }
      throw daqling::module::InvalidParameter(ERS_HERE, str, std::string("Settings::StopBehavior"));
    }
    static FinishedRunBehavior when_finished_run_from_string(const std::string& str, bool use_default) {
      if (str == "IgnoreLast" || str == "close files" || str == "ignore last files") {
        return Settings::FinishedRunBehavior::IgnoreLast;
      } else if (str == "ClearLast" || str == "clear files" || str == "trim last files" || str == "clear last files") {
        return Settings::FinishedRunBehavior::ClearLast;
      } else {
        if (use_default)
          return Settings::FinishedRunBehavior::IgnoreLast;
      }
      throw daqling::module::InvalidParameter(ERS_HERE, str, std::string("Settings::FinishedRunBehavior"));
    }
  };

  struct WriteState {
    uint64_t chid;
    unsigned run_number;
    unsigned filenum;
    std::size_t num_events_written;
    std::size_t num_total_events_written;
    std::size_t last_event_written;
    std::vector<std::string> filenames;
    // There is some handling of the situation when these channels
    // varies from event to event even if it should not normally happen.
    std::vector<uint16_t> caen_channels;
    enum ErrorCode {
      None,
      FilenameConflictFilesystem,
      FilenameConflictGenerated,
      FilenamesNotChanged,
      FilestreamsNotOpened,
      MaxTotalEventsReached,
    } error_code;

    WriteState(uint64_t channel_id) :
        chid(channel_id), run_number(0), filenum(0), num_events_written(0),
        num_total_events_written(0), last_event_written(SIZE_MAX), filenames(),
        caen_channels(), error_code(None) {}

    void new_run(unsigned run, const Settings& settings)
    {
      if (run_number != run) {
        finish_run(settings);
        run_number = run;
      }
      error_code = (error_code == MaxTotalEventsReached) ? error_code : None;
    }

    void finish_run(const Settings& settings) {
      filenum = 0;
      num_events_written = 0;
      if (!filenames.empty() && settings.when_finished_run == Settings::FinishedRunBehavior::ClearLast) {
          ERS_DEBUG(0, "Finished run. Deleting files: "<<list_files(filenames));
          for (const auto& fn : filenames) {
            fs::path file_path(fn);
            std::error_code ec;
            fs::remove(file_path, ec);
          }
        }
      if (!filenames.empty()) {
        ERS_DEBUG(0, "Finished run. Last files were: "<<list_files(filenames));
      }
      filenames.clear();
      caen_channels.clear();
    }

    bool is_error(void) const { return error_code != None; }
  };

  using EventPointType = uint16_t;
  using EventDataType = caen_output_data<EventPointType>;
  using PayloadQueue = folly::ProducerConsumerQueue<SharedDataType<EventDataType>>;
  struct Context {
    Context(size_t queue_size, std::array<unsigned int, 2> tids, WriteState initial_state, const Settings chid_setings) :
        queue(queue_size), consumer(tids[0]), producer(tids[1]), write_state(initial_state), settings(chid_setings) {}
    PayloadQueue queue;
    daqling::utilities::ReusableThread consumer;
    daqling::utilities::ReusableThread producer;
    WriteState write_state;
    Settings settings;
  };

  struct Metrics {
    std::atomic<size_t> bytes_written = 0;
    std::atomic<size_t> payload_queue_size = 0;
    std::atomic<size_t> payload_size = 0;
  };

  std::atomic<bool> m_start_completed{true};
  std::atomic<bool> m_stop_completed{true};

  /// Output file generator with a named "var_is_{var}" filename pattern.
  /// fmt library is used for formatting. See https://fmt.dev/latest/syntax.html#grammar-token-sf-format_spec
  /// The example of file pattern with all parameters is:
  /// "{daqling_dir}/data_dir_{date}/{year}/{month}/{day}/{hour}-{minute}-{second}/{timestamp}_{time}_{device}_run{run}_ch{ch}_{filenum}.dat"
  /// where {date} is equivalent to {year}-{month}-{day}, {time} is equivalent to {hour}:{minute}:{second}, and
  /// {timestamp} is equivalent to {date}-{time}
  /// {daqling_dir} is root directory for daqling installation (DAQLING_LOCATION in cmake/setup.sh)
  /// {ch} is substituted to "head" for event header info
  /// (when FileSplitting::FilePerDeviceHead and FileSplitting::FilePerChannelHead are used).
  /// {ch} is substituted to empty string if all channels are written to the same file.
  /// {run} and {filenum} are formatted as numbers so leading zeros can be added:
  /// {run:03d} (will print 000, 001, etc).
  class FileGenerator {
  public:
    FileGenerator(const WriteState state, const Settings &settings)
        : m_state(state), m_settings(settings) {}

    /// Generates the next output file streams in the sequence.
    /// @warning May silently overwrites files if they already exist (including previous output files)
    /// depending on settings of the file writer.
    WriteState next();

  private:
    /// Generates the next output file names in the sequence.
    std::vector<std::string> next_fnames();

    /// Returns whether next filenames in sequence have name conflicts.
    /// Checks both conflicts with the filesystem and between themselves.
    /// If false is returned, appropriate error_code is set in m_state.
    bool check_name_conflicts (const std::vector<std::string> &fnames);

    /// Returns whether next filenames in sequence are all different from the previous once.
    /// This is to prevent endless loop in case NameConflictBehavior::NextFilenames setting is used.
    /// If false is returned, appropriate error_code is set in m_state.
    bool check_all_names_changed (const std::vector<std::string> &fnames);

  private:
    WriteState m_state;
    const Settings m_settings;
  };

  /// Opens filestreams for WriteState. Creates necessary directories if necessary.
  /// If failed, appropriate error_code is set in m_state.
  /// @warning Always overwrites existing files.
  static std::vector<std::ofstream> open_ofstreams(WriteState& state, const Settings &settings, bool append = false);

  static std::string list_files(const std::vector<std::string> &filenames) {
      std::stringstream ss;
      for(const auto& fn : filenames)
        ss << fn <<"\t\t\n";
      return ss.str();
    }

  /// Makes sure that event channels are the same as those in write state.
  /// @warning Can modify event data if channels are changed between events.
  static void normalize_event(const std::vector<uint16_t>& state, EventDataType* event);


  static void write_event_single_file_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_single_file_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_single_file_head_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_single_file_head_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_per_channel_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_per_channel_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_per_channel_head_text(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);
  static void write_event_per_channel_head_binary(EventDataType *data, std::vector<std::ofstream> & streams, bool is_short);

  // Configs
  std::map<uint64_t, Settings> m_channelSettings;
  std::map<uint64_t, WriteState> m_channelStates;
  uint64_t m_channels = 0;

  // Thread control
  std::atomic<bool> m_stopWriters;

  // Metrics
  mutable std::map<uint64_t, Metrics> m_channelMetrics;

  // Internals
  void flusher(uint64_t chid, Context &context) const;
  std::map<uint64_t, Context> m_channelContexts;
  std::thread m_monitor_thread;
};
