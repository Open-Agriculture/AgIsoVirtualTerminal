/*******************************************************************************
** @file       InputBooleanComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputBooleanComponent.hpp"

InputBooleanComponent::InputBooleanComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputBoolean sourceObject) :
  isobus::InputBoolean(sourceObject),
  parentWorkingSet(workingSet)
{
	setOpaque(false);
	setSize(get_width(), get_height());

	setEnabled(get_enabled());
}

void InputBooleanComponent::paint(Graphics &g)
{
	// Draw background
	auto vtColour = parentWorkingSet->get_colour(get_background_color());
	g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	g.drawRect(0, 0, static_cast<int>(get_width()), static_cast<int>(get_height()), 0);

	g.setColour(Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 1.0f));
	// Change colour to foreground colour if present
	if (isobus::NULL_OBJECT_ID != get_foreground_colour_object_id())
	{
		auto child = get_object_by_id(get_foreground_colour_object_id(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type()))
		{
			vtColour = parentWorkingSet->get_colour(std::static_pointer_cast<isobus::FontAttributes>(child)->get_background_color());
			g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
		}
	}

	bool isChecked = (0 != get_value());
	// Change use number variable if one was provided
	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto child = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			isChecked = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
		}
	}

	if (isChecked)
	{
		g.drawLine(0, get_height() / 2, get_width() / 2, get_height());
		g.drawLine(get_width() / 2, get_height(), get_width(), 0);
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!get_enabled())
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}
