/*******************************************************************************
** @file       InputListComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
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

void InputListComponent::paintOverChildren(Graphics &g)
{
	if (!get_option(Options::Enabled))
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}

void InputListComponent::onChanged(bool initial)
{
	childComponent.reset();

	std::uint32_t selectedIndex = get_value();

	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto child = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
			{
				selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
			}
		}
	}

	if ((get_number_children() > 0) &&
	    (selectedIndex < static_cast<std::uint32_t>(get_number_children())))
	{
		// The number variable will always be the first one
		auto listItem = get_object_by_id(get_child_id(static_cast<std::uint16_t>(selectedIndex)), parentWorkingSet->get_object_tree());
		childComponent = JuceManagedWorkingSetCache::create_component(parentWorkingSet, listItem);

		if (nullptr != childComponent)
		{
			addAndMakeVisible(*childComponent);
			childComponent->setTopLeftPosition(0, 0);
		}
	}

	if (!initial)
	{
		repaint();
	}
}
