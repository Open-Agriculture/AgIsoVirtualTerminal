/*******************************************************************************
** @file       WorkingSetComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetComponent.hpp"

WorkingSetComponent::WorkingSetComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::WorkingSet sourceObject) :
  isobus::WorkingSet(sourceObject),
  parentWorkingSet(workingSet)
{
	setBounds(getLocalBounds());
}

void WorkingSetComponent::paint(Graphics &g)
{
	auto vtColour = colourTable.get_colour(get_background_color());
	auto background = Colour(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	g.setColour(background);
	g.fillAll();
}
