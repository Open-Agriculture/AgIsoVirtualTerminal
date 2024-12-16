if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG db00fa406a6d83bf8cb75cc32650eb29e0f396e9)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
