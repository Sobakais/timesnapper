#include <mpg123.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <utility>
#include <utils.cpp>
#include <vector>

static float SIGNAL_THREASHOLD = 0.2f;
static size_t WAVE_SMOOTHER_WINDOW_SIZE = 400;
static size_t GAP_MINIMUM_FRAME_SIZE = 1000;

std::pair<std::vector<short>, long> readToRaw(const std::string &filename,
                                              mpg123_handle *mh) {
  if (mpg123_open(mh, filename.c_str()) != MPG123_OK) {
    mpg123_delete(mh);
    mpg123_exit();
    throw std::runtime_error("Failed to open MP3 file: " + filename);
  }

  long rate;
  int channels;
  int encoding;
  if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    throw std::runtime_error("Failed to get MP3 format.");
  }

  mpg123_format_none(mh);
  if (mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16) != MPG123_OK) {
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    throw std::runtime_error("Failed to set output format to 16-bit.");
  }

  size_t buffer_size = mpg123_outblock(mh);
  std::vector<unsigned char> buffer(buffer_size);
  size_t done = 0;

  std::vector<short> monoSamples;

  while (mpg123_read(mh, buffer.data(), buffer_size, &done) == MPG123_OK) {
    size_t numSamples = done / 2;
    short *samples = reinterpret_cast<short *>(buffer.data());

    if (channels == 1) {
      monoSamples.insert(monoSamples.end(), samples, samples + numSamples);
    } else {
      size_t numFrames = numSamples / channels;
      for (size_t frame = 0; frame < numFrames; frame++) {
        int sum = 0;
        for (int ch = 0; ch < channels; ch++) {
          sum += samples[frame * channels + ch];
        }
        short monoSample = static_cast<short>(sum / channels);
        monoSamples.push_back(monoSample);
      }
    }
  }

  mpg123_exit();

  return {monoSamples, rate};
}

std::vector<short> smoothWave(const std::vector<short> &raw,
                              size_t window_size) {
  size_t n = raw.size();
  size_t window_half = window_size / 2;
  size_t window_sum{0};

  std::vector<short> smoothed(n);

  for (size_t i = 0; i < window_size && i < n; i++) {
    window_sum += std::abs(raw[i]);
  }

  for (size_t i = 0; i < window_half && i < n; i++) {
    smoothed[i] = window_sum / window_size;
  }

  for (size_t i = window_half; i < n - window_half - 1; i++) {
    smoothed[i] = window_sum / window_size;

    window_sum += std::abs(raw[i + window_half + 1]);
    window_sum -= std::abs(raw[i - window_half]);
  }

  for (size_t i = n - window_half; i < n; i++) {
    smoothed[i] = window_sum / window_size;
  }

  return smoothed;
}

std::vector<bool> toSignal(const std::vector<short> &raw, float threshold) {
  float mvalue{0};
  for (const auto &value : raw) {
    mvalue = std::max(static_cast<float>(value), mvalue);
  }

  std::vector<bool> signal(raw.size());
  for (size_t i = 0; i < signal.size(); ++i) {
    signal[i] = ((static_cast<float>(raw[i]) / mvalue) > threshold);
  }

  return signal;
}

timestampPQ markTimestamps(const std::vector<bool> signal, size_t minimal_gap) {
  timestampPQ timestamps;
  size_t last_mark = 0;

  for (size_t frame = 1; frame < signal.size(); ++frame) {
    if (signal[frame - 1] && !signal[frame]) {
      last_mark = frame;
    }
    if (!signal[frame - 1] && signal[frame]) {
      if (frame - last_mark < minimal_gap) continue;
      timestamps.emplace(frame, frame - last_mark);
    }
  }

  return timestamps;
}

void processTimestampsCount(timestampPQ &ts, std::ofstream &out, size_t rate,
                            size_t cnt) {
  char del{','};
  out << "FRAME" << del << "TIME\n";
  while (!ts.empty() && --cnt) {
    out << ts.top().first << del << (static_cast<float>(ts.top().first) / rate)
        << '\n';
    ts.pop();
  }
}

void processTimestampsThreashold(timestampPQ &ts, std::ofstream &out,
                                 size_t rate, size_t th) {
  char del{','};
  out << "FRAME" << del << "TIME\n";
  while (!ts.empty() && ts.top().second > th) {
    out << ts.top().first << del << (ts.top().first / rate) << '\n';
    ts.pop();
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: ./mp3processor <filename.mp3>" << std::endl;
    return -1;
  }

  if (argc == 4) {
    SIGNAL_THREASHOLD = std::stof(argv[2]);
    WAVE_SMOOTHER_WINDOW_SIZE = std::stoul(argv[3]);
  }

  mpg123_init();
  int err = MPG123_OK;
  mpg123_handle *mh = nullptr;
  mh = mpg123_new(nullptr, &err);

  try {
    auto [raw, rate] = readToRaw(argv[1], mh);
    auto smooth = smoothWave(raw, WAVE_SMOOTHER_WINDOW_SIZE);
    auto sig = toSignal(smooth, SIGNAL_THREASHOLD);
    timestampPQ ts = markTimestamps(sig, GAP_MINIMUM_FRAME_SIZE);

    std::cout << "Processed successfully\nAmount of pcm frames: " << sig.size()
              << std::endl;

    std::ofstream timestamp("ts.csv");
    std::ofstream wave("wave.csv");

    char del{','};
    processTimestampsCount(ts, timestamp, rate, 10);
    wave << "FRAME" << del << "RAW" << del << "SMOOTH" << del << "SIG\n";
    for (size_t frame = 0; frame < sig.size(); ++frame) {
      wave << frame << del << raw[frame] << del << smooth[frame] << del
           << sig[frame] << '\n';
    }

    timestamp.close();
    wave.close();
  } catch (const std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
  return 0;
}
