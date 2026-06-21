//================================================================================================
/// @file AckSettingsWindow.hpp
///
/// @brief Defines a dialog where ACK settings can be configured
/// @author Miklos Marton
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "JuceHeader.h"

class AckSettingsWindow : public juce::AlertWindow
  , public juce::KeyListener
{
public:
	AckSettingsWindow(int alarmAckKeyCode, bool showAckButton, Component *associatedComponent = nullptr);

	void resized() override;
	bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
	int alarmAckKeyCode() const;
	bool shouldShowAckButton() const;

private:
	juce::TextButton selectAlarmAckKeyButton;
	juce::ToggleButton showAckButtonCheckbox;
	int alarmAckKey = juce::KeyPress::escapeKey;

	bool keySelectionMode = false;

	void updateAlarmAckButtonLabel(const juce::KeyPress &key);

	void setAlarmAckKeySelection(bool isEnabled);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AckSettingsWindow)
};
