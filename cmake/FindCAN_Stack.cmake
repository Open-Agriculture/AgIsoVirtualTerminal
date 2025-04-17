if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 1b842ad3afd67a882c33885d50d86a8ac7608aae)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
