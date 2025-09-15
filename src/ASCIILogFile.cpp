/*******************************************************************************
** @file       ASCIILogFile.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ASCIILogFile.hpp"

#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

ASCIILogFile::ASCIILogFile()
{
	auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
	initialTimestamp = Time::getCurrentTime();
	auto fileNameTime = currentTime;
	fileNameTime = currentTime.replaceCharacter(' ', '_');
	fileNameTime = currentTime.replaceCharacter(':', '_');

	logFile = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
	               File::getSeparatorString() +
	               "Open-Agriculture" +
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

	// Write vector ascii header
	if (logFile.hasWriteAccess())
	{
		logFile.appendText("date " + currentTime + "\n");
		logFile.appendText("base hex timestamps absolute\n");
		logFile.appendText("no internal events logged\n");
		canFrameReceivedListener = isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
			logFile.appendText("   ");
			auto currentTime = Time::getCurrentTime() - initialTimestamp;
			auto milliseconds = isobus::to_string(currentTime.inMilliseconds() % 1000);

			while (milliseconds.length() < 3)
			{
				milliseconds = "0" + milliseconds;
			}

			logFile.appendText(isobus::to_string(std::floor(currentTime.inSeconds())) +
			                   "." +
			                   milliseconds +
			                   "000 1  " +
			                   String::toHexString(canFrame.identifier).toUpperCase().toStdString() +
			                   "x       Rx   d " +
			                   isobus::to_string(static_cast<int>(canFrame.dataLength)) +
			                   " ");

			for (std::uint_fast8_t i = 0; i < canFrame.dataLength; i++)
			{
				logFile.appendText(String::toHexString(canFrame.data[i]).paddedLeft('0', 2).toUpperCase().toStdString() + " ");
			}

			for (std::uint_fast8_t i = canFrame.dataLength; i < 8; i++)
			{
				logFile.appendText("00 ");
			}
			logFile.appendText("\n");
		});

		canFrameSentListener = isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
			logFile.appendText("   ");
			auto currentTime = Time::getCurrentTime() - initialTimestamp;
			auto milliseconds = isobus::to_string(currentTime.inMilliseconds() % 1000);

			while (milliseconds.length() < 3)
			{
				milliseconds = "0" + milliseconds;
			}

			logFile.appendText(isobus::to_string(std::floor(currentTime.inSeconds())) +
			                   "." +
			                   milliseconds +
			                   "000 1  " +
			                   String::toHexString(canFrame.identifier).toUpperCase().toStdString() +
			                   "x       Tx   d " +
			                   isobus::to_string(static_cast<int>(canFrame.dataLength)) +
			                   " ");

			for (std::uint_fast8_t i = 0; i < canFrame.dataLength; i++)
			{
				logFile.appendText(String::toHexString(canFrame.data[i]).paddedLeft('0', 2).toUpperCase().toStdString() + " ");
			}

			for (std::uint_fast8_t i = canFrame.dataLength; i < 8; i++)
			{
				logFile.appendText("00 ");
			}
			logFile.appendText("\n");
		});
	}
	else
	{
		RuntimePermissions::request(RuntimePermissions::writeExternalStorage, nullptr);
	}
}

std::string ASCIILogFile::currentLogFile() const
{
	return logFile.getFullPathName().toStdString();
}
