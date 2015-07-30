// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/cpu_data_collector.h"

#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "chrome/browser/chromeos/power/power_data_collector.h"
#include "content/public/browser/browser_thread.h"

namespace chromeos {

namespace {
// The sampling of CPU idle or CPU freq data should not take more than this
// limit.
const int kSamplingDurationLimitMs = 500;

// The CPU data is sampled every |kCpuDataSamplePeriodSec| seconds.
const int kCpuDataSamplePeriodSec = 30;

// The value in the file /sys/devices/system/cpu/cpu<n>/online which indicates
// that CPU-n is online.
const int kCpuOnlineStatus = 1;

// The base of the path to the files and directories which contain CPU data in
// the sysfs.
const char kCpuDataPathBase[] = "/sys/devices/system/cpu";

// Suffix of the path to the file listing the range of possible CPUs on the
// system.
const char kPossibleCpuPathSuffix[] = "/possible";

// Format of the suffix of the path to the file which contains information
// about a particular CPU being online or offline.
const char kCpuOnlinePathSuffixFormat[] = "/cpu%d/online";

// Format of the suffix of the path to the file which contains freq state
// information of a CPU.
const char kCpuFreqTimeInStatePathSuffixFormat[] =
    "/cpu%d/cpufreq/stats/time_in_state";

// Format of the suffix of the path to the directory which contains information
// about an idle state of a CPU on the system.
const char kCpuIdleStateDirPathSuffixFormat[] = "/cpu%d/cpuidle/state%d";

// Format of the suffix of the path to the file which contains the name of an
// idle state of a CPU.
const char kCpuIdleStateNamePathSuffixFormat[] = "/cpu%d/cpuidle/state%d/name";

// Format of the suffix of the path which contains information about time spent
// in an idle state on a CPU.
const char kCpuIdleStateTimePathSuffixFormat[] = "/cpu%d/cpuidle/state%d/time";

// Returns the index at which |str| is in |vector|. If |str| is not present in
// |vector|, then it is added to it before its index is returned.
size_t IndexInVector(const std::string& str,
                  std::vector<std::string>* vector) {
  for (size_t i = 0; i < vector->size(); ++i) {
    if (str == (*vector)[i])
      return i;
  }

  // If this is reached, then it means |str| is not present in vector.  Add it.
  vector->push_back(str);
  return vector->size() - 1;
}

// Returns true if the |i|-th CPU is online; false otherwise.
bool CpuIsOnline(const int i) {
  const std::string online_file_format = base::StringPrintf(
      "%s%s", kCpuDataPathBase, kCpuOnlinePathSuffixFormat);
  const std::string cpu_online_file = base::StringPrintf(
      online_file_format.c_str(), i);
  if (!base::PathExists(base::FilePath(cpu_online_file))) {
    // If the 'online' status file is missing, then it means that the CPU is
    // not hot-pluggable and hence is always online.
    return true;
  }

  int online;
  std::string cpu_online_string;
  if (base::ReadFileToString(base::FilePath(cpu_online_file),
                             &cpu_online_string)) {
    base::TrimWhitespace(cpu_online_string, base::TRIM_ALL, &cpu_online_string);
    if (base::StringToInt(cpu_online_string, &online))
      return online == kCpuOnlineStatus;
  }

  LOG(ERROR) << "Bad format or error reading " << cpu_online_file << ". "
             << "Assuming offline.";
  return false;
}

// Samples the CPU idle state information from sysfs. |cpu_count| is the number
// of possible CPUs on the system. Sample at index i in |idle_samples|
// corresponds to the idle state information of the i-th CPU.
void SampleCpuIdleData(
    int cpu_count,
    std::vector<std::string>* cpu_idle_state_names,
    std::vector<CpuDataCollector::StateOccupancySample>* idle_samples) {
  base::Time start_time = base::Time::Now();
  for (int cpu = 0; cpu < cpu_count; ++cpu) {
    CpuDataCollector::StateOccupancySample idle_sample;
    idle_sample.time = base::Time::Now();
    idle_sample.time_in_state.reserve(cpu_idle_state_names->size());

    if (!CpuIsOnline(cpu)) {
      idle_sample.cpu_online = false;
    } else {
      idle_sample.cpu_online = true;

      const std::string idle_state_dir_format = base::StringPrintf(
          "%s%s", kCpuDataPathBase, kCpuIdleStateDirPathSuffixFormat);
      for (int state_count = 0; ; ++state_count) {
        std::string idle_state_dir = base::StringPrintf(
            idle_state_dir_format.c_str(), cpu, state_count);
        // This insures us from the unlikely case wherein the 'cpuidle_stats'
        // kernel module is not loaded. This could happen on a VM.
        if (!base::DirectoryExists(base::FilePath(idle_state_dir)))
          break;

        const std::string name_file_format = base::StringPrintf(
            "%s%s", kCpuDataPathBase, kCpuIdleStateNamePathSuffixFormat);
        const std::string name_file_path = base::StringPrintf(
            name_file_format.c_str(), cpu, state_count);
        DCHECK(base::PathExists(base::FilePath(name_file_path)));

        const std::string time_file_format = base::StringPrintf(
            "%s%s", kCpuDataPathBase, kCpuIdleStateTimePathSuffixFormat);
        const std::string time_file_path = base::StringPrintf(
            time_file_format.c_str(), cpu, state_count);
        DCHECK(base::PathExists(base::FilePath(time_file_path)));

        std::string state_name, occupancy_time_string;
        int64 occupancy_time_usec;
        if (!base::ReadFileToString(base::FilePath(name_file_path),
                                    &state_name) ||
            !base::ReadFileToString(base::FilePath(time_file_path),
                                    &occupancy_time_string)) {
          // If an error occurs reading/parsing single state data, drop all the
          // samples as an incomplete sample can mislead consumers of this
          // sample.
          LOG(ERROR) << "Error reading idle state from "
                     << idle_state_dir << ". Dropping sample.";
          idle_samples->clear();
          return;
        }

        base::TrimWhitespace(state_name, base::TRIM_ALL, &state_name);
        base::TrimWhitespace(
            occupancy_time_string, base::TRIM_ALL, &occupancy_time_string);
        if (base::StringToInt64(occupancy_time_string, &occupancy_time_usec)) {
          // idle state occupancy time in sysfs is recorded in microseconds.
          int64 time_in_state_ms = occupancy_time_usec / 1000;
          size_t index = IndexInVector(state_name, cpu_idle_state_names);
          if (index >= idle_sample.time_in_state.size())
            idle_sample.time_in_state.resize(index + 1);
          idle_sample.time_in_state[index] = time_in_state_ms;
        } else {
          LOG(ERROR) << "Bad format in " << time_file_path << ". "
                     << "Dropping sample.";
          idle_samples->clear();
          return;
        }
      }
    }

    idle_samples->push_back(idle_sample);
  }

  // If there was an interruption in sampling (like system suspended),
  // discard the samples!
  int64 delay =
      base::TimeDelta(base::Time::Now() - start_time).InMilliseconds();
  if (delay > kSamplingDurationLimitMs) {
    idle_samples->clear();
    LOG(WARNING) << "Dropped an idle state sample due to excessive time delay: "
                 << delay << "milliseconds.";
  }
}

// Samples the CPU freq state information from sysfs. |cpu_count| is the number
// of possible CPUs on the system. Sample at index i in |freq_samples|
// corresponds to the freq state information of the i-th CPU.
void SampleCpuFreqData(
    int cpu_count,
    std::vector<std::string>* cpu_freq_state_names,
    std::vector<CpuDataCollector::StateOccupancySample>* freq_samples) {
  base::Time start_time = base::Time::Now();
  for (int cpu = 0; cpu < cpu_count; ++cpu) {
    CpuDataCollector::StateOccupancySample freq_sample;
    freq_sample.time_in_state.reserve(cpu_freq_state_names->size());

    if (!CpuIsOnline(cpu)) {
      freq_sample.time = base::Time::Now();
      freq_sample.cpu_online = false;
    } else {
      freq_sample.cpu_online = true;

      const std::string time_in_state_path_format = base::StringPrintf(
          "%s%s", kCpuDataPathBase, kCpuFreqTimeInStatePathSuffixFormat);
      const std::string time_in_state_path = base::StringPrintf(
          time_in_state_path_format.c_str(), cpu);
      if (!base::PathExists(base::FilePath(time_in_state_path))) {
        // If the path to the 'time_in_state' for a single CPU is missing,
        // then 'time_in_state' for all CPUs is missing. This could happen
        // on a VM where the 'cpufreq_stats' kernel module is not loaded.
        LOG_IF(ERROR, base::SysInfo::IsRunningOnChromeOS())
            << "CPU freq stats not available in sysfs.";
        freq_samples->clear();
        return;
      }

      std::string time_in_state_string;
      // Note time as close to reading the file as possible. This is not
      // possible for idle state samples as the information for each state there
      // is recorded in different files.
      base::Time now = base::Time::Now();
      if (!base::ReadFileToString(base::FilePath(time_in_state_path),
                                  &time_in_state_string)) {
        LOG(ERROR) << "Error reading " << time_in_state_path << ". "
                   << "Dropping sample.";
        freq_samples->clear();
        return;
      }

      freq_sample.time = now;

      std::vector<base::StringPiece> lines =
          base::SplitStringPiece(time_in_state_string, "\n",
                                 base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
      // The last line could end with '\n'. Ignore the last empty string in
      // such cases.
      size_t state_count = lines.size();
      if (state_count > 0 && lines.back().empty())
        state_count -= 1;
      for (size_t state = 0; state < state_count; ++state) {
        int freq_in_khz;
        int64 occupancy_time_centisecond;

        // Occupancy of each state is in the format "<state> <time>"
        std::vector<base::StringPiece> pair = base::SplitStringPiece(
            lines[state], " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
        if (pair.size() == 2 &&
            base::StringToInt(pair[0], &freq_in_khz) &&
            base::StringToInt64(pair[1], &occupancy_time_centisecond)) {
          const std::string state_name = base::IntToString(freq_in_khz / 1000);
          size_t index = IndexInVector(state_name, cpu_freq_state_names);
          if (index >= freq_sample.time_in_state.size())
            freq_sample.time_in_state.resize(index + 1);
          // The occupancy time is in units of centiseconds.
          freq_sample.time_in_state[index] = occupancy_time_centisecond * 10;
        } else {
          LOG(ERROR) << "Bad format in " << time_in_state_path << ". "
                     << "Dropping sample.";
          freq_samples->clear();
          return;
        }
      }
    }

    freq_samples->push_back(freq_sample);
  }

  // If there was an interruption in sampling (like system suspended),
  // discard the samples!
  int64 delay =
      base::TimeDelta(base::Time::Now() - start_time).InMilliseconds();
  if (delay > kSamplingDurationLimitMs) {
    freq_samples->clear();
    LOG(WARNING) << "Dropped a freq state sample due to excessive time delay: "
                 << delay << "milliseconds.";
  }
}

// Samples CPU idle and CPU freq data from sysfs. This function should run on
// the blocking pool as reading from sysfs is a blocking task. Elements at
// index i in |idle_samples| and |freq_samples| correspond to the idle and
// freq samples of CPU i. This also function reads the number of CPUs from
// sysfs if *|cpu_count| < 0.
void SampleCpuStateOnBlockingPool(
    int* cpu_count,
    std::vector<std::string>* cpu_idle_state_names,
    std::vector<CpuDataCollector::StateOccupancySample>* idle_samples,
    std::vector<std::string>* cpu_freq_state_names,
    std::vector<CpuDataCollector::StateOccupancySample>* freq_samples) {
  DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (*cpu_count < 0) {
    // Set |cpu_count_| to 1. If it is something else, it will get corrected
    // later. A system will at least have one CPU. Hence, a value of 1 here
    // will serve as a default value in case of errors.
    *cpu_count = 1;
    const std::string possible_cpu_path = base::StringPrintf(
        "%s%s", kCpuDataPathBase, kPossibleCpuPathSuffix);
    if (!base::PathExists(base::FilePath(possible_cpu_path))) {
      LOG(ERROR) << "File listing possible CPUs missing. "
                 << "Defaulting CPU count to 1.";
    } else {
      std::string possible_string;
      if (base::ReadFileToString(base::FilePath(possible_cpu_path),
                                 &possible_string)) {
        int max_cpu;
        // The possible CPUs are listed in the format "0-N". Hence, N is present
        // in the substring starting at offset 2.
        base::TrimWhitespace(possible_string, base::TRIM_ALL, &possible_string);
        if (possible_string.find("-") != std::string::npos &&
            possible_string.length() > 2 &&
            base::StringToInt(possible_string.substr(2), &max_cpu)) {
          *cpu_count = max_cpu + 1;
        } else {
          LOG(ERROR) << "Unknown format in the file listing possible CPUs. "
                     << "Defaulting CPU count to 1.";
        }
      } else {
        LOG(ERROR) << "Error reading the file listing possible CPUs. "
                   << "Defaulting CPU count to 1.";
      }
    }
  }

  // Initialize the deques in the data vectors.
  SampleCpuIdleData(*cpu_count, cpu_idle_state_names, idle_samples);
  SampleCpuFreqData(*cpu_count, cpu_freq_state_names, freq_samples);
}

}  // namespace

// Set |cpu_count_| to -1 and let SampleCpuStateOnBlockingPool discover the
// correct number of CPUs.
CpuDataCollector::CpuDataCollector() : cpu_count_(-1), weak_ptr_factory_(this) {
}

CpuDataCollector::~CpuDataCollector() {
}

void CpuDataCollector::Start() {
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromSeconds(kCpuDataSamplePeriodSec),
               this,
               &CpuDataCollector::PostSampleCpuState);
}

void CpuDataCollector::PostSampleCpuState() {
  int* cpu_count = new int(cpu_count_);
  std::vector<std::string>* cpu_idle_state_names =
      new std::vector<std::string>(cpu_idle_state_names_);
  std::vector<StateOccupancySample>* idle_samples =
      new std::vector<StateOccupancySample>;
  std::vector<std::string>* cpu_freq_state_names =
      new std::vector<std::string>(cpu_freq_state_names_);
  std::vector<StateOccupancySample>* freq_samples =
      new std::vector<StateOccupancySample>;

  content::BrowserThread::PostBlockingPoolTaskAndReply(
      FROM_HERE,
      base::Bind(&SampleCpuStateOnBlockingPool,
                 base::Unretained(cpu_count),
                 base::Unretained(cpu_idle_state_names),
                 base::Unretained(idle_samples),
                 base::Unretained(cpu_freq_state_names),
                 base::Unretained(freq_samples)),
      base::Bind(&CpuDataCollector::SaveCpuStateSamplesOnUIThread,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Owned(cpu_count),
                 base::Owned(cpu_idle_state_names),
                 base::Owned(idle_samples),
                 base::Owned(cpu_freq_state_names),
                 base::Owned(freq_samples)));
}

void CpuDataCollector::SaveCpuStateSamplesOnUIThread(
    const int* cpu_count,
    const std::vector<std::string>* cpu_idle_state_names,
    const std::vector<CpuDataCollector::StateOccupancySample>* idle_samples,
    const std::vector<std::string>* cpu_freq_state_names,
    const std::vector<CpuDataCollector::StateOccupancySample>* freq_samples) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  cpu_count_ = *cpu_count;

