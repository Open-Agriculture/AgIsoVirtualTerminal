if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG 80a1823e62ab65eb660cf1c1f9231d5fd9c7fc7f
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
