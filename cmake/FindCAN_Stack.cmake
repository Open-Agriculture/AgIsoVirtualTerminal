if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 92c9356467f87fabc4683308774ca595bf72b59e)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
