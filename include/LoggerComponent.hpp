//================================================================================================
/// @file LoggerComponent.hpp
///
/// @brief Defines a GUI component to draw log output.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef LOGGER_COMPONENT_HPP
#define LOGGER_COMPONENT_HPP

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

/// @brief Defines a GUI component that will draw log info sunk from the stack
class LoggerComponent : public Component
  , public FileLogger
  , public isobus::CANStackLogger
{
public:
	LoggerComponent();

	void paint(Graphics &g) override;

	void sink_CAN_stack_log(LoggingLevel level, const std::string &logText) override;
	static const int HEIGHT = 200;

private:
	struct LogData
	{
		String logText;
		isobus::CANStackLogger::LoggingLevel logLevel;
	};
	static constexpr std::size_t MAX_NUMBER_MESSAGES = 3000;
	std::deque<LogData> loggedMessages;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoggerComponent)
};

#endif // LOGGER_COMPONENT_HPP
