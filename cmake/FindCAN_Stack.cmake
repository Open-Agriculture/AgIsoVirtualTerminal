if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 8fd1ea1962574724e0b287a8ac921c86420eae75)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
