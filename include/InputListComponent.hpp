//================================================================================================
/// @file InputListComponent.hpp
///
/// @brief Defines a GUI component to draw an input list.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef INPUT_LIST_COMPONENT_HPP
#define INPUT_LIST_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class InputListComponent : public isobus::InputList
  , public Component
{
public:
	InputListComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputList sourceObject);

	void paint(Graphics &g) override;
	void paintOverChildren(Graphics &g) override;

	void onChanged(bool initial);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::shared_ptr<Component> childComponent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputListComponent)
};

#endif // INPUT_LIST_COMPONENT_HPP
