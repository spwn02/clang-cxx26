cmake_minimum_required(VERSION 4.4 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD ON)

if(CMAKE_VERSION VERSION_LESS 4.5
   AND NOT DEFINED CMAKE_EXPERIMENTAL_CXX_IMPORT_STD)
  set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
      "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
endif()

project(P2996ReferenceToolchainSmoke LANGUAGES CXX)

if(NOT 26 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
  message(FATAL_ERROR
          "The packaged reference toolchain does not expose C++26 import std")
endif()

add_executable(P2996ReferenceToolchainSmoke main.cxx)

target_compile_features(P2996ReferenceToolchainSmoke PRIVATE cxx_std_26)
target_compile_options(P2996ReferenceToolchainSmoke PRIVATE -Wall -Wextra -Wpedantic -Werror)

set_target_properties(
  P2996ReferenceToolchainSmoke
  PROPERTIES CXX_STANDARD 26
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             CXX_SCAN_FOR_MODULES ON
             CXX_MODULE_STD ON)
