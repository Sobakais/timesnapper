#pragma once
#include <mpg123.h>

#include <utils.hpp>
#include <vector>

class AudioProcessor {
 private:
  Config cfg;
  mpg123_handle *mh = nullptr;
  size_t sample_rate;

  bool marked_timestamps = false;
  std::vector<short> raw_buffer;
  std::vector<bool> signal_buffer;
  timestampPQ timestamps;

  void readToRaw();
  void smoothWave();
  void toSignal();
  void markTimestamps();

 public:
  void startProcessor() {
    readToRaw();
    smoothWave();
    toSignal();
    markTimestamps();
    marked_timestamps = true;
  }

  AudioProcessor(const Config &cfg);
  ~AudioProcessor();

  AudioProcessor(const AudioProcessor &) = delete;
  AudioProcessor &operator=(const AudioProcessor &) = delete;

  void processTimestampsCount();
  void processTimestampsThreashold();
};
