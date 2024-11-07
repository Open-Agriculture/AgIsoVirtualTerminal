/*******************************************************************************
** @file       InputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputStringComponent.hpp"

#include "StringEncodingConversions.hpp"

InputStringComponent::InputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputString sourceObject) :
  isobus::InputString(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void InputStringComponent::paint(Graphics &g)
{
	std::string value = get_value();
	std::uint8_t fontHeight = 0;
	auto fontType = isobus::FontAttributes::FontType::ISO8859_1;

	if (!get_option(Options::Transparent))
	{
		auto vtColour = parentWorkingSet->get_colour(get_background_color());
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	// Get font data
	if (isobus::NULL_OBJECT_ID != get_font_attributes())
	{
		auto child = get_object_by_id(get_font_attributes(), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			if (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
			{
				auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
				fontType = font->get_type();

				auto colour = parentWorkingSet->get_colour(font->get_colour());
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
		}
	}

	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto child = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::StringVariable == child->get_object_type()))
		{
			value = std::static_pointer_cast<isobus::StringVariable>(child)->get_value();
		}
	}

	if (0 == fontHeight)
	{
		fontHeight = 8;
	}

	String decodedValue(value);

	if ((value.length() >= 2) &&
	    (0xFF == static_cast<std::uint8_t>(value.at(0))) &&
	    (0xFE == static_cast<std::uint8_t>(value.at(1))))
	{
		// String is UTF-16 encoded.
		if (0 != (value.length() % 2))
		{
			// If the length attribute does not indicate an even number of bytes the last byte is ignored
			value.pop_back();
		}
		decodedValue = String::createStringFromData(value.c_str(), value.size());
	}
	else
	{
		switch (fontType)
		{
			case isobus::FontAttributes::FontType::ISO8859_1:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_1, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_15:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_15, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_2:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_2, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_4:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_4, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_5:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_5, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_7:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_7, value, utf8String, get_option(Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;
		}
	}

	if (get_option(Options::AutoWrap)) // TODO need to figure out proper font clipping
	{
		g.drawFittedText(decodedValue, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
	else
	{
		g.drawFittedText(decodedValue, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!get_enabled())
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}

Justification InputStringComponent::convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification)
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
