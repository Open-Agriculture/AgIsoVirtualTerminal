if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG ab38a6dcfb4a073d0f0d1817ca37b48d19d7e1be)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
