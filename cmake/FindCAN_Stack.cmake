if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 2899c1576ee541b06de8787e70cbdb3f2df3afa1)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
