if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG e3a16f2f26698736b58ae05606fc979289ae5354)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
