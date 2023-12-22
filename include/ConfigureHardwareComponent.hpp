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
	ComboBox hardwareInterfaceSelector;
	TextEditor socketCANNameEditor;
	TextEditor touCANSerialEditor;
	TextButton okButton;
	std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &parentCANDrivers;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigureHardwareComponent)
};

#endif // CONFIGURE_HARDWARE_COMPONENT_HPP
