/*******************************************************************************
** @file       InputStringFilter.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "InputStringFilter.hpp"

#include <algorithm>

InputStringFilter::InputStringFilter(int maximumLength, std::shared_ptr<isobus::InputAttributes> inputAttributes) :
  maximumLength(maximumLength),
  hasValidation(nullptr != inputAttributes),
  charactersAreValid(hasValidation && (isobus::InputAttributes::ValidationType::ValidCharactersAreListed == inputAttributes->get_validation_type()))
{
	if (hasValidation)
	{
		// An 8-bit string, so each byte is the Latin-1 character with that code
		for (const auto byte : inputAttributes->get_validation_string())
		{
			validationCharacters += static_cast<juce::juce_wchar>(static_cast<std::uint8_t>(byte));
		}
	}
}

juce::String InputStringFilter::filterNewText(juce::TextEditor &editor, const juce::String &newInput)
{
	juce::String allowed;

	for (const auto character : newInput)
	{
		if (is_allowed(character))
		{
			allowed += character;
		}
	}

	const int room = maximumLength - (editor.getTotalNumChars() - editor.getHighlightedRegion().getLength());
	return allowed.substring(0, std::max(0, room));
}

bool InputStringFilter::is_allowed(juce::juce_wchar character) const
{
	return !hasValidation || (validationCharacters.containsChar(character) == charactersAreValid);
}
