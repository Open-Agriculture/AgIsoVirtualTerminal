//================================================================================================
/// @file ShortcutsWindow.hpp
///
/// @brief Defines a dialog where keyboard shortcuts could be selected
/// @author Miklos Marton
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "JuceHeader.h"

class ShortcutsWindow : public juce::AlertWindow
  , public juce::KeyListener
{
public:
	ShortcutsWindow(int alarmAckKeyCode, Component *associatedComponent = nullptr);

	void resized() override;
	bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
	int alarmAckKeyCode() const;

private:
	juce::TextButton selectAlarmAckKeyButton;
	int alarmAckKey = juce::KeyPress::escapeKey;

	bool keySelectionMode = false;

	void updateAlarmAckButtonLabel(const juce::KeyPress &key);

	void setAlarmAckKeySelection(bool isEnabled);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShortcutsWindow)
};
