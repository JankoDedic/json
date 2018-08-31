cmake_minimum_required(VERSION 3.11)
project(json)
add_executable(json json.cpp)
target_compile_features(json PRIVATE cxx_std_17)
