if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG d6dfd16f35f8eb401af5bab65c2ca6284a6761d3
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
