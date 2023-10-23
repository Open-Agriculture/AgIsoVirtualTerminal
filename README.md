# AgIsoVirtualTerminal

This project is a free, but experimental ISO11783-6 (ISOBUS) virtual terminal server GUI meant for agricultural and forestry equipment meant to serve as the reference/example implementation of the AgIsoStack++ VT server interface.

The project is written in C++ and is based on AgIsoStack++ and the JUCE GUI framework.

This project is somewhat functional for a single VT client, but is still in the early stages of development, and as such is not fit for general consumption.

## Project Status

This section will be updated as progress is made on the project.

Supported features:

- All basic ISO11783 stuff, such as address claiming, TP, ETP, diagnostic protocol, etc.
- Exactly 1 VT client
- Object pool deserializer (most objects up to version 6 with some exceptions)
- Data masks
- Soft key masks
- Key objects
- Containers
- Object Pointers
- Input numbers
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
- Some CAN messages (Object pool upload state machine and handshake messages, maintenance messages, change active mask response, change child location response, change numeric value and response, key/button activation and release messages, change numeric value, select input object)

Unimplemented features (for now)

- Alarm masks
- All audio functionality
- Selecting different working sets
- Window masks
- Aux N/O
- Most macro functionality
- Animations
- Output Lists
- Output arched bar graph
- Logging
- Input Boolean
- Graphics contexts
- Pointing events
- TAN
- Several messages, such as ESC
- Probably more things to be honest

## Compilation

This project is compiled with CMake and your favorite C++17 compiler.

Make sure you select your desired CAN driver as supported by AgIso Stack by defining the `CAN_DRIVER` variable.

Example:

```
cmake -S. -B build -DCAN_DRIVER=TouCAN
cmake --build build
```

### Disclaimers

This project is not associated with the Agricultural Industry Electronics Foundation (AEF) in any way. By acquiring or using this project you agree to the [JUCE License](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md) as well as any applicable licenses provided by dependencies such as AgIsoStack.
