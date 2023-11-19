//================================================================================================
/// @file OutputEllipseComponent.hpp
///
/// @brief Defines a GUI component to draw an input number.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_ELLIPSE_COMPONENT_HPP
#define OUTPUT_ELLIPSE_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputEllipseComponent : public isobus::OutputEllipse
  , public Component
{
public:
	OutputEllipseComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputEllipse sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputEllipseComponent)
};

#endif // OUTPUT_ELLIPSE_COMPONENT_HPP
