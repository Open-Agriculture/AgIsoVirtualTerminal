/*******************************************************************************
** @file       Main.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "isobus/hardware_integration/available_can_drivers.hpp"

#include <JuceHeader.h>
#include "ASCIILogFile.hpp"
#include "AppImages.h"
#include "ServerMainComponent.hpp"
#include "Settings.hpp"
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
	bool moreThanOneInstanceAllowed() override
	{
		return true;
	}

	//==============================================================================
	void initialise(const juce::String &commandLineParameters) override
	{
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

		mainWindow.reset(new MainWindow(getApplicationName(), vtNumber));
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

	//==============================================================================
	/*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
	class MainWindow : public juce::DocumentWindow
	{
	public:
		MainWindow(juce::String name, std::uint8_t vtNumberCmdLineArg = 0) :
		  DocumentWindow(name,
		                 juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
		                 DocumentWindow::allButtons)
		{
			int vtNumber = vtNumberCmdLineArg;
#ifdef JUCE_WINDOWS
			canDrivers.push_back(std::make_shared<isobus::PCANBasicWindowsPlugin>(static_cast<WORD>(PCAN_USBBUS1)));
#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
			canDrivers.push_back(std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0));
#else
			canDrivers.push_back(nullptr);
#endif
			canDrivers.push_back(std::make_shared<isobus::TouCANPlugin>(static_cast<std::int16_t>(0), 0));
			canDrivers.push_back(std::make_shared<isobus::SysTecWindowsPlugin>());
#elif defined(JUCE_MAC)
			canDrivers.push_back(std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1));
#else
			canDrivers.push_back(std::make_shared<isobus::SocketCANInterface>("can0"));
#endif

			jassert(!canDrivers.empty()); // You need some kind of CAN interface to run this program!
			isobus::CANHardwareInterface::set_number_of_can_channels(1);

#ifndef JUCE_WINDOWS
			isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDrivers.at(0));
#endif
			isobus::NAME serverNAME(0);

			Settings settings;
			if (!settings.load_settings())
			{
				{
					isobus::CANStackLogger::info("Config file not found, using defaults.");
#ifdef JUCE_LINUX
					std::static_pointer_cast<isobus::SocketCANInterface>(canDrivers.at(0))->set_name("can0");
					isobus::CANStackLogger::warn("Socket CAN interface name not yet configured. Using default of \"can0\"");
#endif
				}
			}
			else
			{
				if (0 == vtNumberCmdLineArg)
				{
					// no command line argument provided -> use the saved setting
					serverNAME.set_function_instance(settings.vt_number() - 1);
					vtNumber = settings.vt_number();
				}
				else
				{
					// VT number provided from the vtNumberCmdLineArg line
					serverNAME.set_function_instance(settings.vt_number() - 1);
				}
			}

			serverNAME.set_arbitrary_address_capable(true);
			serverNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
			serverNAME.set_industry_group(2);
			serverNAME.set_manufacturer_code(1407);
			serverInternalControlFunction = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(serverNAME, 0, 0x26);
			setUsingNativeTitleBar(true);
			setContentOwned(new ServerMainComponent(serverInternalControlFunction, canDrivers, settings.settingsValueTree(), vtNumber), true);

#if JUCE_IOS || JUCE_ANDROID
			setFullScreen(true);
#else
			setResizable(true, true);
			centreWithSize(getWidth(), getHeight());
#endif

			setIcon(ImageCache::getFromMemory(AppImages::logosmall_png, AppImages::logosmall_pngSize));
#if JUCE_LINUX
			// this hack is needed on Linux
			ComponentPeer *peer = getPeer();
			if (peer)
			{
				peer->setIcon(ImageCache::getFromMemory(AppImages::logosmall_png, AppImages::logosmall_pngSize));
			}
#endif
			setVisible(true);
		}

		void closeButtonPressed() override
		{
			// This is called when the user tries to close this window. Here, we'll just
			// ask the app to quit when this happens, but you can change this to do
			// whatever you need.
			isobus::CANHardwareInterface::stop();
			JUCEApplication::getInstance()->systemRequestedQuit();
		}

		/* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

	private:
		std::shared_ptr<isobus::InternalControlFunction> serverInternalControlFunction;
		std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> canDrivers;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	std::unique_ptr<MainWindow> mainWindow;
	ASCIILogFile logFile;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(AgISOVirtualTerminalApplication)
