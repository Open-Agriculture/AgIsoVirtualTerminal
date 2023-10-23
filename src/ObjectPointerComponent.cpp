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
	auto child = get_object_by_id(get_child_id(0));

	if (nullptr != child)
	{
		childComponent = JuceManagedWorkingSetCache::create_component(parentWorkingSet, child);

		if (nullptr != childComponent)
		{
			addAndMakeVisible(*childComponent);
			childComponent->setTopLeftPosition(get_child_x(0), get_child_y(0));
			setSize(child->get_width(), child->get_height());
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void ObjectPointerComponent::paint(Graphics &)
{
}
