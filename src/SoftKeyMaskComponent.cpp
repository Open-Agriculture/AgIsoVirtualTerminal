/*******************************************************************************
** @file       SoftKeyMaskComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "SoftKeyMaskComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

SoftKeyMaskComponent::SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject) :
  isobus::SoftKeyMask(sourceObject),
  parentWorkingSet(workingSet)
{
	setOpaque(true);
	setBounds(0, 0, 100, 480);
	on_content_changed(true);
}

void SoftKeyMaskComponent::on_content_changed(bool initial)
{
	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			childComponents.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());
				childComponents.back()->setTopLeftPosition(10, 10 + (60 * i) + (10 * i));
			}
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void SoftKeyMaskComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}
