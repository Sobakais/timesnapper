#include <AudioProcessor.hpp>
#include <fstream>
#include <stdexcept>

AudioProcessor::AudioProcessor(const Config &cfg) : cfg(cfg) {
  mpg123_init();
  int err = MPG123_OK;
  mh = mpg123_new(nullptr, &err);
}

AudioProcessor::~AudioProcessor() {
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
}

void AudioProcessor::readToRaw() {
  if (mpg123_open(mh, cfg.input_path.c_str()) != MPG123_OK) {
    throw std::runtime_error("Failed to open MP3 file: " + cfg.input_path);
  }

  int channels;

  if (mpg123_getformat(mh, (long *)&sample_rate, &channels, NULL) !=
      MPG123_OK) {
    throw std::runtime_error("Failed to get MP3 format.");
  }

  mpg123_format_none(mh);
  if (mpg123_format(mh, sample_rate, channels, MPG123_ENC_SIGNED_16) !=
      MPG123_OK) {
    throw std::runtime_error("Failed to set output format to 16-bit.");
  }

  size_t buffer_size = mpg123_outblock(mh);
  std::vector<unsigned char> buffer(buffer_size);
  size_t done = 0;

  while (mpg123_read(mh, buffer.data(), buffer_size, &done) == MPG123_OK) {
    size_t numSamples = done / 2;
    short *samples = reinterpret_cast<short *>(buffer.data());

    if (channels == 1) {
      raw_buffer.insert(raw_buffer.end(), samples, samples + numSamples);
    } else {
      size_t numFrames = numSamples / channels;
      for (size_t frame = 0; frame < numFrames; frame++) {
        int sum = 0;
        for (int ch = 0; ch < channels; ch++) {
          sum += samples[frame * channels + ch];
        }
        short monoSample = static_cast<short>(sum / channels);
        raw_buffer.push_back(monoSample);
      }
    }
  }
}

void AudioProcessor::smoothWave() {
  size_t n = raw_buffer.size();
  size_t window_size = cfg.wave_smoother_window_size;
  size_t window_half = window_size / 2;
  size_t window_sum{0};

  std::vector<short> smoothed(n);

  for (size_t i = 0; i < window_size && i < n; i++) {
    window_sum += std::abs(raw_buffer[i]);
  }

  for (size_t i = 0; i < window_half && i < n; i++) {
    smoothed[i] = window_sum / window_size;
  }

  for (size_t i = window_half; i < n - window_half - 1; i++) {
    smoothed[i] = window_sum / window_size;

    window_sum += std::abs(raw_buffer[i + window_half + 1]);
    window_sum -= std::abs(raw_buffer[i - window_half]);
  }

  for (size_t i = n - window_half; i < n; i++) {
    smoothed[i] = window_sum / window_size;
  }

  std::swap(raw_buffer, smoothed);
}

void AudioProcessor::toSignal() {
  float mvalue{0};
  for (const auto &value : raw_buffer) {
    mvalue = std::max(static_cast<float>(value), mvalue);
  }

  signal_buffer.resize(raw_buffer.size());
  for (size_t i = 0; i < signal_buffer.size(); ++i) {
    signal_buffer[i] =
        ((static_cast<float>(raw_buffer[i]) / mvalue) > cfg.signal_threshold);
  }
}

void AudioProcessor::markTimestamps() {
  size_t last_mark = 0;

  for (size_t frame = 1; frame < signal_buffer.size(); ++frame) {
    if (signal_buffer[frame - 1] && !signal_buffer[frame]) {
      last_mark = frame;
    }
    if (!signal_buffer[frame - 1] && signal_buffer[frame]) {
      if (frame - last_mark < cfg.time_marker_min_gap_size) continue;
      timestamps.emplace(frame, frame - last_mark);
    }
  }
}

void AudioProcessor::processTimestampsCount() {
  if (!marked_timestamps) startProcessor();

  size_t cnt = cfg.amount_of_timestamps;
  std::ofstream out(cfg.output_path);
  std::ofstream wave("wave.csv");

  std::string del = cfg.table_delimiter;

  out << "FRAME" << del << "TIME\n";
  while (!timestamps.empty() && cnt--) {
    out << timestamps.top().first << del
        << (static_cast<float>(timestamps.top().first) / sample_rate) << '\n';
    timestamps.pop();
  }

  wave << "FRAME" << del << "RAW" << del << "SMOOTH" << del << "SIG\n";
  for (size_t i = 0; i < raw_buffer.size(); ++i) {
    wave << i << del << 0 << del << raw_buffer[i] << del << signal_buffer[i]
         << '\n';
  }

  out.close();
  wave.close();
}

void AudioProcessor::processTimestampsThreashold() {
  if (!marked_timestamps) startProcessor();

  std::ofstream out(cfg.output_path);
  std::ofstream wave("wave.csv");

  std::string del = cfg.table_delimiter;

  out << "FRAME" << del << "TIME\n";
  while (!timestamps.empty() &&
         timestamps.top().second > cfg.timestamps_threshold) {
    out << timestamps.top().first << del
        << (static_cast<float>(timestamps.top().first) / sample_rate) << '\n';
    timestamps.pop();
  }

  wave << "FRAME" << del << "RAW" << del << "SMOOTH" << del << "SIG\n";
  for (size_t i = 0; i < raw_buffer.size(); ++i) {
    wave << i << del << 0 << del << raw_buffer[i] << del << signal_buffer[i]
         << '\n';
  }

  out.close();
  wave.close();
}
