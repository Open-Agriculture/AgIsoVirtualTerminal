//================================================================================================
/// @file NumericKeypadComponent.hpp
///
/// @brief Defines a touch friendly numeric keypad used to edit input number objects.
///
/// @details A slider cannot hit an exact value with a finger, especially over the wide ranges an
/// input number can have, so values are typed on this keypad instead. The keys are sized for
/// touch rather than for a mouse.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef NUMERIC_KEYPAD_COMPONENT_HPP
#define NUMERIC_KEYPAD_COMPONENT_HPP

#include "JuceHeader.h"

#include <cstdint>
#include <memory>
#include <vector>

class NumericKeypadComponent : public Component
{
public:
	/// @brief Constructor
	/// @param[in] initialValue The value the input number currently has, in displayed units
	/// @param[in] minimumValue The lowest value the object accepts, in displayed units
	/// @param[in] maximumValue The highest value the object accepts, in displayed units
	/// @param[in] numberOfDecimals How many decimals the object displays. Zero means the decimal
	/// point key is disabled, because a fractional value could not be represented.
	NumericKeypadComponent(double initialValue,
	                       double minimumValue,
	                       double maximumValue,
	                       std::uint8_t numberOfDecimals);

	/// @brief Returns what has been typed, clamped into the object's range
	/// @returns The entered value in displayed units
	double get_value() const;

	/// @brief Checks if what has been typed is inside the object's range. Out of range entries are
	/// still accepted and clamped, but the display shows them in red beforehand.
	/// @returns True if the typed value needs no clamping
	bool is_within_range() const;

	void paint(Graphics &graphics) override;

	void resized() override;

private:
	/// @brief Appends a character to what has been typed, if it makes sense to do so
	/// @param[in] character The character the operator pressed
	void append(juce::juce_wchar character);

	/// @brief Removes the last typed character
	void backspace();

	/// @brief Clears the entry back to empty
	void clear();

	/// @brief Redraws the entry and recolours it when the value is out of range
	void refresh_display();

	/// @brief Creates one key and adds it to the keypad
	/// @param[in] text The label of the key
	/// @param[in] action What the key does when it is pressed
	/// @returns A pointer to the created button
	TextButton *add_key(const String &text, std::function<void()> action);

	static constexpr int KEY_SIZE = 76; ///< Key size in pixels, comfortably above a fingertip
	static constexpr int KEY_GAP = 6; ///< Space between the keys
	static constexpr int DISPLAY_HEIGHT = 84; ///< Height of the entry and range readout
	static constexpr int NUMBER_OF_COLUMNS = 4; ///< Digits take three columns, edit keys the fourth
	static constexpr int NUMBER_OF_ROWS = 4; ///< Three digit rows plus the bottom row

	OwnedArray<TextButton> keys;
	Label entryLabel;
	Label rangeLabel;
	String entry;
	double minimum;
	double maximum;
	std::uint8_t decimals;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumericKeypadComponent)
};

#endif // NUMERIC_KEYPAD_COMPONENT_HPP
