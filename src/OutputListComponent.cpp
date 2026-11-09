/*******************************************************************************
** @file       OutputListComponent.cpp
** @author     Sujan Dumaru
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputListComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

OutputListComponent::OutputListComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputList sourceObject) :
  isobus::OutputList(sourceObject)
{
	setSize(get_width(), get_height());

	const std::uint32_t selectedIndex = get_selected_index(*this, workingSet);

	// An index past the end of the list, such as 255, is valid and shows nothing
	if (selectedIndex < get_number_children())
	{
		auto listItem = get_object_by_id(get_child_id(static_cast<std::uint16_t>(selectedIndex)), workingSet->get_object_tree());
		childComponent = JuceManagedWorkingSetCache::create_component(workingSet, listItem);

		if (nullptr != childComponent)
		{
			addAndMakeVisible(*childComponent);
		}
	}
}

std::uint32_t OutputListComponent::get_selected_index(const isobus::OutputList &list, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	std::uint32_t selectedIndex = list.get_value();
	auto variable = isobus::VTObject::get_object_by_id(list.get_variable_reference(), workingSet->get_object_tree());

	if ((nullptr != variable) && (isobus::VirtualTerminalObjectType::NumberVariable == variable->get_object_type()))
	{
		selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(variable)->get_value();
	}
	return selectedIndex;
}
