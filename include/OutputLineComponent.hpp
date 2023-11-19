//================================================================================================
/// @file OutputLineComponent.hpp
///
/// @brief Defines a GUI component to draw an output line.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_LINE_COMPONENT_HPP
#define OUTPUT_LINE_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputLineComponent : public isobus::OutputLine
  , public Component
{
public:
	OutputLineComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLine sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputLineComponent)
};

#endif // OUTPUT_LINE_COMPONENT_HPP
