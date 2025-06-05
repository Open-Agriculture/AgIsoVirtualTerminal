/*******************************************************************************
** @file       InputNumberComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputNumberComponent.hpp"

#include <iomanip>
#include <sstream>

InputNumberComponent::InputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputNumber sourceObject) :
  isobus::InputNumber(sourceObject),
  NumberComponent(workingSet)
{
	setSourceObject(this);
	setSize(get_width(), get_height());

	if (get_option(isobus::NumberVTObject::Options::Transparent))
	{
		setOpaque(false);
	}
	else
	{
		setOpaque(true);
	}
}
