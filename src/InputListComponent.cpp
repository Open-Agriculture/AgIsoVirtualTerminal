//================================================================================================
/// @file InputListComponent.cpp
///
/// @brief This is a class for drawing an input list.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "InputListComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

InputListComponent::InputListComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputList sourceObject) :
  isobus::InputList(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
	onChanged(true);
}

void InputListComponent::paint(Graphics &)
{
}

void InputListComponent::onChanged(bool initial)
{
	childComponent.reset();

	if (get_number_children() > 1)
	{
		std::uint32_t selectedIndex = get_value();

		for (std::uint16_t i = 0; i < get_number_children(); i++)
		{
			auto child = get_object_by_id(get_child_id(i));

			if (nullptr != child)
			{
				if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
				{
					selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
					break;
				}
			}
		}

		if (selectedIndex < static_cast<std::uint32_t>(get_number_children() - 1))
		{
			// The number variable will always be the first one
			auto listItem = get_object_by_id(get_child_id(static_cast<std::uint16_t>(selectedIndex + 1)));
			childComponent = JuceManagedWorkingSetCache::create_component(parentWorkingSet, listItem);

			if (nullptr != childComponent)
			{
				addAndMakeVisible(*childComponent);
				childComponent->setTopLeftPosition(0, 0);
			}
		}
	}

	if (!initial)
	{
		repaint();
	}
}
