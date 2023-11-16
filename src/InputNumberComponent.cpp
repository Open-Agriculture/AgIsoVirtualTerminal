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
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());

	if (get_option(Options::Transparent))
	{
		setOpaque(false);
	}
	else
	{
		setOpaque(true);
	}
}

void InputNumberComponent::paint(Graphics &g)
{
	if (isOpaque())
	{
		auto vtColour = colourTable.get_colour(backgroundColor);
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	float scaledValue = (get_value() + get_offset()) * get_scale();
	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	// Get font data
	for (std::uint16_t i = 0; i < get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i));

		if (nullptr != child)
		{
			if (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
			{
				auto font = std::static_pointer_cast<isobus::FontAttributes>(child);

				auto colour = colourTable.get_colour(font->get_background_color());
				Font juceFont(Font::getDefaultMonospacedFontName(), font->get_font_height_pixels(), Font::FontStyleFlags::plain);
				auto fontWidth = juceFont.getStringWidthFloat("1");
				juceFont.setHorizontalScale(static_cast<float>(font->get_font_width_pixels()) / fontWidth);
				g.setColour(Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f));
				g.setFont(juceFont);
			}
			else if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
			{
				scaledValue = (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() + get_offset()) * get_scale();
			}
		}
	}

	std::ostringstream valueText;
	valueText << std::fixed << std::setprecision(get_number_of_decimals()) << scaledValue;
	g.drawText(valueText.str(), 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), false);
}

Justification InputNumberComponent::convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification)
{
	Justification retVal = Justification::topLeft;

	switch (horizontalJustification)
	{
		case HorizontalJustification::PositionLeft:
		{
			switch (verticalJustification)
			{
				case VerticalJustification::PositionTop:
				{
					retVal = Justification::topLeft;
				}
				break;

				case VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centredLeft;
				}
				break;

				case VerticalJustification::PositionBottom:
				{
					retVal = Justification::bottomLeft;
				}
				break;

				default:
					break;
			}
		}
		break;

		case HorizontalJustification::PositionMiddle:
		{
			switch (verticalJustification)
			{
				case VerticalJustification::PositionTop:
				{
					retVal = Justification::centredTop;
				}
				break;

				case VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centred;
				}
				break;

				case VerticalJustification::PositionBottom:
				{
					retVal = Justification::centredBottom;
				}
				break;

				default:
					break;
			}
		}
		break;

		case HorizontalJustification::PositionRight:
		{
			switch (verticalJustification)
			{
				case VerticalJustification::PositionTop:
				{
					retVal = Justification::topRight;
				}
				break;

				case VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centredRight;
				}
				break;

				case VerticalJustification::PositionBottom:
				{
					retVal = Justification::bottomRight;
				}
				break;

				default:
					break;
			}
		}
		break;

		default:
		{
		}
		break;
	}
	return retVal;
}
