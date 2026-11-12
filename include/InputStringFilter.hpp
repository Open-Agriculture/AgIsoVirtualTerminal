//================================================================================================
/// @file InputStringFilter.hpp
///
/// @brief Declares the filter for text typed into an Input String.
/// @author Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef INPUT_STRING_FILTER_HPP
#define INPUT_STRING_FILTER_HPP

#include "JuceHeader.h"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"

#include <memory>

/// @brief Limits typed text to an Input String's length and Input Attributes
class InputStringFilter : public juce::TextEditor::InputFilter
{
public:
	/// @brief Constructor
	/// @param[in] maximumLength The most characters the value can hold
	/// @param[in] inputAttributes The object's Input Attributes, or nullptr
	InputStringFilter(int maximumLength, std::shared_ptr<isobus::InputAttributes> inputAttributes);

	/// @brief Returns the part of the new text that may be entered
	/// @param[in] editor The text editor
	/// @param[in] newInput The typed or pasted text
	/// @returns The allowed characters, cut to the room left
	juce::String filterNewText(juce::TextEditor &editor, const juce::String &newInput) override;

	/// @brief Returns if a character may be entered
	/// @param[in] character The character
	/// @returns True if the Input Attributes allow it
	bool is_allowed(juce::juce_wchar character) const;

	/// @brief Returns the limits as text for the user
	/// @returns The maximum length and the allowed or refused characters
	juce::String get_description() const;

private:
	const int maximumLength;
	const bool hasValidation;
	const bool charactersAreValid;
	juce::String validationCharacters;
};

#endif // INPUT_STRING_FILTER_HPP
