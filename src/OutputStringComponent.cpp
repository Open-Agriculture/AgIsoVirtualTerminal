/*******************************************************************************
** @file       OutputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputStringComponent.hpp"

OutputStringComponent::OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject) :
  isobus::OutputString(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputStringComponent::paint(Graphics &g)
{
	std::string value = get_value();
	std::uint8_t fontHeight = 0;

	if (!get_option(Options::Transparent))
	{
		auto vtColour = colourTable.get_colour(get_background_color());
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

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

				auto colour = colourTable.get_colour(font->get_colour());
				Font juceFont;
				int fontStyleFlags = Font::FontStyleFlags::plain;

				if (font->get_style(isobus::FontAttributes::FontStyleBits::Bold))
				{
					fontStyleFlags |= Font::FontStyleFlags::bold;
				}

				if (font->get_style(isobus::FontAttributes::FontStyleBits::Italic))
				{
					fontStyleFlags |= Font::FontStyleFlags::italic;
				}

				if (font->get_style(isobus::FontAttributes::FontStyleBits::Underlined))
				{
					fontStyleFlags |= Font::FontStyleFlags::underlined;
				}

				juceFont = Font(Font::getDefaultMonospacedFontName(), font->get_font_height_pixels(), fontStyleFlags);

				auto fontWidth = juceFont.getStringWidthFloat("a");
				fontHeight = font->get_font_width_pixels();
				juceFont.setHorizontalScale(static_cast<float>(font->get_font_width_pixels()) / fontWidth);
				g.setColour(Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f));
				g.setFont(juceFont);
			}
			else if (isobus::VirtualTerminalObjectType::StringVariable == child->get_object_type())
			{
				value = std::static_pointer_cast<isobus::StringVariable>(child)->get_value();
			}
		}
	}

	if (0 == fontHeight)
	{
		fontHeight = 8;
	}

	if (get_option(Options::AutoWrap)) // TODO need to figure out proper font clipping
	{
		g.drawFittedText(value, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
	else
	{
		g.drawFittedText(value, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
}

Justification OutputStringComponent::convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification)
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

				case VerticalJustification::Reserved:
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

				case VerticalJustification::Reserved:
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

				case VerticalJustification::Reserved:
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
