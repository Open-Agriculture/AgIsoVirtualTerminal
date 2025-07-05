if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 30e51211a760c2df064dc2934a4f180c4cad73cc)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
