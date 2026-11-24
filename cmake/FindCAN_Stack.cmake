set(CAN_STACK_GIT_TAG 5f421d7a107f367854f5d33425037c0cec9b5511)
if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG ${CAN_STACK_GIT_TAG})
  FetchContent_MakeAvailable(CAN_Stack)
endif()
