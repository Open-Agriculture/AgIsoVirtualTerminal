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
	static constexpr int HEIGHT = 200;

	std::uint64_t initialPos() const;

	/// @brief Drains any log messages queued up from background threads and applies them to
	/// the visible log, resizing/repainting at most once regardless of how many arrived. Must
	/// be called from the UI thread - intended to be driven by ServerMainComponent's own
	/// periodic timer, so a burst of stack log activity (e.g. a lot of VT traffic) can never
	/// flood the JUCE message queue with one post per log line - see sink_CAN_stack_log().
	void pump_pending_messages();

private:
	struct LogData
	{
		String logText;
		isobus::CANStackLogger::LoggingLevel logLevel;
	};
	static constexpr std::size_t MAX_NUMBER_MESSAGES = 3000;
	std::deque<LogData> loggedMessages;

	std::mutex pendingMessagesMutex; ///< Guards pendingMessages only, cheap and dedicated - never held across any isobus stack call
	std::deque<LogData> pendingMessages; ///< Messages sunk from background threads, not yet applied to loggedMessages

	std::uint64_t startPos = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoggerComponent)
};

#endif // LOGGER_COMPONENT_HPP
