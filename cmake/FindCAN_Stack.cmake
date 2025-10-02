if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 92ba5609d641795525720bef9a3f10deb19b7308)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
