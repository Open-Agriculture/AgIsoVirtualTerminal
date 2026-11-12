if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG a8f832ed11f5edb12324ff9a45f503126acf37fb)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
