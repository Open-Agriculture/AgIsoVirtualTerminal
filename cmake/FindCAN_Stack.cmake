if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 1a037eea1a31084055ab8125c7d0ca236377792a)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
