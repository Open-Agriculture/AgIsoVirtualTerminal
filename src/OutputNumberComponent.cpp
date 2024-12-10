/*******************************************************************************
** @file       OutputNumberComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputNumberComponent.hpp"

#include <iomanip>
#include <sstream>

OutputNumberComponent::OutputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputNumber sourceObject) :
  isobus::OutputNumber(sourceObject),
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

