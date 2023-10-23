//================================================================================================
/// @file OutputLinearBarGraphComponent.hpp
///
/// @brief Defines a GUI component to draw a linear bar graph.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_LINEAR_BAR_GRAPH_COMPONENT_HPP
#define OUTPUT_LINEAR_BAR_GRAPH_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputLinearBarGraphComponent : public isobus::OutputLinearBarGraph
  , public Component
{
public:
	OutputLinearBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLinearBarGraph sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputLinearBarGraphComponent);
};

#endif // OUTPUT_LINEAR_BAR_GRAPH_COMPONENT_HPP
