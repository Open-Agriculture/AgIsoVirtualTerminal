if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/Open-Agriculture/AgIsoStack-plus-plus.git
    GIT_TAG a192d1a5bdb7ac834dbe6a45af19d15fd735572e)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
