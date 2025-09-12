/*******************************************************************************
** @file       Main.hpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#pragma once

#include <JuceHeader.h>
#include "ASCIILogFile.hpp"
#include "AppImages.h"
#include "ServerMainComponent.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"

//==============================================================================
class AgISOVirtualTerminalApplication : public juce::JUCEApplication
{
public:
	//==============================================================================
	AgISOVirtualTerminalApplication() {}

	const juce::String getApplicationName() override
	{
		return ProjectInfo::projectName;
	}

	const juce::String getApplicationVersion() override
	{
		return ProjectInfo::versionString;
	}

	static std::string getApplicationBuildInfo();
	static std::string getApplicationNameWithBuildInfo();

	bool moreThanOneInstanceAllowed() override
	{
		return true;
	}

	//==============================================================================
	void initialise(const juce::String &commandLineParameters) override
	{
		SystemStats::setApplicationCrashHandler(onCrash);

		juce::StringArray args;
		args.addTokens(commandLineParameters, true);

		std::uint8_t vtNumber = 0;
		for (const auto &arg : args)
		{
			if (arg.startsWith("--vt-number"))
			{
				vtNumber = arg.fromFirstOccurrenceOf("--vt-number=", false, false).getIntValue();
				if (0 == vtNumber || vtNumber > 32)
				{
					std::cout << "The VT number must be between 1 and 32";
					vtNumber = 0;
				}
			}
		}

		mainWindow.reset(new MainWindow(getApplicationNameWithBuildInfo(), logFile.currentLogFile(), vtNumber));
	}

	void shutdown() override
	{
		// Add your application's shutdown code here..

		mainWindow = nullptr; // (deletes our window)
	}

	//==============================================================================
	void systemRequestedQuit() override
	{
		// This is called when the app is being asked to quit: you can ignore this
		// request and let the app carry on running, or call quit() to allow the app to close.
		quit();
	}

	void anotherInstanceStarted(const juce::String &) override
	{
		// When another instance of the app is launched while this one is running,
		// this method is invoked, and the commandLine parameter tells you what
		// the other instance's command-line arguments were.
	}

	static void onCrash(void *)
	{
		auto stackTrace = SystemStats::getStackBacktrace();
		auto appDataDir = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
		                       File::getSeparatorString() +
		                       "Open-Agriculture" +
		                       File::getSeparatorString());
		auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
		auto fileNameTime = currentTime;
		fileNameTime = currentTime.replaceCharacter(' ', '_');
		fileNameTime = currentTime.replaceCharacter(':', '_');

		if (!appDataDir.exists())
		{
			appDataDir.createDirectory();
		}

		auto outputFile = appDataDir.getChildFile(JUCEApplication::getInstance()->getApplicationName() + "_crash_" + currentTime + ".txt");
		outputFile.appendText(stackTrace);
	}

	//==============================================================================
	/*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
	class MainWindow : public juce::DocumentWindow
	{
	public:
		/**
     * @brief MainWindow
     * @param name - window name to be displayed in the window title
     * @param vtNumberCmdLineArg - in the range of 1 - 32
     */
		MainWindow(juce::String name, const std::string &canLogPath, int vtNumberCmdLineArg = 0);

		/* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */
		void closeButtonPressed() override;

	private:
		std::shared_ptr<isobus::InternalControlFunction> serverInternalControlFunction;
		std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> canDrivers;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	std::unique_ptr<MainWindow> mainWindow;
	ASCIILogFile logFile;
};
