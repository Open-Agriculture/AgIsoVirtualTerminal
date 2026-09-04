/*******************************************************************************
** @file       NumericKeypadComponent.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "NumericKeypadComponent.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

NumericKeypadComponent::NumericKeypadComponent(double initialValue,
                                               double minimumValue,
                                               double maximumValue,
                                               std::uint8_t numberOfDecimals) :
  minimum(std::min(minimumValue, maximumValue)),
  maximum(std::max(minimumValue, maximumValue)),
  decimals(numberOfDecimals)
{
	entry = String(initialValue, static_cast<int>(decimals));

	entryLabel.setJustificationType(Justification::centredRight);
	entryLabel.setFont(Font(34.0f, Font::bold));
	addAndMakeVisible(entryLabel);

	rangeLabel.setJustificationType(Justification::centredRight);
	rangeLabel.setFont(Font(13.0f));
	rangeLabel.setText("Range " + String(minimum, static_cast<int>(decimals)) +
	                     " to " + String(maximum, static_cast<int>(decimals)),
	                   dontSendNotification);
	addAndMakeVisible(rangeLabel);

	// Built row by row exactly as they are laid out: three digits then an editing key.
	// The digits are ordered like a telephone keypad, which is what an operator expects.
	const char *digitRows[3] = { "789", "456", "123" };
	std::function<void()> editActions[3] = {
		[this]() { backspace(); },
		[this]() { clear(); },
		[this]() {
		  entry = entry.startsWith("-") ? entry.substring(1) : ("-" + entry);
		  refresh_display();
		}
	};
	const char *editLabels[3] = { "\xe2\x8c\xab", "C", "\xc2\xb1" };
	const char *editTooltips[3] = { "Backspace", "Clear the entry", "Change the sign" };

	for (int row = 0; row < 3; row++)
	{
		for (const char *c = digitRows[row]; '\0' != *c; c++)
		{
			const auto character = static_cast<juce::juce_wchar>(*c);
			add_key(String::charToString(character), [this, character]() { append(character); });
		}

		auto *editKey = add_key(String::fromUTF8(editLabels[row]), editActions[row]);
		editKey->setTooltip(editTooltips[row]);

		// A sign key is pointless when the object cannot represent a negative value
		if (2 == row)
		{
			editKey->setEnabled(minimum < 0.0);
		}
	}

	// Bottom row: a double width zero under the digits, then the decimal point
	add_key("0", [this]() { append('0'); });
	add_key(".", [this]() { append('.'); })->setEnabled(0 != decimals);

	refresh_display();

	setSize((NUMBER_OF_COLUMNS * KEY_SIZE) + ((NUMBER_OF_COLUMNS + 1) * KEY_GAP),
	        DISPLAY_HEIGHT + (NUMBER_OF_ROWS * KEY_SIZE) + ((NUMBER_OF_ROWS + 1) * KEY_GAP));
}

TextButton *NumericKeypadComponent::add_key(const String &text, std::function<void()> action)
{
	auto *key = new TextButton(text);

	key->onClick = std::move(action);
	key->setWantsKeyboardFocus(false);
	addAndMakeVisible(key);
	keys.add(key);
	return key;
}

void NumericKeypadComponent::append(juce::juce_wchar character)
{
	if ('.' == character)
	{
		// Only one decimal point, and only when the object actually displays decimals
		if ((0 == decimals) || entry.containsChar('.'))
		{
			return;
		}
	}
	else if (0 != decimals)
	{
		// Refuse digits which would be dropped when the value is displayed anyway
		const int pointIndex = entry.indexOfChar('.');

		if ((pointIndex >= 0) && ((entry.length() - pointIndex - 1) >= static_cast<int>(decimals)))
		{
			return;
		}
	}

	// A lone leading zero is replaced rather than appended to
	if (entry.equalsIgnoreCase("0") && ('.' != character))
	{
		entry = String::charToString(character);
	}
	else if (entry.equalsIgnoreCase("-0") && ('.' != character))
	{
		entry = "-" + String::charToString(character);
	}
	else
	{
		entry += String::charToString(character);
	}
	refresh_display();
}

void NumericKeypadComponent::backspace()
{
	if (entry.isNotEmpty())
	{
		entry = entry.dropLastCharacters(1);
	}
	refresh_display();
}

void NumericKeypadComponent::clear()
{
	entry.clear();
	refresh_display();
}

void NumericKeypadComponent::refresh_display()
{
	entryLabel.setText(entry.isEmpty() ? "0" : entry, dontSendNotification);

	// Out of range entries are still accepted and clamped when the dialog is confirmed, but the
	// operator gets told before that happens
	entryLabel.setColour(Label::textColourId,
	                     is_within_range() ? getLookAndFeel().findColour(Label::textColourId) : Colours::red);
	rangeLabel.setColour(Label::textColourId,
	                     getLookAndFeel().findColour(Label::textColourId).withAlpha(is_within_range() ? 0.6f : 1.0f));
	rangeLabel.repaint();
}

double NumericKeypadComponent::get_value() const
{
	const double typed = entry.isEmpty() ? 0.0 : entry.getDoubleValue();

	return std::min(std::max(typed, minimum), maximum);
}

bool NumericKeypadComponent::is_within_range() const
{
	const double typed = entry.isEmpty() ? 0.0 : entry.getDoubleValue();

	return (typed >= minimum) && (typed <= maximum);
}

void NumericKeypadComponent::paint(Graphics &graphics)
{
	auto displayArea = getLocalBounds().removeFromTop(DISPLAY_HEIGHT).reduced(KEY_GAP, KEY_GAP);

	graphics.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId).darker(0.4f));
	graphics.fillRoundedRectangle(displayArea.toFloat(), 4.0f);
}

void NumericKeypadComponent::resized()
{
	auto bounds = getLocalBounds();
	auto displayArea = bounds.removeFromTop(DISPLAY_HEIGHT).reduced(KEY_GAP * 2, KEY_GAP);

	rangeLabel.setBounds(displayArea.removeFromBottom(18));
	entryLabel.setBounds(displayArea);

	// Rows of three digits plus an editing key
	int index = 0;

	for (int row = 0; row < NUMBER_OF_ROWS - 1; row++)
	{
		const int y = bounds.getY() + KEY_GAP + (row * (KEY_SIZE + KEY_GAP));

		for (int column = 0; column < NUMBER_OF_COLUMNS; column++)
		{
			keys[index]->setBounds(KEY_GAP + (column * (KEY_SIZE + KEY_GAP)), y, KEY_SIZE, KEY_SIZE);
			index++;
		}
	}

	// Bottom row: zero spans the first two columns, the decimal point sits in the third
	const int lastRowY = bounds.getY() + KEY_GAP + ((NUMBER_OF_ROWS - 1) * (KEY_SIZE + KEY_GAP));

	keys[index]->setBounds(KEY_GAP, lastRowY, (2 * KEY_SIZE) + KEY_GAP, KEY_SIZE);
	index++;
	keys[index]->setBounds(KEY_GAP + (2 * (KEY_SIZE + KEY_GAP)), lastRowY, KEY_SIZE, KEY_SIZE);
}
