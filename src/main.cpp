#include <mpg123.h>

#include <AudioProcessor.hpp>
#include <cstdlib>
#include <cxxopts.hpp>
#include <iostream>
#include <utils.hpp>

Config parse_args(int argc, char *argv[], bool &by_amount) {
  Config cfg;
  cxxopts::Options options(
      "timesnapper",
      "Tool to make timestamps that better fit speech lines in an MP3 file.\n"
      "Amount output will be by default.\n");
  options.positional_help("<inputfile>.mp3").show_positional_help();

  options.add_options()(
      "s,signal_threshold",
      "Minimum normalized amplitude to be considered as signal (default: 0.18)",
      cxxopts::value<float>())(
      "w,window_size",
      "Size of window in running sum smoother (default: 2000 frames)",
      cxxopts::value<size_t>())(
      "g,min_gap",
      "Minimum number of frames between timestamps to be considered as "
      "possible timestamp (default: 5000 frames)",
      cxxopts::value<size_t>())("t,timestamps_threshold",
                                "Minimum framecount threshold to include "
                                "timestamp",
                                cxxopts::value<size_t>())(
      "n,count",
      "Number of timestamps to extract (incompatible with "
      "--timestamps_threshold)",
      cxxopts::value<size_t>())("d,delimiter",
                                "Delimiter for output tables (default: ',')",
                                cxxopts::value<std::string>())(
      "o,output", "Output CSV file path (default: ts.csv)",
      cxxopts::value<std::string>())(
      "input", "Input file path", cxxopts::value<std::string>())("h,help",
                                                                 "Print usage");

  options.parse_positional({"input", "output"});
  auto result = options.parse(argc, argv);

  if (result.count("count") && result.count("timestamps_threshold")) {
    std::cout << "For work specify amount of timestamps OR threshold that will "
                 "be aplied to pick them, not both.";
    exit(-1);
  }

  if (result.count("timestamps_threshold")) {
    by_amount = false;
    cfg.timestamps_threshold = result["timestamps_threshold"].as<size_t>();
  }
  if (result.count("count")) {
    cfg.amount_of_timestamps = result["count"].as<size_t>();
  }
  if (result.count("window_size")) {
    cfg.wave_smoother_window_size = result["window_size"].as<size_t>();
  }
  if (result.count("min_gap")) {
    cfg.time_marker_min_gap_size = result["min_gap"].as<size_t>();
  }
  if (result.count("delimiter")) {
    cfg.table_delimiter = result["delimiter"].as<std::string>();
  }

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  if (!result.count("input")) {
    std::cout << "Not enough arguments.\nUsage: timesnap [options] "
                 "<inputfile>.mp3\n   For diplaying help use: timesnap -h\n";
    exit(-1);
  } else {
    cfg.input_path = result["input"].as<std::string>();
  }

  if (result.count("output")) {
    cfg.output_path = result["output"].as<std::string>();
  }

  return cfg;
}

int main(int argc, char *argv[]) {
  bool by_amount = true;
  Config cfg = parse_args(argc, argv, by_amount);
  AudioProcessor proc(cfg);

  try {
    proc.startProcessor();
  } catch (const std::exception &ex) {
    std::cerr << "Exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (by_amount) {
    std::cout << "By amount\n";
    proc.processTimestampsCount();
  } else {
    std::cout << "By TH\n";
    proc.processTimestampsThreashold();
  }

  return 0;
}
