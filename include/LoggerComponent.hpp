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

#include <deque>
#include <mutex>
#include <string>
#include <vector>

/// @brief Defines a GUI component that will draw log info sunk from the stack
///
/// @details The sink runs on the logging thread, mostly the CAN thread, which must answer
/// transport protocol handshakes in time, so it only queues. The file is written on this class's own thread.
class LoggerComponent : public Component
  , public isobus::CANStackLogger
  , private AsyncUpdater
  , private Thread
{
public:
	LoggerComponent();
	~LoggerComponent() override;

	void paint(Graphics &g) override;

	void sink_CAN_stack_log(LoggingLevel level, const std::string &logText) override;
	static constexpr int HEIGHT = 200;

	const File &getLogFile() const;

private:
	struct LogData
	{
		String logText;
		isobus::CANStackLogger::LoggingLevel logLevel;
	};
	static constexpr std::size_t MAX_NUMBER_MESSAGES = 3000;
	static constexpr std::size_t MAX_QUEUED_FILE_LINES = 20000; ///< Cap so a stuck disk cannot eat all memory
	static constexpr std::int64_t MAX_FILE_SIZE_BYTES = 1024000;
	static constexpr int WRITE_INTERVAL_MS = 250;
	static constexpr int LOG_RETENTION_DAYS = 3;

	void handleAsyncUpdate() override;

	void run() override;
	void write_to_file(const std::vector<std::string> &lines, std::size_t droppedLines);
	void trim_file();

	std::deque<LogData> loggedMessages;
	std::mutex pendingMessagesMutex;
	std::deque<LogData> pendingMessages;
	std::vector<std::string> pendingFileLines;
	std::size_t droppedFileLines = 0;

	File logFile;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoggerComponent)
};

#endif // LOGGER_COMPONENT_HPP
