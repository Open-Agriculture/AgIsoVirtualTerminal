/*******************************************************************************
** @file       OutputNumberComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputNumberComponent.hpp"

OutputNumberComponent::OutputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputNumber sourceObject) :
  isobus::OutputNumber(sourceObject),
  NumberComponent(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputNumberComponent::paint(Graphics &g)
{
	paintNumber(g);
}
