cmake_minimum_required(VERSION 4.0)

set(TOML26_TEST_SCRIPT_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(TOML26_FAIL_SCRIPT "${TOML26_TEST_SCRIPT_DIR}/run_fail_case.cmake")

if(CMAKE_BUILD_TYPE)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" TOML26_BUILD_TYPE_UPPER)
  set(TOML26_TEST_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${TOML26_BUILD_TYPE_UPPER}}")
else()
  set(TOML26_TEST_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

function(toml26_require_case_files case_dir case_name)
  set(case_main "${case_dir}/main.cpp")
  set(case_toml "${case_dir}/case.toml")
  if(NOT EXISTS "${case_main}")
    message(FATAL_ERROR "Missing main.cpp for case ${case_name}: ${case_main}")
  endif()
  if(NOT EXISTS "${case_toml}")
    message(FATAL_ERROR "Missing case.toml for case ${case_name}: ${case_toml}")
  endif()
endfunction()

function(toml26_add_pass_case case_dir)
  get_filename_component(case_name "${case_dir}" NAME)
  string(REGEX REPLACE "^pass_" "" case_label "${case_name}")
  toml26_require_case_files("${case_dir}" "${case_name}")
  set(case_main "${case_dir}/main.cpp")

  set(target_name "toml26_test_${case_name}")
  add_executable("${target_name}" "${case_main}")
  target_include_directories("${target_name}" PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_test(NAME "toml26.pass.${case_label}" COMMAND "$<TARGET_FILE:${target_name}>")
endfunction()

function(toml26_add_fail_case case_dir)
  get_filename_component(case_name "${case_dir}" NAME)
  string(REGEX REPLACE "^fail_" "" case_label "${case_name}")
  toml26_require_case_files("${case_dir}" "${case_name}")
  set(case_main "${case_dir}/main.cpp")

  add_test(
    NAME "toml26.fail.${case_label}"
    COMMAND "${CMAKE_COMMAND}"
      "-DCASE_SOURCE=${case_main}"
      "-DCXX=${CMAKE_CXX_COMPILER}"
      "-DCXX_STANDARD=${CMAKE_CXX_STANDARD}"
      "-DCXX_FLAGS=${TOML26_TEST_CXX_FLAGS}"
      "-DPROJECT_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/include"
      -P "${TOML26_FAIL_SCRIPT}"
  )
endfunction()

file(GLOB TOML26_PASS_CASE_MAINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/pass_*/main.cpp")
file(GLOB TOML26_FAIL_CASE_MAINS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/fail_*/main.cpp")

foreach(case_main IN LISTS TOML26_PASS_CASE_MAINS)
  get_filename_component(case_dir "${case_main}" DIRECTORY)
  toml26_add_pass_case("${case_dir}")
endforeach()

foreach(case_main IN LISTS TOML26_FAIL_CASE_MAINS)
  get_filename_component(case_dir "${case_main}" DIRECTORY)
  toml26_add_fail_case("${case_dir}")
endforeach()
