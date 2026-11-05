if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 16f2b90397146781c5a685e84e5560ae598cc91c)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
