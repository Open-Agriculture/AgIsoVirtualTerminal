if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 0c5e3d2f264270f0a750cc3fdedaaa5861d71a59)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
