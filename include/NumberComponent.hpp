//================================================================================================
/// @file NumberComponent.hpp
///
/// @brief Common functions for drawing numbers
/// @author Miklos Marton
///
/// @copyright 2024 Adrian Del Grosso
//================================================================================================
#ifndef NUMBER_COMPONENT_HPP
#define NUMBER_COMPONENT_HPP

#include "TextDrawingComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class NumberComponent : public TextDrawingComponent
{
public:
	NumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

protected:
	void paintNumber(Graphics &g, bool enabled = true);

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberComponent)
};

#endif // NUMBER_COMPONENT_HPP
