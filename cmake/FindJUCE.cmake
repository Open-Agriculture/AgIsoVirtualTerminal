if(NOT TARGET JUCE::JUCE)
  include(FetchContent)
  FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5)
  FetchContent_MakeAvailable(JUCE)
endif()
