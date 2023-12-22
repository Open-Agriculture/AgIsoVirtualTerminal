//================================================================================================
/// @file ASCIILogFile.hpp
///
/// @brief Defines a CAN logger that saves messages in a Vector .asc file.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ASCII_LOG_FILE_HPP
#define ASCII_LOG_FILE_HPP

#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include "JuceHeader.h"

/// @brief Logs to Vector .asc file
class ASCIILogFile
{
public:
	ASCIILogFile();

	~ASCIILogFile() = default;

private:
	File logFile;
	std::shared_ptr<void> canFrameReceivedListener;
	std::shared_ptr<void> canFrameSentListener;
	Time initialTimestamp;
};

#endif // ASCII_LOG_FILE_HPP
