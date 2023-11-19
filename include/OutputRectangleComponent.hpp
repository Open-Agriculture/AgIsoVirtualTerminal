//================================================================================================
/// @file OutputRectangleComponent.hpp
///
/// @brief Defines a GUI component to draw an output rectangle.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_RECTANGLE_COMPONENT_HPP
#define OUTPUT_RECTANGLE_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputRectangleComponent : public isobus::OutputRectangle
  , public Component
{
public:
	OutputRectangleComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputRectangle sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputRectangleComponent)
};

#endif // OUTPUT_RECTANGLE_COMPONENT_HPP
