set(CAN_STACK_GIT_TAG c683deaacc09d5fd251587efe5ae2b4cec2acef3)
if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG ${CAN_STACK_GIT_TAG})
  FetchContent_MakeAvailable(CAN_Stack)
endif()
