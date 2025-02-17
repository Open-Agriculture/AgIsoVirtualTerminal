if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG 784ddfac010f6d61db90c48fa6997d3ffe12128c)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
