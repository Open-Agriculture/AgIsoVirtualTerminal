if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG a71c186efd189b45759364ddf790b5634b8ba878)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
