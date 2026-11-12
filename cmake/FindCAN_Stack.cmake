set(CAN_STACK_GIT_TAG a8f832ed11f5edb12324ff9a45f503126acf37fb)
if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG ${CAN_STACK_GIT_TAG})
  FetchContent_MakeAvailable(CAN_Stack)
endif()
