if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 68bc01550156a0d9ce7f191d2be8145cdc084771)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
