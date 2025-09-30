/*******************************************************************************
** @file       OutputStringComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputStringComponent.hpp"

#include "StringEncodingConversions.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

OutputStringComponent::OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject) :
  isobus::OutputString(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputStringComponent::paint(Graphics &g)
{
	std::string value = displayed_value(parentWorkingSet);

	std::size_t pos = value.find('\0');
	if (pos != std::string::npos)
	{
		value = value.substr(0, pos);
	}

	std::uint8_t fontHeight = 0;
	auto fontType = isobus::FontAttributes::FontType::ISO8859_1;

	auto vtColour = parentWorkingSet->get_colour(get_background_color());
	juce::Colour backgroundColor = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	juce::Colour drawColor = getLookAndFeel().findColour(ListBox::textColourId);

	// Get font data
	auto fontAttrID = get_font_attributes();

	if (isobus::NULL_OBJECT_ID != fontAttrID)
	{
		auto child = get_object_by_id(fontAttrID, parentWorkingSet->get_object_tree());

		if (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);

			if (font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden) && !show && !font->get_style(isobus::FontAttributes::FontStyleBits::Flashing)) // Bit 6 (Flashing) has priority over bit 5 (FlashingHidden)
			{
				return;
			}

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
			drawColor = Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f);
			g.setFont(juceFont);

			// swap background and draw colors for inverted draw style
			if (font->get_style(isobus::FontAttributes::FontStyleBits::Inverted) || (font->get_style(isobus::FontAttributes::FontStyleBits::Flashing) && !show))
			{
				auto tmpColor = backgroundColor;
				backgroundColor = drawColor;
				drawColor = tmpColor;
			}
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
		// String is UTF-16 encoded, font type is ignored.
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

	if (!get_option(Options::Transparent))
	{
		g.fillAll(backgroundColor);
	}
	g.setColour(drawColor);

	if (get_option(Options::AutoWrap)) // TODO need to figure out proper font clipping
	{
		g.drawFittedText(decodedValue, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
	else
	{
		g.drawFittedText(decodedValue, 0, 0, get_width(), get_height(), convert_justification(get_horizontal_justification(), get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(get_height()) + 0.1f) / fontHeight)), 0.8f);
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

void OutputStringComponent::visibilityChanged()
{
	if (visible != isVisible())
	{
		visible = isVisible();

		if (visible && isFlashing())
		{
			startTimer(500);
		}
		else
		{
			stopTimer();
		}
	}
}

void OutputStringComponent::timerCallback()
{
	show = !show;
	repaint();
}

bool OutputStringComponent::isFlashing() const
{
	auto fontAttrID = get_font_attributes();
	if (isobus::NULL_OBJECT_ID != fontAttrID)
	{
		auto child = get_object_by_id(fontAttrID, parentWorkingSet->get_object_tree());

		if (isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			return font->get_style(isobus::FontAttributes::FontStyleBits::Flashing) || font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden);
		}
	}
	return false;
}
