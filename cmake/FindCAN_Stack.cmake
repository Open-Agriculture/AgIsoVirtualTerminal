if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG d22bea645bd04c7c65b454326b3d0e15d7f76d8b)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
