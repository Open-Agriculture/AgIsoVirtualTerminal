set(CAN_STACK_GIT_TAG 03f8846af2bce635920cb0c146ebcc1a47aed29c)
if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG ${CAN_STACK_GIT_TAG})
  FetchContent_MakeAvailable(CAN_Stack)
endif()
