if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG c9ffdc2d92ede8634c2d7ed128c59a08f2847245
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
