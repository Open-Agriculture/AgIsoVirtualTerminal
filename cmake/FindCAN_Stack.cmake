if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 5e1f9f600cc626463d92a3278707b60047b4a2cd)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
