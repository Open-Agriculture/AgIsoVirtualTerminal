/*******************************************************************************
** @file       ConfigureHardwareComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ConfigureHardwareComponent.hpp"

#include "ConfigureHardwareWindow.hpp"
#include "ServerMainComponent.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#ifdef JUCE_WINDOWS
#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#elif JUCE_LINUX
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

ConfigureHardwareComponent::ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  parentWindow(parent),
  okButton("OK"),
  parentCANDrivers(canDrivers)
{
	setSize(400, 280);
	okButton.setSize(100, 30);
	okButton.setTopLeftPosition(getWidth() / 2 - okButton.getWidth() / 2, 200);
	addAndMakeVisible(okButton);

#ifdef JUCE_WINDOWS
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN", "TouCAN", "SysTec", "candleLight / gs_usb" }, 1);
#else
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN (not supported with mingw)", "TouCAN", "SysTec", "candleLight / gs_usb (not supported with mingw)" }, 1);
#endif
	int selectedID = 1;

	for (std::uint8_t i = 0; i < parentCANDrivers.size(); i++)
	{
		if ((nullptr != parentCANDrivers.at(i)) &&
		    (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0)))
		{
			selectedID = i + 1;
			break;
		}
	}
	hardwareInterfaceSelector.setSelectedId(selectedID);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() { update_driver_options(); };
	addAndMakeVisible(hardwareInterfaceSelector);

	deviceSelector.setName("CAN Adapter");
	deviceSelector.setSize(getWidth() - 20, 30);
	deviceSelector.setTopLeftPosition(10, 140);
	addChildComponent(deviceSelector);

	auto inputFilter = new TextEditor::LengthAndCharacterRestriction(10, "1234567890");
	touCANSerialEditor.setName("TouCAN Serial Number");
	touCANSerialEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->get_serial_number()));
	touCANSerialEditor.setSize(getWidth() - 20, 30);
	touCANSerialEditor.setTopLeftPosition(10, 140);
	touCANSerialEditor.setInputFilter(inputFilter, true);
	addChildComponent(touCANSerialEditor);
	update_driver_options();
#elif JUCE_LINUX
	socketCANNameEditor.setName("SocketCAN Interface Name");
	socketCANNameEditor.setText(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
	socketCANNameEditor.setSize(getWidth() - 20, 30);
	socketCANNameEditor.setTopLeftPosition(10, 80);
	addAndMakeVisible(socketCANNameEditor);
#endif
	okButton.onClick = [this, &parent]() {
#ifdef JUCE_WINDOWS
		if (3 == hardwareInterfaceSelector.getSelectedId()) // TouCAN
		{
			int serial = touCANSerialEditor.getText().trim().getIntValue();
			std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1))->reconfigure(0, static_cast<std::uint32_t>(serial));
		}
		apply_selected_device();

		if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
		{
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
		}
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1));
		isobus::CANStackLogger::info("Updated assigned CAN driver.");
#elif JUCE_LINUX
		std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name(socketCANNameEditor.getText().toStdString());
		isobus::CANStackLogger::info("Updated socket CAN interface name to: " + socketCANNameEditor.getText().toStdString());
#endif
		parent.parentServer.save_settings();
		parent.exitModalState(1);
		parent.setVisible(false);
		// The dialog is hidden rather than destroyed, so its text caret would otherwise outlive it
		parent.parentServer.release_text_input_focus();
	};
}

void ConfigureHardwareComponent::paint(Graphics &graphics)
{
	auto bounds = getLocalBounds();
	graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	graphics.setColour(getLookAndFeel().findColour(Label::textColourId));
	graphics.setFont(16.0f);
#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Select the CAN driver to use", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#elif JUCE_LINUX
	graphics.drawFittedText("Enter the name of the CAN interface to use (like \"can0\")", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#endif

	graphics.setFont(12.0f);

#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);

	if (3 == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("TouCAN Serial Number", touCANSerialEditor.getBounds().getX(), touCANSerialEditor.getBounds().getY() - 14, touCANSerialEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
	else if (deviceSelector.isVisible())
	{
		graphics.drawFittedText("Adapter", deviceSelector.getBounds().getX(), deviceSelector.getBounds().getY() - 14, deviceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
#elif JUCE_LINUX
	graphics.drawFittedText("Socket CAN Interface Name", socketCANNameEditor.getBounds().getX(), socketCANNameEditor.getBounds().getY() - 14, socketCANNameEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
#endif
}

void ConfigureHardwareComponent::resized()
{
}

void ConfigureHardwareComponent::update_driver_options()
{
	const auto driverIndex = static_cast<std::size_t>(hardwareInterfaceSelector.getSelectedId() - 1);

	touCANSerialEditor.setVisible(3 == hardwareInterfaceSelector.getSelectedId());
	deviceSelector.setVisible(CANDeviceList::has_device_list(driverIndex));

	if (deviceSelector.isVisible())
	{
		fill_device_list(driverIndex);
	}
	repaint();
}

void ConfigureHardwareComponent::fill_device_list(std::size_t driverIndex)
{
	listedDevices = CANDeviceList::list_devices(driverIndex);
	const auto current = CANDeviceList::find_device(listedDevices, parentWindow.parentServer.get_selected_can_device(driverIndex));

	deviceSelector.clear(dontSendNotification);
	for (std::size_t i = 0; i < listedDevices.size(); i++)
	{
		deviceSelector.addItem(listedDevices.at(i).description, static_cast<int>(i) + 1);

		if (current.has_value() && (listedDevices.at(i).id == current->id) && (listedDevices.at(i).serial == current->serial))
		{
			deviceSelector.setSelectedId(static_cast<int>(i) + 1, dontSendNotification);
		}
	}

	if (0 == deviceSelector.getSelectedId())
	{
		deviceSelector.setSelectedId(1, dontSendNotification);
	}
}

void ConfigureHardwareComponent::apply_selected_device()
{
	const auto driverIndex = static_cast<std::size_t>(hardwareInterfaceSelector.getSelectedId() - 1);
	const auto listIndex = deviceSelector.getSelectedItemIndex();

	if (CANDeviceList::has_device_list(driverIndex) && (listIndex >= 0) && (static_cast<std::size_t>(listIndex) < listedDevices.size()))
	{
		parentWindow.parentServer.select_can_device(driverIndex, listedDevices.at(static_cast<std::size_t>(listIndex)));
	}
}
