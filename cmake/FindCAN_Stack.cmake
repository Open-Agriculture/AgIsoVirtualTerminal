if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 4ad80207b5e44d041d8e0fba8ef044367dfbfe86)
  FetchContent_MakeAvailable(CAN_Stack)
