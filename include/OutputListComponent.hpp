//================================================================================================
/// @file OutputListComponent.hpp
///
/// @brief Defines a GUI component to draw an output list.
/// @author Sujan Dumaru
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef OUTPUT_LIST_COMPONENT_HPP
#define OUTPUT_LIST_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputListComponent : public isobus::OutputList
  , public Component
{
public:
	OutputListComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputList sourceObject);

	/// @brief Gets the index of the item to show, which can be past the end of the list. Drawing and press handling share this so they agree.
	/// @param list The list to read. A Number Variable reference overrides its value.
	/// @param workingSet The working set that owns the list
	/// @returns The index of the item to show
	static std::uint32_t get_selected_index(const isobus::OutputList &list, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

private:
	std::shared_ptr<Component> childComponent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputListComponent)
};

#endif // OUTPUT_LIST_COMPONENT_HPP
