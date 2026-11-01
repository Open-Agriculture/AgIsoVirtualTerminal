if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 46f3c2267a06bc0bd6ae90e30ca08605d2f10096)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
