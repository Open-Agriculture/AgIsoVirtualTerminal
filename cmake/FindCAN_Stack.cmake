if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 7b771274422ea0f62d4eac042fa1684ee373d463)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
