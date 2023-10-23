//================================================================================================
/// @file OutputStringComponent.hpp
///
/// @brief Defines a GUI component to draw an output string.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_STRING_COMPONENT_HPP
#define OUTPUT_STRING_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputStringComponent : public isobus::OutputString
  , public Component
{
public:
	OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject);

	void paint(Graphics &g) override;

	static Justification convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStringComponent);
};

#endif // OUTPUT_STRING_COMPONENT_HPP
