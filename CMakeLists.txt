cmake_minimum_required(VERSION 3.12)
project(PrefixPGMIndex)

execute_process(COMMAND git submodule update --init --recursive)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -g")

include_directories(.)
include_directories(wide-integer/math/wide_integer)

add_executable(benchmark.tsk
  main.cpp
  intel_pmu_stats.cpp
  intel_skylake_pmu.cpp
  benchmark_loadfile.cpp
  benchmark_slice.cpp
  benchmark_textscan.cpp)
