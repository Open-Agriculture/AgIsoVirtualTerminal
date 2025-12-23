if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG d5d8ff1d8769d714424b6dee2bd59e10f61c3845)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
