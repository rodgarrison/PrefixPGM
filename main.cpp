#include <string>
#include <iostream>

#include <pgm_index_prefix.hpp>

#include <benchmark_loadfile.h>
#include <benchmark_slice.h>
#include <benchmark_textscan.h>
#include <intel_pmu_stats.h>
#include <intel_skylake_pmu.h>

#include <unistd.h>

// PGM Epsilon tolerated 
int32_t epsilon = 8;

// How much (at most) of each key is considerd to produce integer
int32_t prefixSize = 16;

// Binary file containing sorted keys
std::string filename;

void usageAndExit() {
  printf("benchmark.tsk -f <file> [-e <epsilon> -p <size>]\n");
  printf("\n");
  printf("        -f <file>      mandatory: text file containing one key per UNIX delimited line\n");
  printf("        -e <epsilon>   optional : PGM index epsilon default %d. Must be a power of 2 and >=2\n", epsilon);
  printf("        -p <size>      optional : Maximum number of characters >=8 (default %d) to consider per key for integer mapping\n", prefix_size);
  printf("\n");
  exit(2);
}

void parseCommandLine(int argc, char **argv) {
  int opt;

  const char *switches = "f:e:p:";

  while ((opt = getopt(argc, argv, switches)) != -1) {
    switch (opt) {
      case 'f':
        {
          if (strlen(optarg)>0) {
            filename = optarg;
          } else {
            usageAndExit();
          }
        }
        break;

      case 'e':                                                                                                         
        {                                                                                                               
          epsilon = atoi(optarg);
          int ok = (epsilon & (epsilon - 1)) == 0;
          if (epsilon<2 || !ok) {
            usageAndExit();
          }
        }                                                                                                               
        break;

      case 'p':
        {
          prefixSize = atoi(optarg);
          if (prefixSize<=8) {
            usageAndExit();
          }
        }
        break;
 
      default:
        usageAndExit();
    }
  }

  if (filename.empty()) {
    usageAndExit();
  }
}

std::vector<std::string> read_string_prefixes(const std::string &path, size_t prefix_length, size_t limit = -1) {
    auto previous_value = std::ios::sync_with_stdio(false);
    std::vector<std::string> result;
    std::ifstream in(path.c_str());
    std::string str;
    while (std::getline(in, str) && limit-- > 0) {
        if (str.size() > prefix_length)
            str.resize(prefix_length);
        result.push_back(str);
    }
    std::ios::sync_with_stdio(previous_value);
    return result;
}

template<int First, int Last, typename Lambda>
inline void static_for_pow_two(Lambda const &f) {
    if constexpr (First <= Last) {
        { f(std::integral_constant<int, First>{}); }
        static_for_pow_two<First << 1, Last>(f);
    }
}

int main() {
  parseCommandLine();

  std::vector<std::string> data = read_string_prefixes(/share/dict/words", prefix_size);
    std::cout << "Read " << data.size() << " lines" << std::endl;
    std::sort(data.begin(), data.end());
    auto new_end = std::unique(data.begin(), data.end());
    if (new_end != data.end()) {
        data.erase(new_end, data.end());
        std::cout << "After removing duplicates " << data.size() << std::endl;
    }

    static_for_pow_two<8, 64>([&](auto epsilon) {
        std::cout << std::string(79, '-') << std::endl;

        // -------------- PGM CONSTRUCTION --------------
        std::cout << "Prefix size " << prefix_size << ", epsilon " << epsilon << std::endl;

        pgm::PrefixPGMIndex<prefix_size, epsilon, 0, true> pgm(data.begin(), data.end());
        std::cout << "PGM #segm: " << pgm.segments_count() << std::endl;
        std::cout << "PGM bytes: " << pgm.size_in_bytes() << std::endl;

        // -------------- TEST --------------
        for (size_t i = 0; i < data.size(); ++i) {
            auto range = pgm.approximate_position(data[i]);
            if (i > range.hi || i < range.lo) {
                std::cout << data[i] << " " << i << " vs returned " << range.lo << " " << range.hi << std::endl;
                exit(1);
            }
        }
        std::cout << "Tests done" << std::endl;
    });
    return 0;
}
