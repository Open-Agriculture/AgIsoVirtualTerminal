/*******************************************************************************
** @file       NumberComponent.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "TextDrawingComponent.hpp"

#include "StringEncodingConversions.hpp"

TextDrawingComponent::TextDrawingComponent(
  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) :
  parentWorkingSet(workingSet)
{
}

void TextDrawingComponent::setSourceObject(const isobus::VTObject *newSourceObject)
{
	sourceObject = newSourceObject;
}

Justification TextDrawingComponent::convert_justification(
  isobus::TextualVTObject::HorizontalJustification horizontalJustification,
  isobus::TextualVTObject::VerticalJustification verticalJustification)
{
	Justification retVal = Justification::topLeft;

	switch (horizontalJustification)
	{
		case isobus::TextualVTObject::HorizontalJustification::PositionLeft:
		{
			switch (verticalJustification)
			{
				case isobus::TextualVTObject::VerticalJustification::PositionTop:
				{
					retVal = Justification::topLeft;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centredLeft;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionBottom:
				{
					retVal = Justification::bottomLeft;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::Reserved:
				default:
					break;
			}
		}
		break;

		case isobus::TextualVTObject::HorizontalJustification::PositionMiddle:
		{
			switch (verticalJustification)
			{
				case isobus::TextualVTObject::VerticalJustification::PositionTop:
				{
					retVal = Justification::centredTop;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centred;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionBottom:
				{
					retVal = Justification::centredBottom;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::Reserved:
				default:
					break;
			}
		}
		break;

		case isobus::TextualVTObject::HorizontalJustification::PositionRight:
		{
			switch (verticalJustification)
			{
				case isobus::TextualVTObject::VerticalJustification::PositionTop:
				{
					retVal = Justification::topRight;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionMiddle:
				{
					retVal = Justification::centredRight;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::PositionBottom:
				{
					retVal = Justification::bottomRight;
				}
				break;

				case isobus::TextualVTObject::VerticalJustification::Reserved:
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

std::uint8_t TextDrawingComponent::prepare_text_painting(Graphics &g,
                                                         std::shared_ptr<isobus::FontAttributes> font,
                                                         char referenceCharForWidthCalc)
{
	Font juceFont = Font(Font::getDefaultMonospacedFontName(), 0, 0);
	std::uint8_t fontHeight;
	auto colour = parentWorkingSet->get_colour(font->get_colour());
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

	juceFont.setStyleFlags(fontStyleFlags);
	juceFont.setHeight(font->get_font_height_pixels());

	auto fontWidth = juceFont.getStringWidthFloat(juce::String::fromUTF8(&referenceCharForWidthCalc, 1));
	fontHeight = font->get_font_height_pixels();
	if (fontHeight == 0)
	{
		fontHeight = 8;
	}

	if (!approximatelyEqual(fontWidth, 0.0f))
	{
		juceFont.setHorizontalScale(static_cast<float>(font->get_font_width_pixels()) / fontWidth);
	}

	g.setColour(Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f));
	g.setFont(juceFont);

	return fontHeight;
}

void TextDrawingComponent::drawStrikeThrough(Graphics &g, int w, int h, const String &str, isobus::TextualVTObject::HorizontalJustification justification)
{
	auto font = g.getCurrentFont();
	auto textWidth = font.getStringWidth(str);
	auto lineThickness = h * 0.05f; // set the thickness to 5% of the total text height
	if (lineThickness < 1.0f)
	{
		lineThickness = 1.0f;
	}

	auto y = h / 2;

	switch (justification)
	{
		case isobus::TextualVTObject::HorizontalJustification::PositionLeft:
			g.drawLine(0, y, textWidth, y, lineThickness);
			break;
		case isobus::TextualVTObject::HorizontalJustification::PositionMiddle:
			g.drawLine(w / 2 - textWidth / 2, y, w / 2 + textWidth / 2, y, lineThickness);
			break;
		case isobus::TextualVTObject::HorizontalJustification::PositionRight:
			g.drawLine(w - textWidth, y, w, y, lineThickness);
			break;
		default:
			break;
	}
}

void TextDrawingComponent::paintText(Graphics &g, const std::string &text, bool enabled)
{
	if (isobus::VirtualTerminalObjectType::InputString != sourceObject->get_object_type() &&
	    isobus::VirtualTerminalObjectType::OutputString != sourceObject->get_object_type())
	{
		jassert(false);
		return;
	}
	std::string value = text;
	std::uint8_t fontHeight = 8;
	bool strikeThrough = false;
	auto fontType = isobus::FontAttributes::FontType::ISO8859_1;
	auto sourceString = static_cast<const isobus::StringVTObject *>(sourceObject);

	if (!sourceString->get_option(isobus::StringVTObject::Options::Transparent))
	{
		auto vtColour = parentWorkingSet->get_colour(sourceString->get_background_color());
		g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	}

	g.setColour(getLookAndFeel().findColour(ListBox::textColourId));

	// Get font data
	auto fontAttrID = sourceString->get_font_attributes();

	if (isobus::NULL_OBJECT_ID != fontAttrID)
	{
		auto child = sourceString->get_object_by_id(fontAttrID, parentWorkingSet->get_object_tree());

		if (child != nullptr && isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			fontHeight = prepare_text_painting(g, font, 'a');
			strikeThrough = font->get_style(isobus::FontAttributes::FontStyleBits::CrossedOut);
		}
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
				convert_string_to_utf_8(SourceEncoding::ISO8859_1, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_15:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_15, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_2:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_2, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_4:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_4, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_5:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_5, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;

			case isobus::FontAttributes::FontType::ISO8859_7:
			{
				std::string utf8String;
				convert_string_to_utf_8(SourceEncoding::ISO8859_7, value, utf8String, sourceString->get_option(isobus::StringVTObject::Options::AutoWrap));
				decodedValue = utf8String;
			}
			break;
		}
	}

	if (sourceString->get_option(isobus::StringVTObject::Options::AutoWrap)) // TODO need to figure out proper font clipping
	{
		g.drawFittedText(decodedValue, 0, 0, sourceString->get_width(), sourceString->get_height(), convert_justification(sourceString->get_horizontal_justification(), sourceString->get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(sourceString->get_height()) + 0.1f) / fontHeight)), 0.8f);
	}
	else
	{
		g.drawFittedText(decodedValue, 0, 0, sourceString->get_width(), sourceString->get_height(), convert_justification(sourceString->get_horizontal_justification(), sourceString->get_vertical_justification()), static_cast<int>(std::floor((static_cast<float>(sourceString->get_height()) + 0.1f) / fontHeight)), 0.8f);
	}

	if (strikeThrough)
	{
		// Juce does not support Strikethrough text drawing, draw the line manually
		drawStrikeThrough(g, sourceString->get_width(), sourceString->get_height(), decodedValue, sourceString->get_horizontal_justification());
	}

	// If disabled, try and show that by drawing some semi-transparent grey
	if (!enabled)
	{
		g.fillAll(Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.5f));
	}
}
