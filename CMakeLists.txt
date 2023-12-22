cmake_minimum_required(VERSION 3.16)

project(
  "AgISOVirtualTerminal"
  LANGUAGES CXX
  VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(JUCE_WEB_BROWSER OFF)
set(BUILD_TESTING OFF)

find_package(JUCE MODULE)
find_package(CAN_Stack MODULE)
find_package(Threads REQUIRED)

# set(CAN_DRIVER "TouCAN") Set this to be whatever your CAN driver is, or pass it in with -DCAN_DRIVER=...

juce_add_gui_app(
  AgISOVirtualTerminal
  ICON_BIG smallLogo.png
  # image file to use as an icon ICON_SMALL ...
  COMPANY_NAME
  "Open-Agriculture"
  PRODUCT_NAME
  NEEDS_CURL FALSE
  NEEDS_WEB_BROWSER FALSE
  DESCRIPTION "An ISO11783-6 Server"
  "AgISOVirtualTerminal")

set_target_properties(AgISOVirtualTerminal PROPERTIES CXX_STANDARD 17)

target_compile_definitions(AgISOVirtualTerminal PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)

juce_generate_juce_header(AgISOVirtualTerminal)

target_sources(
  AgISOVirtualTerminal
  PRIVATE src/Main.cpp
          "src/ServerMainComponent.cpp"
          "src/DataMaskComponent.cpp"
          "src/WorkingSetSelectorComponent.cpp"
          "src/PictureGraphicComponent.cpp"
          "src/WorkingSetComponent.cpp"
          "src/AlarmMaskComponent.cpp"
          "src/DataMaskRenderAreaComponent.cpp"
          "src/JuceManagedWorkingSetCache.cpp"
          "src/OutputRectangleComponent.cpp"
          "src/OutputStringComponent.cpp"
          "src/ButtonComponent.cpp"
          "src/ContainerComponent.cpp"
          "src/ObjectPointerComponent.cpp"
          "src/OutputNumberComponent.cpp"
          "src/SoftkeyMaskRenderArea.cpp"
          "src/KeyComponent.cpp"
          "src/SoftKeyMaskComponent.cpp"
          "src/OutputMeterComponent.cpp"
          "src/InputBooleanComponent.cpp"
          "src/LoggerComponent.cpp"
          "src/InputNumberComponent.cpp"
          "src/OutputEllipseComponent.cpp"
          "src/OutputLineComponent.cpp"
          "src/OutputLinearBarGraphComponent.cpp"
          "src/OutputPolygonComponent.cpp"
          "src/InputStringComponent.cpp"
          "src/AlarmMaskAudio.cpp"
          "src/AppImages.cpp"
          "src/ASCIILogFile.cpp"
          "src/InputListComponent.cpp")

target_include_directories(AgISOVirtualTerminal
                           PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(
  AgISOVirtualTerminal
  PRIVATE juce::juce_gui_extra juce::juce_audio_basics juce::juce_audio_utils isobus::Isobus isobus::HardwareIntegration
          isobus::Utility
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
