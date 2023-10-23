/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ServerMainComponent.hpp"
#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"

//==============================================================================
class AgISOUniversalTerminalApplication : public juce::JUCEApplication
{
public:
	//==============================================================================
	AgISOUniversalTerminalApplication() {}

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
	void initialise(const juce::String &commandLine) override
	{
		// This method is where you should put your application's initialisation code..

		mainWindow.reset(new MainWindow(getApplicationName()));
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

	void anotherInstanceStarted(const juce::String &commandLine) override
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
		MainWindow(juce::String name) :
		  DocumentWindow(name,
		                 juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
		                 DocumentWindow::allButtons)
		{
			std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
			canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
			canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(static_cast<WORD>(PCAN_USBBUS1));
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
			canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
			canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_TOUCAN_AVAILABLE)
			canDriver = std::make_shared<isobus::TouCANPlugin>(static_cast<std::int16_t>(0), change_me_to_your_serial_number);
#endif
			isobus::CANHardwareInterface::set_number_of_can_channels(1);
			isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
			isobus::CANHardwareInterface::start();

			isobus::NAME serverNAME(0);
			serverNAME.set_arbitrary_address_capable(true);
			serverNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
			serverNAME.set_industry_group(2);
			serverNAME.set_manufacturer_code(64);
			serverInternalControlFunction = isobus::InternalControlFunction::create(serverNAME, 0x26, 0);
			setUsingNativeTitleBar(true);
			setContentOwned(new ServerMainComponent(serverInternalControlFunction), true);

#if JUCE_IOS || JUCE_ANDROID
			setFullScreen(true);
#else
			setResizable(true, true);
			centreWithSize(getWidth(), getHeight());
#endif

			setVisible(true);
		}

		void closeButtonPressed() override
		{
			// This is called when the user tries to close this window. Here, we'll just
			// ask the app to quit when this happens, but you can change this to do
			// whatever you need.
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

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(AgISOUniversalTerminalApplication)
