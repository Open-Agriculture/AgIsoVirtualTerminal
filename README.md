# AgIsoVirtualTerminal

This project is a free, but experimental ISO11783-6 (ISOBUS) virtual terminal server GUI meant for agricultural and forestry equipment meant to serve as the reference/example implementation of the AgIsoStack++ VT server interface.

The project is written in C++ and is based on AgIsoStack++ and the JUCE GUI framework.

This project is somewhat functional for a single VT client, but is still in the early stages of development, and as such is not fit for general consumption.

## Project Status

This section will be updated as progress is made on the project.

Supported features:

- All basic ISO11783 stuff, such as address claiming, TP, ETP, diagnostic protocol, etc.
- Object pool deserializer (most objects up to version 6 with some exceptions)
- Data masks
- Alarm masks
- Soft key masks
- Key objects
- Containers
- Object Pointers
- Input numbers
- Input Boolean
- Output numbers
- Output rectangles
- Output ellipse
- Output polygon
- Output line
- Buttons
- Output linear bar graph (except target line)
- Output meter (except tick marks and target line)
- Picture graphics (with and without run-length encoding)
- Output Strings (partial - font clipping is not compliant)
- Input lists (partial - drawing the selector needs work)
- Most relevant VT server CAN messages
- Logging
- Multiple simultaneous VT clients
- Selecting different working sets

Unimplemented features (for now)

- Arbitrary audio control functionality
- Window masks (tolerated in the object pool though)
- Aux N/O
- Most macro functionality
- Animations
- Output Lists
- Output arched bar graph
- Graphics contexts
- Pointing events
- TAN
- Several messages, such as ESC
- Probably more things to be honest

## Compilation

This project is compiled with CMake and your favorite C++17 compiler.

CMake 3.22 or higher is required.

### Dependencies

Linux:
```
sudo apt install git cmake pkg-config libfreetype-dev libfreetype6 libfreetype6-dev libasound2-dev libcurl4-openssl-dev libgtk-3-dev libwebkit2gtk-4.0-dev webkit2gtk-4.0
```

Windows:

On Windows, you will need to install Visual Studio to get the MSVC compiler and CMake. The easiest way to do that would be to install [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) and select the "Desktop development with C++" workload.

### Building

Make sure you select your desired CAN driver as supported by AgIso Stack by defining the `CAN_DRIVER` variable.

Example:

```
cmake -S. -B build -DCAN_DRIVER=TouCAN
cmake --build build
```

### Disclaimers

This project is not associated with the Agricultural Industry Electronics Foundation (AEF) in any way. By acquiring or using this project you agree to the [JUCE License](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md) as well as any applicable licenses provided by dependencies such as AgIsoStack.
