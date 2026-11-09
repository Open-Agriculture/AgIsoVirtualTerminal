if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 02f3fe910de5fdc73e2014394c2e80900490b9cc)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
