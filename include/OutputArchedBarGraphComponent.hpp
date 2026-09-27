//================================================================================================
/// @file OutputArchedBarGraphComponent.hpp
///
/// @brief Defines a GUI component to draw an arched bar graph.
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP
#define OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputArchedBarGraphComponent : public isobus::OutputArchedBarGraph
  , public Component
{
public:
	OutputArchedBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputArchedBarGraph sourceObject);

	void paint(Graphics &g) override;

private:
	/// @brief Returns the value of the referenced number variable, or the fallback if there is no such variable
	std::uint16_t get_referenced_value(std::uint16_t variableReference, std::uint16_t fallback) const;

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputArchedBarGraphComponent)
};

#endif // OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP
