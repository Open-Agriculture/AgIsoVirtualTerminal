if(NOT TARGET JUCE::JUCE)
  include(FetchContent)
  FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8)
  FetchContent_MakeAvailable(JUCE)
endif()
