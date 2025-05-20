if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG e1f0eef1aeb848412daf80a04c978bcad650ec3a)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
