if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG a0650c2a2cbc77b79ddf6389279694d2e03ecf03)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
