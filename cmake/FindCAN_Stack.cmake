if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG vt-server-managed-working-set
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
