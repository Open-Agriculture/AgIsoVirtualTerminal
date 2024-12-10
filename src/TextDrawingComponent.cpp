/*******************************************************************************
** @file       TextDrawingComponent.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "TextDrawingComponent.hpp"

TextDrawingComponent::TextDrawingComponent(
  std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) :
  parentWorkingSet(workingSet)
{
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
                                                         char referenceCharForWidthCalc,
                                                         juce::Colour &drawColour,
                                                         juce::Colour &backgroundColor)
{
	Font juceFont = Font(Font::getDefaultMonospacedFontName(), 0, 0);
	std::uint8_t fontHeight;
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

	// swap background and draw colors for inverted draw style

	if (font->get_style(isobus::FontAttributes::FontStyleBits::Inverted) || (font->get_style(isobus::FontAttributes::FontStyleBits::Flashing) && !show))
	{
		auto tmpColor = backgroundColor;
		backgroundColor = drawColour;
		drawColour = tmpColor;
	}

	g.setColour(drawColour);
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

void TextDrawingComponent::visibilityChanged()
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

void TextDrawingComponent::timerCallback()
{
	show = !show;
	repaint();
}

bool TextDrawingComponent::isFlashing() const
{
	auto fontAttrID = static_cast<const isobus::TextualVTObject *>(vtObject())->get_font_attributes();
	if (isobus::NULL_OBJECT_ID != fontAttrID)
	{
		auto child = parentWorkingSet->get_object_by_id(fontAttrID);

		if (nullptr != child && isobus::VirtualTerminalObjectType::FontAttributes == child->get_object_type())
		{
			auto font = std::static_pointer_cast<isobus::FontAttributes>(child);
			return font->get_style(isobus::FontAttributes::FontStyleBits::Flashing) || font->get_style(isobus::FontAttributes::FontStyleBits::FlashingHidden);
		}
	}
	return false;
}
