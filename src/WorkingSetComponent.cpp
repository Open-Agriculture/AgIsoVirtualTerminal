/*******************************************************************************
** @file       WorkingSetComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

WorkingSetComponent::WorkingSetComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::WorkingSet sourceObject, int keyHeight, int keyWidth) :
  isobus::WorkingSet(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(keyWidth, keyHeight);
	setOpaque(false);

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

void WorkingSetComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(get_background_color());
	auto background = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	g.setColour(background);
	g.fillAll();
}
