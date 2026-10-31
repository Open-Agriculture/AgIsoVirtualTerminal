//================================================================================================
/// @file SoftKeyPageBar.hpp
///
/// @brief Declares the bar below the soft keys that pages through a soft key mask with more keys
/// than the VT has positions for.
/// @author Sujan Dumaru, Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef SOFT_KEY_PAGE_BAR_HPP
#define SOFT_KEY_PAGE_BAR_HPP

#include "SoftKeyMaskComponent.hpp"

#include "JuceHeader.h"

#include <functional>

/// @brief A previous and a next page button, with the current page and the page count between them
class SoftKeyPageBar : public Component
{
public:
	SoftKeyPageBar();

	/// @brief Where the bar goes in the soft key area: below the key positions, inside the padding
	/// @param[in] dimensions The soft key layout of the VT
	/// @returns The bounds of the bar in the soft key area
	static juce::Rectangle<int> get_area(const SoftKeyMaskDimensions &dimensions);

	/// @brief Shows a page and enables the buttons that lead somewhere. With fewer than two pages
	/// the bar is hidden
	/// @param[in] currentPage The page shown, counted from 0
	/// @param[in] pageCount How many pages the soft key mask has
	void set_page(int currentPage, int pageCount);

	/// @brief The bar lies on the soft key mask, so the page is written in a colour that stands out
	/// from the mask's background
	/// @param[in] background The background colour of the soft key mask
	void set_mask_background(juce::Colour background);

	/// @brief Called with -1 or 1 when a button is clicked
	std::function<void(int)> onPageChange;

	void paint(Graphics &g) override;

	void resized() override;

	static constexpr int HEIGHT = 40;

private:
	static constexpr int GAP = 4;
	static constexpr int MIN_BUTTON_WIDTH = HEIGHT / 2;
	static constexpr float TEXT_HEIGHT = 16.0f;

	class ButtonLookAndFeel : public LookAndFeel_V4
	{
	public:
		void drawButtonBackground(Graphics &g, Button &button, const Colour &backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
		void drawButtonText(Graphics &g, TextButton &button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
	};

	void change_page(int delta);
	String get_page_text() const;

	ButtonLookAndFeel buttonLookAndFeel; // Declared before the buttons, so it outlives them
	juce::TextButton previousButton;
	juce::TextButton nextButton;
	juce::Rectangle<int> textArea;
	juce::Colour textColour = juce::Colours::black;
	int page = 0;
	int pages = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftKeyPageBar)
};

#endif // SOFT_KEY_PAGE_BAR_HPP
