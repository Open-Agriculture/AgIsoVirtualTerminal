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
		  if (onCancel)
		  {
			  onCancel();
		  }
		}
	};
	const char *editLabels[3] = { "\xe2\x8c\xab", "C", "\xe2\x9c\x95" };
	const char *editTooltips[3] = { "Backspace", "Clear the entry", "Cancel" };

	for (int row = 0; row < 3; row++)
	{
		for (const char *c = digitRows[row]; '\0' != *c; c++)
		{
			const auto character = static_cast<juce::juce_wchar>(*c);
			add_key(String::charToString(character), [this, character]() { append(character); });
		}

		auto *editKey = add_key(String::fromUTF8(editLabels[row]), editActions[row]);
		editKey->setTooltip(editTooltips[row]);
	}
	keys.getLast()->setColour(TextButton::buttonColourId, Colour(0xffc62828));

	// Bottom row: sign, zero, decimal point and confirm
	auto *signKey = add_key("+/-", [this]() {
		entry = entry.startsWith("-") ? entry.substring(1) : ("-" + entry);
		refresh_display();
	});
	signKey->setTooltip("Change the sign");
	signKey->setEnabled(minimum < 0.0); // Pointless when the object cannot represent a negative value
	add_key("0", [this]() { append('0'); });
	add_key(".", [this]() { append('.'); })->setEnabled(0 != decimals);
	confirmKey = add_key(String::fromUTF8("\xe2\x9c\x93"), [this]() { confirm(); });
	confirmKey->setTooltip("Confirm");
	confirmKey->setColour(TextButton::buttonColourId, Colour(0xff2e7d32));

	setWantsKeyboardFocus(true);
	refresh_display();

	setSize((NUMBER_OF_COLUMNS * KEY_SIZE) + ((NUMBER_OF_COLUMNS + 1) * KEY_GAP),
	        DISPLAY_HEIGHT + (NUMBER_OF_ROWS * KEY_SIZE) + ((NUMBER_OF_ROWS + 1) * KEY_GAP));
}

TextButton *NumericKeypadComponent::add_key(const String &text, std::function<void()> action)
{
	auto *key = new TextButton(text);

	key->onClick = std::move(action);
	key->setWantsKeyboardFocus(false);
	key->setLookAndFeel(&keyLookAndFeel);
	addAndMakeVisible(key);
	keys.add(key);
	return key;
}

void NumericKeypadComponent::append(juce::juce_wchar character)
{
	// The displayed entry is the object's current value until the operator actually types
	// something - the first digit or decimal point overwrites it instead of appending to it,
	// matching what a touch typist expects instead of requiring an explicit Clear press first.
	if (!hasTypedSinceOpening)
	{
		hasTypedSinceOpening = true;
		entry.clear();
	}

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

void NumericKeypadComponent::confirm()
{
	if (has_value() && is_within_range() && onConfirm)
	{
		onConfirm();
	}
}

bool NumericKeypadComponent::has_value() const
{
	return entry.containsAnyOf("0123456789");
}

void NumericKeypadComponent::refresh_display()
{
	// An empty entry stays empty rather than showing 0, which might not be a valid value
	const bool outOfRange = has_value() && !is_within_range();

	entryLabel.setText(entry, dontSendNotification);
	entryLabel.setColour(Label::textColourId,
	                     outOfRange ? Colours::red : getLookAndFeel().findColour(Label::textColourId));
	rangeLabel.setColour(Label::textColourId,
	                     getLookAndFeel().findColour(Label::textColourId).withAlpha(outOfRange ? 1.0f : 0.6f));
	rangeLabel.repaint();

	if (nullptr != confirmKey)
	{
		confirmKey->setEnabled(has_value() && is_within_range());
	}
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

	int index = 0;

	for (int row = 0; row < NUMBER_OF_ROWS; row++)
	{
		const int y = bounds.getY() + KEY_GAP + (row * (KEY_SIZE + KEY_GAP));

		for (int column = 0; column < NUMBER_OF_COLUMNS; column++)
		{
			keys[index]->setBounds(KEY_GAP + (column * (KEY_SIZE + KEY_GAP)), y, KEY_SIZE, KEY_SIZE);
			index++;
		}
	}
}

bool NumericKeypadComponent::keyPressed(const KeyPress &key)
{
	const auto character = key.getTextCharacter();

	if (key.isKeyCode(KeyPress::returnKey))
	{
		confirm();
	}
	else if (key.isKeyCode(KeyPress::escapeKey))
	{
		if (onCancel)
		{
			onCancel();
		}
	}
	else if (key.isKeyCode(KeyPress::backspaceKey))
	{
		backspace();
	}
	else if (CharacterFunctions::isDigit(character) || ('.' == character))
	{
		append(character);
	}
	else
	{
		return false;
	}
	return true;
}

Font NumericKeypadComponent::KeyLookAndFeel::getTextButtonFont(TextButton &, int buttonHeight)
{
	return Font(static_cast<float>(buttonHeight) * 0.42f);
}
