/*******************************************************************************
** @file       NumberComponent.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "NumberComponent.hpp"

#include <iomanip>
#include <sstream>

NumberComponent::NumberComponent(
  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) :
  parentWorkingSet(workingSet),
  TextDrawingComponent(workingSet)
{
}

void NumberComponent::paint(Graphics &g)
{
	if (!sourceObject)
	{
		return;
	}

	auto sourceNumber = static_cast<isobus::NumberVTObject *>(sourceObject);

	if (isOpaque())
	{
		auto vtColour = parentWorkingSet->get_colour(sourceNumber->get_background_color());
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	bool strikeThrough = false;
	float scaledValue = (sourceNumber->get_value() + sourceNumber->get_offset()) * sourceNumber->get_scale();
	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	// Get font data
	if (isobus::NULL_OBJECT_ID != sourceNumber->get_font_attributes())
	{
		auto child = sourceNumber->get_object_by_id(sourceNumber->get_font_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) &&
		    (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type()))
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
			prepare_text_painting(g, font, '1');
		}
	}

	if (isobus::NULL_OBJECT_ID != sourceNumber->get_variable_reference())
	{
		auto child = sourceNumber->get_object_by_id(sourceNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) &&
		    (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			scaledValue = (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() + sourceNumber->get_offset()) * sourceNumber->get_scale();
		}
	}

	std::ostringstream valueText;
	valueText << std::fixed << std::setprecision(sourceNumber->get_number_of_decimals()) << scaledValue;
	g.drawText(valueText.str(), 0, 0, sourceNumber->get_width(), sourceNumber->get_height(), convert_justification(sourceNumber->get_horizontal_justification(), sourceNumber->get_vertical_justification()), false);

	if (strikeThrough)
	{
		drawStrikeThrough(g, sourceNumber->get_width(), sourceNumber->get_height(), valueText.str(), sourceNumber->get_horizontal_justification());
	}
}
