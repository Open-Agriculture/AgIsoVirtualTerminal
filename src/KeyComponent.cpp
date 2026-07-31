/*******************************************************************************
** @file       KeyComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "KeyComponent.hpp"

#include "JuceManagedWorkingSetCache.hpp"

KeyComponent::KeyComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Key sourceObject, int keyWidth, int keyHeight) :
  isobus::Key(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(keyWidth, keyHeight);
	setOpaque(true);

	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(get_child_x(i), get_child_y(i));
			}
		}
	}
}

void KeyComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}

uint8_t KeyComponent::getKeyPosition() const
{
	return keyPosition;
}

void KeyComponent::setKeyPosition(std::uint8_t pos)
{
	keyPosition = pos;
}
