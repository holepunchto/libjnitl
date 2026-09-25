if(IOS OR ANDROID)
  return()
endif()

list(APPEND tests
  basic
  class-loader
  native-method
)

# Windows links against jvm.dll, which sits in a JDK subdirectory that is not
# searched by default.
if(WIN32)
  cmake_path(GET JAVA_JVM_LIBRARY PARENT_PATH java_library_directory)
  cmake_path(GET java_library_directory PARENT_PATH java_home)

  find_file(
    java_runtime jvm.dll
    PATHS "${java_home}/bin/server" "${java_home}/jre/bin/server"
    NO_DEFAULT_PATH
  )

  if(NOT java_runtime)
    message(FATAL_ERROR "Could not find 'jvm.dll' in '${java_home}'")
  endif()

  cmake_path(GET java_runtime PARENT_PATH java_runtime_directory)
  cmake_path(NATIVE_PATH java_runtime_directory java_runtime_directory)
endif()

foreach(test IN LISTS tests)
  add_executable(${test} ${test}.cc)

  set_target_properties(
    ${test}
    PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 20
    CXX_SCAN_FOR_MODULES OFF
  )

  target_link_libraries(
    ${test}
    PRIVATE
      jnitl
    PRIVATE
      JNI::JVM
  )

  add_test(
    NAME ${test}
    COMMAND ${test}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  set_tests_properties(
    ${test}
    PROPERTIES
    TIMEOUT 30
  )

  if(WIN32)
    set_tests_properties(
      ${test}
      PROPERTIES
      ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${java_runtime_directory}"
    )
  endif()
endforeach()

