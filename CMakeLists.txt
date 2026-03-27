cmake_minimum_required(VERSION 4.0)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-java REQUIRED PATHS node_modules/cmake-java)

project(jnitl C CXX)

use_java()

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

if(ANDROID)
  target_link_libraries(
    jnitl
    INTERFACE
      JNI::NativeHelper
  )
endif()

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
