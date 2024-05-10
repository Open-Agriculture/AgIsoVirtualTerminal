//================================================================================================
/// @file WorkingSetComponent.hpp
///
/// @brief Defines a GUI component to draw a working set designator.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef WORKING_SET_COMPONENT_HPP
#define WORKING_SET_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class WorkingSetComponent : public isobus::WorkingSet
  , public Component
{
public:
	WorkingSetComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::WorkingSet sourceObject, int keyHeight, int keyWidth);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetComponent)
};

#endif // WORKING_SET_COMPONENT_HPP
