if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 8ed87459e92448b8256a213f71379b266bf2af88)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
