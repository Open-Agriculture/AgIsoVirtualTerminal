//================================================================================================
/// @file ButtonComponent.hpp
///
/// @brief Defines a GUI component to draw a button.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef BUTTON_COMPONENT_HPP
#define BUTTON_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class ButtonComponent : public isobus::Button
  , public Button
{
public:
	ButtonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Button sourceObject);

	void paint(Graphics &g) override;

	void paintButton(Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void set_options(std::uint8_t value) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonComponent)
};

#endif // BUTTON_COMPONENT_HPP
