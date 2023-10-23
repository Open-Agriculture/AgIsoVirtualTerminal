//================================================================================================
/// @file InputNumberComponent.hpp
///
/// @brief Defines a GUI component to draw an input number.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef INPUT_NUMBER_COMPONENT_HPP
#define INPUT_NUMBER_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class InputNumberComponent : public isobus::InputNumber
  , public Component
{
public:
	InputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputNumber sourceObject);

	void paint(Graphics &g) override;

	static Justification convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputNumberComponent);
};

#endif // INPUT_NUMBER_COMPONENT_HPP
