#include "../lib/include/sorter.h"
#include "../lib/include/tape.h"
#include "../utilities/include/file-guard.h"

#include <format>
#include <fstream>
#include <iostream>

constexpr std::string CALL_FORMAT = "tape-sort <input-file> <output-file> [input-tape-size] [memory-limit]";
constexpr std::string CONFIG_PATH = "config.txt";

bool parse_delays(tape::delay_config& config) {
  std::ifstream fconfig(CONFIG_PATH);
  if (!fconfig) {
    return true;
  }
  for (std::string line; std::getline(fconfig, line);) {
    std::stringstream linestream(line);
    if (line.empty()) continue;
    std::string key;
    size_t value;
    linestream >> key >> value;
    if (!linestream) {
      std::cerr << "incorrect config file" << std::endl;
      return false;
    }
    if (key == "read-delay") {
      config.read_delay = value;
    } else if (key == "write-delay") {
      config.write_delay = value;
    } else if (key == "rewind-step-delay") {
      config.rewind_step_delay = value;
    } else if (key == "rewind-delay") {
      config.rewind_delay = value;
    } else if (key == "next-delay") {
      config.next_delay = value;
    } else {
      std::cerr << "unknown key " << key << std::endl;
    }
  }
  return true;
}

bool get_uint_param(const std::string& string, size_t& N, const std::string& param_name) {
  try {
    N = std::stoull(string);
  } catch (std::invalid_argument& e) {
    std::cerr << "invalid " << param_name << ". unsigned integer expected: " << e.what() << std::endl;
    return false;
  } catch (std::out_of_range& e) {
    std::cerr << param_name << " is out of range: " << e.what() << std::endl;
    return false;
  }
  return true;
}

std::string get_tmp_path() {
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_int_distribution<size_t> distribution;

  return "./tmp/tmp_" + std::to_string(distribution(gen)) + ".txt";
}

int main(const int argc, char* argv[]) {
  if (argc > 4) {
    std::cerr << "too many arguments:" << std::endl << CALL_FORMAT << std::endl;
    return 1;
  }
  if (argc < 2) {
    std::cerr << "the input and output files expected:" << std::endl << CALL_FORMAT << std::endl;
    return 1;
  }

  std::ifstream fin(argv[0]);
  if (!fin) {
    std::cerr << "error opening the input file" << std::endl;
    return 1;
  }

  std::ofstream fout(argv[1], std::ios_base::out | std::ios_base::trunc);
  if (!fout) {
    std::cerr << "error opening the output file" << std::endl;
    return 1;
  }

  size_t N;
  if (argc > 2) {
    if (!get_uint_param(argv[2], N, "input tape size"))
      return 1;
  } else {
    fin.seekg(0, std::ios_base::end);
    N = fin.tellg() / sizeof(int32_t);
    fin.seekg(0, std::ios_base::beg);
  }

  size_t M = 0;
  if (argc >= 4) {
    if (!get_uint_param(argv[3], N, "memory limit"))
      return 1;
  }

  tape::delay_config delays{};
  if (!parse_delays(delays))
    return 1;

  size_t chunk_size = M / sizeof(int32_t);

  tape::tape tin(std::move(fin), N, delays);
  tape::tape tout(std::move(fout), N, delays);

  try {
    if (N <= chunk_size) {
      sort(tin, tout);
    } else {
      file_guard tmp1_guard(get_tmp_path()),
                 tmp2_guard(get_tmp_path()),
                 tmp3_guard(get_tmp_path());
      std::fstream ftmp1(tmp1_guard.path());
      std::fstream ftmp2(tmp2_guard.path());
      std::fstream ftmp3(tmp3_guard.path());
      if (!ftmp1 || !ftmp2 || !ftmp3) {
        std::cerr << "error opening temporary file";
        return 1;
      }
      tape::tape tmp1(std::move(ftmp1), N, delays);
      tape::tape tmp2(std::move(ftmp2), N, delays);
      tape::tape tmp3(std::move(ftmp3), N, delays);

      sort(tin, tout, tmp1, tmp2, tmp3, chunk_size);
      tout.flush();
    }
  } catch (tape::io_exception& e) {
    std::cerr << "i/o error occurred while working with the tapes: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}