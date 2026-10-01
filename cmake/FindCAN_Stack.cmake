if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 40efe24161e4d3f62b7ba1309b56475c280bfa24)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
