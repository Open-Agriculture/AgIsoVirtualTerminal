if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 908057f762de461d6d64190a0930d87e96251077)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