  // |idle_samples| or |freq_samples| could be empty sometimes (for example, if
  // sampling was interrupted due to system suspension). Iff they are not empty,
  // they will have one sample each for each of the CPUs.

  if (!idle_samples->empty()) {
    // When committing the first sample, resize the data vector to the number of
    // CPUs on the system. This number should be the same as the number of
    // samples in |idle_samples|.
    if (cpu_idle_state_data_.empty()) {
      cpu_idle_state_data_.resize(idle_samples->size());
    } else {
      DCHECK_EQ(idle_samples->size(), cpu_idle_state_data_.size());
    }
    for (size_t i = 0; i < cpu_idle_state_data_.size(); ++i)
      AddSample(&cpu_idle_state_data_[i], (*idle_samples)[i]);

    cpu_idle_state_names_ = *cpu_idle_state_names;
  }

  if (!freq_samples->empty()) {
    // As with idle samples, resize the data vector before committing the first
    // sample.
    if (cpu_freq_state_data_.empty()) {
      cpu_freq_state_data_.resize(freq_samples->size());
    } else {
      DCHECK_EQ(freq_samples->size(), cpu_freq_state_data_.size());
    }
    for (size_t i = 0; i < cpu_freq_state_data_.size(); ++i)
      AddSample(&cpu_freq_state_data_[i], (*freq_samples)[i]);

    cpu_freq_state_names_ = *cpu_freq_state_names;
  }
}

CpuDataCollector::StateOccupancySample::StateOccupancySample()
    : cpu_online(false) {
}

CpuDataCollector::StateOccupancySample::~StateOccupancySample() {
}

}  // namespace chromeos
