//================================================================================================
/// @file WorkingSetSelectorComponent.hpp
///
/// @brief Defines a GUI component allow selecting the active working set.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef WORKING_SET_SELECTOR_COMPONENT_HPP
#define WORKING_SET_SELECTOR_COMPONENT_HPP

#include "WorkingSetComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

#include <vector>

class WorkingSetSelectorComponent : public Component
{
public:
	WorkingSetSelectorComponent();

	void add_working_set_to_draw(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);
	void remove_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	void paint(Graphics &g) override;
	void resized() override;

private:
	struct SELECTOR_CHILD_OBJECTS_STRUCT
	{
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		std::vector<std::shared_ptr<Component>> childComponents;
	};
	std::vector<SELECTOR_CHILD_OBJECTS_STRUCT> children;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetSelectorComponent);
};

#endif // WORKING_SET_SELECTOR_COMPONENT_HPP