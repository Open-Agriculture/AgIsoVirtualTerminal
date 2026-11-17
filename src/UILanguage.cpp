/*******************************************************************************
** @file       UILanguage.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "UILanguage.hpp"

#include "TranslationFiles.h"

void UILanguage::apply(const std::string &languageCode)
{
	const auto resourceName = juce::String(languageCode).toLowerCase() + "_json";
	int size = 0;
	const auto *data = TranslationFiles::getNamedResource(resourceName.toRawUTF8(), size);
	juce::String mappings;

	if (nullptr != data)
	{
		const auto parsed = juce::JSON::parse(juce::String::fromUTF8(data, size));

		if (auto *object = parsed.getDynamicObject())
		{
			for (const auto &property : object->getProperties())
			{
				mappings << quote(property.name.toString()) << " = " << quote(property.value.toString()) << juce::newLine;
			}
		}
	}

	// JUCE takes ownership, and no mappings means English
	juce::LocalisedStrings::setCurrentMappings(mappings.isEmpty() ? nullptr : new juce::LocalisedStrings(mappings, false));
}

juce::String UILanguage::quote(const juce::String &text)
{
	return "\"" + text.replace("\"", "\\\"").replace("\n", "\\n") + "\"";
}
