//================================================================================================
/// @file LoggerComponent.cpp
///
/// @brief Implements a GUI component to draw log output.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "LoggerComponent.hpp"

LoggerComponent::LoggerComponent() :
  FileLogger(File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() + "/Open-Agriculture/AgISOVirtualTerminalLog.txt"), "Starting AgIsoVirtualTerminal", 1024000)
{
	auto bounds = getLocalBounds();
	setBounds(10, 10, bounds.getWidth() - 10, bounds.getHeight() - 10);
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
	const auto mmLock = MessageManagerLock();
	auto bounds = getLocalBounds();

	loggedMessages.push_front({ logText, level });

	if (loggedMessages.size() > MAX_NUMBER_MESSAGES)
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
	logMessage(logText);
}
