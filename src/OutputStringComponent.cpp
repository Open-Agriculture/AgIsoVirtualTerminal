/*******************************************************************************
** @file       OutputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputStringComponent.hpp"

OutputStringComponent::OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject) :
  isobus::OutputString(sourceObject),
  StringDrawingComponent(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputStringComponent::paint(Graphics &g)
{
	paintString(g, displayed_value(parentWorkingSet->get_object_tree()));
}
