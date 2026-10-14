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

#include <atomic>
#include <vector>

/// @brief Logs to Vector .asc file
///
/// @details The frame callbacks run on the CAN stack's update thread, which is the same thread
/// that has to answer transport protocol handshakes within a couple of hundred milliseconds.
/// Writing to the file from there is therefore not an option: formatting is cheap and happens
/// inline, but the file access is handed to this class's own thread.
class ASCIILogFile : private Thread
{
public:
	ASCIILogFile();

	~ASCIILogFile() override;

	std::string currentLogFile() const;

	/// @brief Turns CAN traffic logging on or off. It is off unless the settings say otherwise,
	/// because logging every frame costs performance which the transport protocol cannot spare.
	/// @param[in] shouldLog True to log CAN traffic to the .asc file
	static void set_logging_enabled(bool shouldLog);

	/// @brief Returns whether CAN traffic is being logged
	/// @returns True if CAN traffic logging is on
	static bool is_logging_enabled();

private:
	/// @brief Writes whatever has been queued to the file. Runs on this class's own thread.
	void run() override;

	/// @brief Formats one frame into a line of the .asc file and queues it for writing
	/// @param[in] canFrame The frame to log
	/// @param[in] wasTransmitted True for a frame this program sent, false for a received one
	void queue_frame(const isobus::CANMessageFrame &canFrame, bool wasTransmitted);

	/// @brief Opens the file and writes the Vector header. Deferred until something is actually
	/// logged, so that an empty file is not left behind when logging is off.
	/// @returns True if the file is open and ready to be written to
	bool open_file_if_needed();

	static constexpr int WRITE_INTERVAL_MS = 250; ///< How often queued lines are flushed to disk
	static constexpr std::size_t MAXIMUM_QUEUED_LINES = 20000; ///< Cap so a stuck disk cannot eat all memory

	static std::atomic<bool> loggingEnabled;

	File logFile;
	std::unique_ptr<FileOutputStream> logStream;
	CriticalSection queueLock;
	std::vector<String> pendingLines;
	std::atomic<bool> hasReportedOverflow{ false };
	isobus::EventCallbackHandle canFrameReceivedListener;
	isobus::EventCallbackHandle canFrameSentListener;
	Time initialTimestamp;
};

#endif // ASCII_LOG_FILE_HPP
