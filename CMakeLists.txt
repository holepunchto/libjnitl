cmake_minimum_required(VERSION 4.0)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(jnitl C CXX)

find_package(JNI)

add_library(jnitl INTERFACE)

set_target_properties(
  jnitl
  PROPERTIES
  C_STANDARD 11
  CXX_STANDARD 20
  CXX_SCAN_FOR_MODULES OFF
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  jnitl
  INTERFACE
    include/jnitl.h
)

target_include_directories(
  jnitl
  INTERFACE
    include
)

target_link_libraries(
  jnitl
  INTERFACE
    JNI::JNI
)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
