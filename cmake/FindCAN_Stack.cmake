if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/gunicsba/AgIsoStack-plus-plus.git
    GIT_TAG 9f0acdf9e7606580a8df9cc1df7b87761ff6fc17)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
