/*******************************************************************************
** @file       InputNumberComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputNumberComponent.hpp"

InputNumberComponent::InputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputNumber sourceObject) :
  isobus::InputNumber(sourceObject),
  NumberComponent(workingSet)
{
	setSize(get_width(), get_height());
}

void InputNumberComponent::paint(Graphics &g)
{
	paintNumber(g, get_option2(isobus::InputNumber::Options2::Enabled));
}
