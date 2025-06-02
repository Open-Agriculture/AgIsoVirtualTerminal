/*******************************************************************************
** @file       ObjectPointerComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ObjectPointerComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

ObjectPointerComponent::ObjectPointerComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::ObjectPointer sourceObject) :
  isobus::ObjectPointer(sourceObject),
  parentWorkingSet(workingSet)
{
	on_content_changed(true);
}

void ObjectPointerComponent::on_content_changed(bool initial)
{
	childComponent.reset();
	auto child = get_object_by_id(get_value(), parentWorkingSet->get_object_tree());

	if (nullptr != child)
	{
		childComponent = JuceManagedWorkingSetCache::create_component(parentWorkingSet, child);

		if (nullptr != childComponent)
		{
			int w = 0, h = 0;
			addAndMakeVisible(*childComponent);
			childComponent->setTopLeftPosition(get_child_x(0), get_child_y(0));
			std::static_pointer_cast<ObjectPointerComponent>(childComponent)->getChildSizeRecursive(w, h);
			setSize(w, h);
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void ObjectPointerComponent::paint(Graphics &g)
{
}

void ObjectPointerComponent::getChildSizeRecursive(int &w, int &h) const
{
	auto child = get_object_by_id(get_value(), parentWorkingSet->get_object_tree());
	if (nullptr != child)
	{
		if (child->get_object_type() == isobus::VirtualTerminalObjectType::ObjectPointer)
		{
			int numChildren = getNumChildComponents();
			if (numChildren == 1)
			{
				static_cast<ObjectPointerComponent *>(getChildComponent(0))->getChildSizeRecursive(w, h);
			}
		}
		else
		{
			w = child->get_width();
			h = child->get_height();
		}
	}
}
