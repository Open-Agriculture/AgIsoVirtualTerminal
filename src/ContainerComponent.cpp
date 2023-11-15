/*******************************************************************************
** @file       ContainerComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ContainerComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

ContainerComponent::ContainerComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Container sourceObject) :
  isobus::Container(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i));

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				if (get_hidden())
				{
					addChildComponent(*childComponents.back());
				}
				else
				{
					addAndMakeVisible(*childComponents.back());
				}
				childComponents.back()->setTopLeftPosition(get_child_x(i), get_child_y(i));
			}
		}
	}
}

void ContainerComponent::paint(Graphics &)
{
	// g.fillAll(Colour::fromFloatRGBA(0.0, 0.0, 0.0, 0.0));
	if (get_hidden())
	{
		for (auto &child : childComponents)
		{
			if (nullptr != child)
			{
				child->setVisible(false);
			}
		}
		this->setVisible(false);
	}
	else
	{
		for (auto &child : childComponents)
		{
			if (nullptr != child)
			{
				child->setVisible(true);
			}
		}
		this->setVisible(true);
	}
}
