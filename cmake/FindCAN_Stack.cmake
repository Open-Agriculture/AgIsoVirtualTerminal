if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 5652baff6123ccf41651347f6f61d03545c275c6)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
