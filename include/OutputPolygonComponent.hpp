//================================================================================================
/// @file OutputPolygonComponent.hpp
///
/// @brief Defines a GUI component to draw an output polygon.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_POLYGON_COMPONENT_HPP
#define OUTPUT_POLYGON_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputPolygonComponent : public isobus::OutputPolygon
  , public Component
{
public:
	OutputPolygonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputPolygon sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputPolygonComponent)
};

#endif // OUTPUT_POLYGON_COMPONENT_HPP
