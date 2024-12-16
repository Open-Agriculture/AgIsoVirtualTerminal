cmake_minimum_required(VERSION 3.16)

project(
  "AgISOVirtualTerminal"
  LANGUAGES CXX
  VERSION 1.3.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(JUCE_WEB_BROWSER OFF)
set(BUILD_TESTING OFF)

if(WIN32)
    set(CAN_DRIVER "WindowsPCANBasic")
    list(APPEND CAN_DRIVER "TouCAN")

    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND MINGW))
      list(APPEND CAN_DRIVER "WindowsInnoMakerUSB2CAN")
    else()
      message(STATUS "InnoMaker hardware will not be supported in this build because you are using mingw")
    endif()
    list(APPEND CAN_DRIVER "SYS_TEC")
elseif(APPLE)
    set(CAN_DRIVER "MacCANPCAN")
else()
    set(CAN_DRIVER "SocketCAN")
endif()

find_package(JUCE MODULE)
find_package(CAN_Stack MODULE)
find_package(Threads REQUIRED)

juce_add_gui_app(
  AgISOVirtualTerminal
  ICON_BIG doc/smallLogo.png
  # image file to use as an icon ICON_SMALL ...
  COMPANY_NAME "Open-Agriculture"
  COMPANY_COPYRIGHT "Open-Agriculture"
  COMPANY_WEBSITE "https://github.com/Open-Agriculture"
  PRODUCT_NAME "AgISOVirtualTerminal"
  NEEDS_CURL FALSE
  NEEDS_WEB_BROWSER FALSE
  DESCRIPTION "An ISO11783-6 Server")

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
          "src/ConfigureHardwareWindow.cpp"
          "src/ConfigureHardwareComponent.cpp"
          "src/StringEncodingConversions.cpp"
          "src/InputListComponent.cpp"
          "src/ShortcutsWindow.cpp")


target_include_directories(AgISOVirtualTerminal
                           PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(
  AgISOVirtualTerminal
  PRIVATE juce::juce_gui_extra juce::juce_audio_basics juce::juce_audio_utils isobus::Isobus isobus::HardwareIntegration
          isobus::Utility
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

if(WIN32)
    add_custom_command(
            TARGET AgISOVirtualTerminal POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lib/PCANBasic.dll "$<TARGET_FILE_DIR:AgISOVirtualTerminal>"
            VERBATIM
    )

    add_custom_command(
            TARGET AgISOVirtualTerminal POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lib/Usbcan64.dll "$<TARGET_FILE_DIR:AgISOVirtualTerminal>"
            VERBATIM
    )

    add_custom_command(
            TARGET AgISOVirtualTerminal POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lib/canal.dll "$<TARGET_FILE_DIR:AgISOVirtualTerminal>"
            VERBATIM
    )

    add_custom_command(
            TARGET AgISOVirtualTerminal POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lib/InnoMakerUsb2CanLib.dll "$<TARGET_FILE_DIR:AgISOVirtualTerminal>"
            VERBATIM
    )
endif()

if(APPLE)
install(
  TARGETS AgISOVirtualTerminal
  RUNTIME
  DESTINATION bin
  BUNDLE DESTINATION .
  COMPONENT applications 
  )
else()
install(
  TARGETS AgISOVirtualTerminal
  RUNTIME
  DESTINATION bin
  COMPONENT applications 
  )
endif()

if(MSVC)
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/lib/Usbcan64.dll DESTINATION bin COMPONENT applications)
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/lib/PCANBasic.dll DESTINATION bin COMPONENT applications)
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/lib/canal.dll DESTINATION bin COMPONENT applications)
    install(FILES ${CMAKE_CURRENT_LIST_DIR}/lib/InnoMakerUsb2CanLib.dll DESTINATION bin COMPONENT applications)
endif()

set (CPACK_PACKAGE_NAME "AgISOVirtualTerminal")
set (CPACK_PACKAGE_VENDOR "Open-Agriculture")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "An ISO11783-6 Server")
set (CPACK_PACKAGE_VERSION "1.3.0")
set (CPACK_PACKAGE_VERSION_MAJOR "1")
set (CPACK_PACKAGE_VERSION_MINOR "3")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_PACKAGE_INSTALL_DIRECTORY "AgISOVirtualTerminal")
set (CPACK_COMPONENTS_ALL applications)
set (CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "AgISOVirtualTerminal")
set (CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "A free and open source ISO11783-6 Server")
set (CPACK_PACKAGE_CONTACT "delgrossoengineering@protonmail.com")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
set (CPACK_PACKAGE_EXECUTABLES "AgISOVirtualTerminal" "AgISOVirtualTerminal")
set(CPACK_ALL_INSTALL_TYPES Full)
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)
set(CPACK_NSIS_INSTALLED_ICON_NAME ${CMAKE_CURRENT_LIST_DIR}/doc/smallLogo.ico)
set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
set(CPACK_NSIS_MUI_ICON ${CPACK_NSIS_INSTALLED_ICON_NAME})
set(CPACK_NSIS_MUI_UNICON ${CPACK_NSIS_INSTALLED_ICON_NAME})
set(CPACK_NSIS_DISPLAY_NAME "AgISOVirtualTerminal")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT applications)
include (InstallRequiredSystemLibraries)

if(APPLE)
    set(CPACK_GENERATOR DragNDrop ${CPACK_GENERATOR})
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
    set(CPACK_SYSTEM_NAME "OSX")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
    set(CPACK_PACKAGE_ICON ${CPACK_NSIS_INSTALLED_ICON_NAME})
endif()

# Must be after the last CPACK macros
include(CPack)
