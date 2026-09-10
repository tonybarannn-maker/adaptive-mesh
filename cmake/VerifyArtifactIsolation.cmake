if(NOT DEFINED ARTIFACT)
  message(FATAL_ERROR "ARTIFACT is required")
endif()
if(NOT EXISTS "${ARTIFACT}")
  message(FATAL_ERROR "Artifact does not exist: ${ARTIFACT}")
endif()
if(NOT DEFINED NM)
  message(FATAL_ERROR "NM is required")
endif()
execute_process(
  COMMAND "${NM}" -C "${ARTIFACT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE symbols
  ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Unable to inspect ${ARTIFACT}: ${error}")
endif()
if(symbols MATCHES "runLiveScenario|ProductionTransitionEvaluatorScenarioAccess")
  message(FATAL_ERROR "Scenario-only symbol escaped into ${ARTIFACT}")
endif()
