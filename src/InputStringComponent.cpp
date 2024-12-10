/*******************************************************************************
** @file       InputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputStringComponent.hpp"

#include "StringEncodingConversions.hpp"

InputStringComponent::InputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputString sourceObject) :
  isobus::InputString(sourceObject),
  TextDrawingComponent(workingSet)
{
	setSourceObject(this);
	setSize(get_width(), get_height());
	setOpaque(false);
}

void InputStringComponent::paint(Graphics &g)
{
	std::string value = get_value();
	paintText(g, get_value(), get_enabled());
}
