if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG 67e231298f2ca8367d437d903e396aa3bcefe080
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
