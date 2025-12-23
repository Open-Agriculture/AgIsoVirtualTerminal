/*******************************************************************************
** @file       InputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputStringComponent.hpp"

#include "StringEncodingConversions.hpp"

InputStringComponent::InputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputString sourceObject) :
  isobus::InputString(sourceObject),
  StringDrawingComponent(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void InputStringComponent::paint(Graphics &g)
{
	paintString(g, displayed_value(parentWorkingSet->get_object_tree()));
}
