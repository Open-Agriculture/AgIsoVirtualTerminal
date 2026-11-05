//================================================================================================
/// @file LoggerComponent.cpp
///
/// @brief Implements a GUI component to draw log output.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "LoggerComponent.hpp"

#include "ServerMainComponent.hpp"

#include "Main.hpp"

LoggerComponent::LoggerComponent() :
  Thread("Log file writer")
{
	// No bounds here: the hosting Viewport positions this, and the owner sizes it
	const File logDirectory(ServerMainComponent::getAppDataDir());

	for (auto &file : logDirectory.findChildFiles(File::findFiles, false, "AgISOVirtualTerminalLog_*.txt"))
	{
		if (file.getLastModificationTime() < Time::getCurrentTime() - RelativeTime::days(LOG_RETENTION_DAYS))
		{
			file.deleteFile();
		}
	}

	logFile = logDirectory.getNonexistentChildFile("AgISOVirtualTerminalLog_" + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S"), ".txt", false);
	logFile.create(); // FileOutputStream does not create parent directories
	pendingFileLines.push_back(("Starting " + String(AgISOVirtualTerminalApplication::getApplicationNameWithBuildInfo())).toStdString());
	pendingFileLines.push_back(("Log started: " + Time::getCurrentTime().toString(true, true)).toStdString());
	startThread(Thread::Priority::low);
}

void LoggerComponent::paint(Graphics &g)
{
	g.fillAll(Colours::black);
	g.setFont(14.0f);

	int numberOfLinesFitted = getHeight() / 14;

	for (std::size_t i = 0; i < static_cast<int>(loggedMessages.size()) && i < numberOfLinesFitted; i++)
	{
		const auto &message = loggedMessages.at(i);

		switch (message.logLevel)
		{
			case LoggingLevel::Info:
			{
				g.setColour(Colours::white);
			}
			break;

			case LoggingLevel::Warning:
			{
				g.setColour(Colours::yellow);
			}
			break;

			case LoggingLevel::Error:
			case LoggingLevel::Critical:
			{
				g.setColour(Colours::red);
			}
			break;

			case LoggingLevel::Debug:
			{
				g.setColour(Colours::blueviolet);
			}
			break;

			default:
			{
				g.setColour(Colours::white);
			}
			break;
		}
		g.drawFittedText(message.logText, 0, static_cast<int>(i) * 14, getWidth(), 14, Justification::centredLeft, 1);
	}
}

LoggerComponent::~LoggerComponent()
{
	// Unset first, so the writer's last pass sees every line
	isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	stopThread(2000);
	cancelPendingUpdate();
}

void LoggerComponent::sink_CAN_stack_log(LoggingLevel level, const std::string &logText)
{
	DBG(String(logText));
	{
		const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
		pendingMessages.push_back({ logText, level });

		// Only the newest lines are shown anyway
		if (pendingMessages.size() > MAX_NUMBER_MESSAGES)
		{
			pendingMessages.pop_front();
		}

		if (pendingFileLines.size() < MAX_QUEUED_FILE_LINES)
		{
			pendingFileLines.push_back(logText);
		}
		else
		{
			droppedFileLines++;
		}
	}
	triggerAsyncUpdate();
	notify();
}

void LoggerComponent::handleAsyncUpdate()
{
	std::deque<LogData> messages;
	{
		const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
		messages.swap(pendingMessages);
	}

	if (messages.empty())
	{
		return;
	}

	for (auto &message : messages)
	{
		loggedMessages.push_front(std::move(message));
	}

	while (loggedMessages.size() > MAX_NUMBER_MESSAGES)
	{
		loggedMessages.pop_back();
	}

	setSize(getWidth(), jmax(getHeight(), static_cast<int>(loggedMessages.size()) * 14));
	repaint();
}

const File &LoggerComponent::getLogFile() const
{
	return logFile;
}

void LoggerComponent::run()
{
	bool isExiting = false;

	while (!isExiting)
	{
		// Read before draining, so the last pass still writes the tail
		isExiting = threadShouldExit();

		std::vector<std::string> lines;
		std::size_t droppedLines = 0;
		{
			const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
			lines.swap(pendingFileLines);
			std::swap(droppedLines, droppedFileLines);
		}

		if ((!lines.empty()) || (0 != droppedLines))
		{
			write_to_file(lines, droppedLines);
		}

		if (!isExiting)
		{
			wait(-1);
		}
	}
}

void LoggerComponent::write_to_file(const std::vector<std::string> &lines, std::size_t droppedLines)
{
	if (nullptr == logStream)
	{
		// Unbuffered, so every line written so far survives a crash
		logStream = std::make_unique<FileOutputStream>(logFile, 0);

		if (!logStream->openedOk())
		{
			// Drops this batch, the next one tries again
			logStream.reset();
			return;
		}
	}

	MemoryOutputStream batch;
	for (const auto &line : lines)
	{
		batch.write(line.data(), line.size());
		batch << newLine;
	}

	if (0 != droppedLines)
	{
		batch << "[Logger]: " << static_cast<int64>(droppedLines) << " lines were dropped because the log file could not keep up" << newLine;
	}
	logStream->write(batch.getData(), batch.getDataSize());

	if (logStream->getPosition() > 2 * MAX_FILE_SIZE_BYTES)
	{
		// Windows can not replace a file that is still open
		logStream.reset();
		trim_file();
	}
}

void LoggerComponent::trim_file()
{
	// Unlike FileLogger::trimFileSize, keeps the old file if the copy fails, for example on a full disk
	TemporaryFile trimmedFile(logFile);
	bool copied = false;
	{
		FileInputStream in(logFile);
		FileOutputStream out(trimmedFile.getFile());

		if (in.openedOk() && out.openedOk())
		{
			for (int i = 0; i < KEPT_FIRST_LINES; i++)
			{
				out << in.readNextLine() << newLine;
			}
			out << "[Logger]: Older lines were trimmed" << newLine;
			in.setPosition(in.getTotalLength() - MAX_FILE_SIZE_BYTES);
			in.readNextLine(); // Skips the partial first line
			out.writeFromInputStream(in, -1);
			out.flush();
			copied = in.getStatus().wasOk() && out.getStatus().wasOk();
		}
	}

	if (copied)
	{
		trimmedFile.overwriteTargetFileWithTemporary();
	}
}
