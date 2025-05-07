#pragma once

#include <queue>
#include <string>

struct Compare {
  bool operator()(const std::pair<size_t, size_t> &lhs,
                  const std::pair<size_t, size_t> &rhs) {
    return lhs.second < rhs.second;
  }
};

using timestampPQ =
    std::priority_queue<std::pair<size_t, size_t>,
                        std::vector<std::pair<size_t, size_t>>, Compare>;

struct Config {
  float signal_threshold = 0.18f;
  size_t wave_smoother_window_size = 2000;
  size_t time_marker_min_gap_size = 5000;
  size_t timestamps_threshold;
  size_t amount_of_timestamps = 5;
  std::string table_delimiter = ",";
  std::string output_path = "ts.csv";
  std::string input_path;
};
