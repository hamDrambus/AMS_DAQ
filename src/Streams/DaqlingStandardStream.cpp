/*
 *  StandardStream.cxx
 *  ers
 *
 *  Created by Serguei Kolos on 02.08.05.
 *  Copyright 2004 CERN. All rights reserved.
 *
 *  Modified by DAQling
 *  Copyright (C) 2021 CERN
 */

#include "DaqlingStandardStream.hpp"
#include <ers/SampleIssues.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

template <class Device> void ers::DaqlingStandardStream<Device>::write(const Issue &issue) {
  report(device().stream(), issue);
  chained().write(issue);
}
template <class Device>
void ers::DaqlingStandardStream<Device>::report(std::ostream &out, const Issue &issue, bool cause) {

  // time
  out << "[" << issue.time<std::chrono::milliseconds>("%Y-%m-%d %H:%M:%S") << "] ";
  // thread
  out << "[" << m_thread_tagger.readTag(issue.context().thread_id()) << "] ";
  // package
  out << "[" << issue.context().package_name() << "] ";
  // loglvl
  out << "[" << issue.severity() << "] ";
  // file & line
  out << "["
      << std::string(issue.context().file_name())
             .substr(std::string(issue.context().file_name()).find_last_of('/') + 1)
      << ":" << issue.context().line_number() << "] ";

#ifndef DONT_LOG_FUNCTION_NAME
  // function
  out << "[" << issue.context().function_name() << "] ";
#endif
  // message
  out << issue.message() << " ";

  if (cause) {
    out << "[CAUSE]";
  }

  out << std::endl;
  // chained issues
  if (issue.cause() != nullptr) {

    report(out, *issue.cause(), true);
  }
}
namespace {
struct OutDevice { // NOLINT(cppcoreguidelines-special-member-functions)
  explicit OutDevice(std::ostream &out) : out_(out) { ; }

  std::ostream &stream() const { return out_; }

  const OutDevice &device() { return *this; }

public:
  OutDevice(const OutDevice &) = delete;
  OutDevice &operator=(const OutDevice &) = delete;

private:
  std::ostream &out_;
};

struct ObjectLock { // NOLINT(cppcoreguidelines-special-member-functions)
protected:
  ObjectLock() = default;

  std::mutex &mutex() { return mutex_; }

public:
  ObjectLock(const ObjectLock &) = delete;
  ObjectLock &operator=(const ObjectLock &) = delete;

private:
  std::mutex mutex_;
};

template <int LockDiscriminator>
struct ClassLock { // NOLINT(cppcoreguidelines-special-member-functions)
protected:
  ClassLock() = default;

  std::mutex &mutex() {
    static auto *m = new std::mutex;
    return *m;
  }

public:
  ClassLock(const ClassLock &) = delete;
  ClassLock &operator=(const ClassLock &) = delete;
};

template <class L = ObjectLock> struct LockableDevice : public L, public OutDevice {
  using L::mutex;

  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
  struct LockedDevice : public OutDevice {
    LockedDevice(std::ostream &out, std::mutex &mutex) : OutDevice(out), m_lock(mutex) { ; }

  public:
    LockedDevice(const LockedDevice &) = delete;
    LockedDevice &operator=(const LockedDevice &) = delete;

  private:
    std::unique_lock<std::mutex> m_lock;
  };

  LockableDevice(std::ostream &out) : OutDevice(out) { ; }

  LockedDevice device() { return LockedDevice(stream(), mutex()); }
};

template <class D> struct OutputDevice : public D {
  OutputDevice(const std::string & /*unused*/ = "") : D(std::cout) { ; }
};

template <class D> struct ErrorDevice : public D {
  ErrorDevice(const std::string & /*unused*/ = "") : D(std::cerr) { ; }
};

template <class D> struct FileDevice : public D {
  FileDevice(const std::string &file_name) : D(out_), out_(file_name.c_str()) {
    if (!out_) {
      throw ers::CantOpenFile(ERS_HERE, file_name.c_str());
    }
  }

private:
  std::ofstream out_;
};
} // namespace

ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<FileDevice<OutDevice>>, "dfile", file_name)
ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<OutputDevice<OutDevice>>, "dstdout",
                           ERS_EMPTY)
ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<ErrorDevice<OutDevice>>, "dstderr", ERS_EMPTY)

ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<FileDevice<LockableDevice<>>>, "dlfile",
                           file_name)
ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<OutputDevice<LockableDevice<ClassLock<1>>>>,
                           "dlstdout", ERS_EMPTY)
ERS_REGISTER_OUTPUT_STREAM(ers::DaqlingStandardStream<ErrorDevice<LockableDevice<ClassLock<2>>>>,
                           "dlstderr", ERS_EMPTY)