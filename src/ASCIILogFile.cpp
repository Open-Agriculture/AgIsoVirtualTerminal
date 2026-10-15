/*******************************************************************************
** @file       ASCIILogFile.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ASCIILogFile.hpp"

#include "ServerMainComponent.hpp"

#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

std::atomic<bool> ASCIILogFile::loggingEnabled{ false };

ASCIILogFile::ASCIILogFile() :
  Thread("CAN ASCII log writer")
{
	auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
	initialTimestamp = Time::getCurrentTime();
	auto fileNameTime = currentTime;
	fileNameTime = currentTime.replaceCharacter(' ', '_');
	fileNameTime = currentTime.replaceCharacter(':', '_');

	logFile = File(ServerMainComponent::getAppDataDir() +
	               File::getSeparatorString() +
	               "CANLog_" +
	               fileNameTime +
	               ".asc");

	// Prune old log files
	auto logDirectory = logFile.getParentDirectory();
	auto childFiles = logDirectory.findChildFiles(File::findFiles, false, "*.asc");

	for (auto &file : childFiles)
	{
		if (file.getCreationTime() < Time::getCurrentTime() - RelativeTime::days(3))
		{
			file.deleteFile();
		}
	}

	if (!logFile.getParentDirectory().hasWriteAccess())
	{
		RuntimePermissions::request(RuntimePermissions::writeExternalStorage, nullptr);
	}

	// The listeners are always registered, but they return immediately while logging is off, so
	// the cost of having them attached is a single atomic read per frame.
	canFrameReceivedListener = isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
		queue_frame(canFrame, false);
	});
	canFrameSentListener = isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
		queue_frame(canFrame, true);
	});

	startThread(Thread::Priority::low);
}

ASCIILogFile::~ASCIILogFile()
{
	// Give the writer a moment to get the last lines out before the file is closed
	stopThread(2000);
	logStream.reset();
}

void ASCIILogFile::set_logging_enabled(bool shouldLog)
{
	loggingEnabled = shouldLog;
}

bool ASCIILogFile::is_logging_enabled()
{
	return loggingEnabled;
}

void ASCIILogFile::queue_frame(const isobus::CANMessageFrame &canFrame, bool wasTransmitted)
{
	if (!loggingEnabled)
	{
		return;
	}

	auto currentTime = Time::getCurrentTime() - initialTimestamp;
	auto milliseconds = isobus::to_string(currentTime.inMilliseconds() % 1000);

	while (milliseconds.length() < 3)
	{
		milliseconds = "0" + milliseconds;
	}

	// Built as one string, because every separate write to the file used to cost an open and a
	// close of it, and there were a dozen of those per frame
	String line;

	line << "   "
	     << String(isobus::to_string(std::floor(currentTime.inSeconds())))
	     << "."
	     << String(milliseconds)
	     << "000 1  "
	     << String::toHexString(canFrame.identifier).toUpperCase()
	     << (wasTransmitted ? "x       Tx   d " : "x       Rx   d ")
	     << String(static_cast<int>(canFrame.dataLength))
	     << " ";

	for (std::uint_fast8_t i = 0; i < canFrame.dataLength; i++)
	{
		line << String::toHexString(canFrame.data[i]).paddedLeft('0', 2).toUpperCase() << " ";
	}

	for (std::uint_fast8_t i = canFrame.dataLength; i < 8; i++)
	{
		line << "00 ";
	}
	line << "\n";

	{
		const ScopedLock lock(queueLock);

		// If the disk cannot keep up, drop frames rather than grow without bound. Logging is a
		// diagnostic aid and must never be the reason the program runs out of memory.
		if (pendingLines.size() >= MAXIMUM_QUEUED_LINES)
		{
			if (!hasReportedOverflow.exchange(true))
			{
				LOG_WARNING("[CAN Logger]: The log queue is full, some frames will be missing from the .asc file.");
			}
			return;
		}
		pendingLines.push_back(line);
	}
	notify();
}

bool ASCIILogFile::open_file_if_needed()
{
	if (nullptr != logStream)
	{
		return true;
	}

	if (!logFile.getParentDirectory().hasWriteAccess())
	{
		return false;
	}

	logStream = logFile.createOutputStream();

	if (nullptr == logStream)
	{
		return false;
	}

	// Vector ascii header
	logStream->writeText("date " + Time::getCurrentTime().toString(true, true, true, false) + "\n", false, false, nullptr);
	logStream->writeText("base hex timestamps absolute\n", false, false, nullptr);
	logStream->writeText("no internal events logged\n", false, false, nullptr);
	return true;
}

void ASCIILogFile::run()
{
	while (!threadShouldExit())
	{
		std::vector<String> linesToWrite;
		{
			const ScopedLock lock(queueLock);
			linesToWrite.swap(pendingLines);
		}

		if (!linesToWrite.empty() && open_file_if_needed())
		{
			for (const auto &line : linesToWrite)
			{
				logStream->writeText(line, false, false, nullptr);
			}
			logStream->flush();
		}

		if (!threadShouldExit())
		{
			wait(WRITE_INTERVAL_MS);
		}
	}

	// Drain whatever is left so the tail of a session is not lost on shutdown
	std::vector<String> remaining;
	{
		const ScopedLock lock(queueLock);
		remaining.swap(pendingLines);
	}

	if (!remaining.empty() && open_file_if_needed())
	{
		for (const auto &line : remaining)
		{
			logStream->writeText(line, false, false, nullptr);
		}
		logStream->flush();
	}
}

std::string ASCIILogFile::currentLogFile() const
{
	return logFile.getFullPathName().toStdString();
}
