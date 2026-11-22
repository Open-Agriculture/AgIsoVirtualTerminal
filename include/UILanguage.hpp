//================================================================================================
/// @file UILanguage.hpp
///
/// @brief Declares the loader for the translations of the VT's own user interface.
/// @author Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef UI_LANGUAGE_HPP
#define UI_LANGUAGE_HPP

#include "JuceHeader.h"

#include <string>

/// @brief Loads one of the translations/*.json files built into the program, for TRANS() to use.
/// @details The files are kept on Weblate. Each maps an English string to its translation.
class UILanguage
{
public:
	/// @brief Shows the UI in the language with this ISO 639 code, or in English if there is no translation for it
	/// @param[in] languageCode The language code, like "hu"
	static void apply(const std::string &languageCode);

private:
	/// @brief Quotes a string the way juce::LocalisedStrings reads it
	static juce::String quote(const juce::String &text);
};

#endif // UI_LANGUAGE_HPP
