if(NOT DEFINED CASE_SOURCE)
  message(FATAL_ERROR "CASE_SOURCE is required")
endif()
if(NOT DEFINED CXX)
  message(FATAL_ERROR "CXX is required")
endif()
if(NOT DEFINED CXX_STANDARD)
  message(FATAL_ERROR "CXX_STANDARD is required")
endif()
if(NOT DEFINED PROJECT_INCLUDE_DIR)
  message(FATAL_ERROR "PROJECT_INCLUDE_DIR is required")
endif()

set(flag_list "")
if(DEFINED CXX_FLAGS AND NOT CXX_FLAGS STREQUAL "")
  separate_arguments(flag_list NATIVE_COMMAND "${CXX_FLAGS}")
endif()

execute_process(
  COMMAND "${CXX}" "-std=c++${CXX_STANDARD}" -fsyntax-only ${flag_list} "-I${PROJECT_INCLUDE_DIR}" "${CASE_SOURCE}"
  RESULT_VARIABLE compile_rv
  OUTPUT_VARIABLE compile_out
  ERROR_VARIABLE compile_err
)

if(compile_rv EQUAL 0)
  message(FATAL_ERROR "Expected compile failure but succeeded for ${CASE_SOURCE}")
endif()
