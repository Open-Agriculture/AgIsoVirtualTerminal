if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 2dd415cd54a148115ebdd2128bd6366d097d28a4)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
