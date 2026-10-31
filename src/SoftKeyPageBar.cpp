/*******************************************************************************
** @file       SoftKeyPageBar.cpp
** @author     Sujan Dumaru, Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "SoftKeyPageBar.hpp"

SoftKeyPageBar::SoftKeyPageBar() :
  previousButton(juce::String::fromUTF8("\xe2\x97\x80")),
  nextButton(juce::String::fromUTF8("\xe2\x96\xb6"))
{
	// Only the buttons take clicks, so a click between them reaches the soft key area as before
	setInterceptsMouseClicks(false, true);
	previousButton.onClick = [this]() { change_page(-1); };
	nextButton.onClick = [this]() { change_page(1); };
	for (auto button : { &previousButton, &nextButton })
	{
		button->setWantsKeyboardFocus(false);
		button->setLookAndFeel(&buttonLookAndFeel);
		button->setColour(juce::TextButton::buttonColourId, juce::Colours::lightgrey);
		button->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
		addAndMakeVisible(*button);
	}
}

juce::Rectangle<int> SoftKeyPageBar::get_area(const SoftKeyMaskDimensions &dimensions)
{
	return { SoftKeyMaskDimensions::PADDING, dimensions.total_height(), dimensions.total_width() - 2 * SoftKeyMaskDimensions::PADDING, HEIGHT };
}

void SoftKeyPageBar::set_page(int currentPage, int pageCount)
{
	page = currentPage;
	pages = pageCount;
	previousButton.setEnabled(0 < page);
	nextButton.setEnabled(page + 1 < pages);
	setVisible(1 < pages);
	resized();
	repaint();
}

void SoftKeyPageBar::set_mask_background(juce::Colour background)
{
	textColour = background.contrasting();
	repaint();
}

void SoftKeyPageBar::paint(Graphics &g)
{
	if (!textArea.isEmpty())
	{
		g.setColour(textColour);
		g.setFont(juce::Font(juce::FontOptions(TEXT_HEIGHT)));
		g.drawText(get_page_text(), textArea, Justification::centred, false);
	}
}

void SoftKeyPageBar::resized()
{
	auto area = getLocalBounds();
	const int textWidth = GlyphArrangement::getStringWidthInt(juce::Font(juce::FontOptions(TEXT_HEIGHT)), get_page_text()) + 2 * GAP;

	// Without room for buttons that can still be hit, the page is left out and the buttons share the bar
	const bool showText = (area.getWidth() - textWidth) / 2 >= MIN_BUTTON_WIDTH;
	const int buttonWidth = (area.getWidth() - (showText ? textWidth : GAP)) / 2;

	previousButton.setBounds(area.removeFromLeft(buttonWidth));
	nextButton.setBounds(area.removeFromRight(buttonWidth));
	textArea = showText ? area : juce::Rectangle<int>();
}

void SoftKeyPageBar::change_page(int delta)
{
	if (onPageChange)
	{
		onPageChange(delta);
	}
}

String SoftKeyPageBar::get_page_text() const
{
	return String(page + 1) + " / " + String(pages);
}

void SoftKeyPageBar::ButtonLookAndFeel::drawButtonBackground(Graphics &g, Button &button, const Colour &backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	// LookAndFeel_V4 fades a disabled face, so the mask shows through
	auto colour = backgroundColour;
	if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
	{
		colour = colour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
	}
	const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
	g.setColour(colour);
	g.fillRoundedRectangle(bounds, 6.0f);
	g.setColour(button.findColour(ComboBox::outlineColourId));
	g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void SoftKeyPageBar::ButtonLookAndFeel::drawButtonText(Graphics &g, TextButton &button, bool, bool)
{
	g.setFont(Font(std::min(0.4f * button.getWidth(), static_cast<float>(button.getHeight()))));
	g.setColour(button.findColour(TextButton::textColourOffId).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.35f));
	g.drawText(button.getButtonText(), button.getLocalBounds(), Justification::centred, false);
}
