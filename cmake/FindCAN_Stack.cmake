if (NOT TARGET isobus::isobus)
include(FetchContent)
FetchContent_Declare(
  CAN_Stack
  GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
  GIT_TAG 29dab887a48bb204aae983b06052d52b0f2314d5
)
FetchContent_MakeAvailable(CAN_Stack)
endif()
