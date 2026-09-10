execute_process(
  COMMAND "${CXX}" -std=c++20 -I"${GENERATED_INCLUDE_DIR}" -I"${INCLUDE_DIR}"
    -c "${SOURCE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)
if(result EQUAL 0)
  message(FATAL_ERROR "Expected compilation failure, but probe compiled: ${SOURCE}")
endif()
