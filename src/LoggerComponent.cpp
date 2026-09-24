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

void LoggerComponent::sink_CAN_stack_log(LoggingLevel level, const std::string &logText)
{
	// This sink is called from arbitrary isobus stack threads, potentially while those
	// threads hold an isobus-internal mutex (e.g. a transport protocol session lock) that
	// the main/UI thread also needs during its own periodic work. A synchronous
	// MessageManagerLock() here used to block the calling thread until the UI thread was
	// free - if the UI thread was itself waiting on that same isobus-internal mutex, the
	// two threads deadlocked each other permanently (the whole app, since the UI thread
	// drives the message pump).
	//
	// Posting one juce::MessageManager::callAsync() per call fixed that deadlock but traded
	// it for a different problem: this sink fires on every single LOG_DEBUG in the stack, so
	// under real VT traffic (a client toggling several objects a second) it was flooding the
	// JUCE message queue with one closure per log line - and JUCE's own Timer callbacks are
	// dispatched through that same queue, so the working-set repaint timer got starved behind
	// the backlog and stopped firing in practice, even though nothing was deadlocked anymore.
	//
	// Instead, just buffer the message behind a small dedicated mutex (never held across any
	// isobus stack call, so it can't itself become a deadlock the way the isobus-internal
	// mutexes could) and let pump_pending_messages(), called from ServerMainComponent's
	// existing periodic timer, apply the whole backlog and repaint at most once per tick.
	logMessage(logText);

	const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
	pendingMessages.push_back({ logText, level });
}

void LoggerComponent::pump_pending_messages()
{
	std::deque<LogData> drained;
	{
		const std::lock_guard<std::mutex> lock(pendingMessagesMutex);
		if (pendingMessages.empty())
		{
			return;
		}
		drained.swap(pendingMessages);
	}

	auto bounds = getLocalBounds();

	for (auto it = drained.rbegin(); it != drained.rend(); ++it)
	{
		loggedMessages.push_front(*it);
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
	setSize(bounds.getWidth(), newSize);
	repaint();
}

std::uint64_t LoggerComponent::initialPos() const
{
	return startPos;
}
