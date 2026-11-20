//================================================================================================
/// @file ConfigureHardwareComponent.hpp
///
/// @brief Defines a GUI component which allows selecting and configuring a hardware interface.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Contributors
//================================================================================================
#ifndef CONFIGURE_HARDWARE_COMPONENT_HPP
#define CONFIGURE_HARDWARE_COMPONENT_HPP

#include "CANDeviceList.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include <JuceHeader.h>

class ConfigureHardwareWindow;

class ConfigureHardwareComponent : public Component
{
public:
	ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers);

	void paint(Graphics &graphics) override;

	void resized() override;

private:
	/// @brief Shows the options of the selected driver: the TouCAN serial number or the list of attached adapters
	void update_driver_options();

	/// @brief Fills the adapter list for the selected driver and selects the adapter it uses now
	void fill_device_list(std::size_t driverIndex);

	/// @brief Switches the selected driver to the adapter picked in the list, if it has a list
	void apply_selected_device();

	ConfigureHardwareWindow &parentWindow;
	ComboBox hardwareInterfaceSelector;
	ComboBox deviceSelector;
	std::vector<CANDeviceList::Device> listedDevices; ///< The adapters in deviceSelector, in its order
	TextEditor socketCANNameEditor;
	TextEditor touCANSerialEditor;
	TextButton okButton;
	std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &parentCANDrivers;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigureHardwareComponent)
};

#endif // CONFIGURE_HARDWARE_COMPONENT_HPP
