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
  FileLogger(File(ServerMainComponent::getAppDataDir() + "/AgISOVirtualTerminalLog.txt"),
             "Starting " + AgISOVirtualTerminalApplication::getApplicationNameWithBuildInfo(),
             1024000)
{
	// Note: no bounds are set here on purpose. This component is hosted by a Viewport, which
	// owns its position, and the owner sizes it once the log area is laid out.
	startPos = getLogFile().getSize();
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
	cancelPendingUpdate();
}

void LoggerComponent::sink_CAN_stack_log(LoggingLevel level, const std::string &logText)
{
	{
		const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
		pendingMessages.push_back({ logText, level });
	}
	logMessage(logText);
	triggerAsyncUpdate();
}

void LoggerComponent::handleAsyncUpdate()
{
	std::vector<LogData> messages;
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

	int newSize = static_cast<int>(loggedMessages.size()) * 14;

	if (newSize < getHeight())
	{
		newSize = getHeight();
	}
	setSize(getWidth(), newSize);
	repaint();
}

std::uint64_t LoggerComponent::initialPos() const
{
	return startPos;
}
