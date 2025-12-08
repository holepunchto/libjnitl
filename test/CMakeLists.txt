list(APPEND tests
  basic
)

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
endforeach()

