/*******************************************************************************
** @file       AckSettingsWindow.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "AckSettingsWindow.hpp"

AckSettingsWindow::AckSettingsWindow(int alarmAckKeyCode, bool showAckButton, Component *associatedComponent) :
  juce::AlertWindow("ACK button", "", MessageBoxIconType::NoIcon, associatedComponent),
  alarmAckKey(alarmAckKeyCode)
{
	juce::KeyPress alarmAckKey(alarmAckKeyCode);
	addCustomComponent(&selectAlarmAckKeyButton);
	selectAlarmAckKeyButton.onClick = [this]() { setAlarmAckKeySelection(true); };
	updateAlarmAckButtonLabel(alarmAckKey);

	showAckButtonCheckbox.setButtonText("Show dedicated ACK button when an alarm mask is active");
	showAckButtonCheckbox.setToggleState(showAckButton, juce::dontSendNotification);
	addCustomComponent(&showAckButtonCheckbox);

	addButton("OK", 5, KeyPress(KeyPress::returnKey, 0, 0));
	addButton("Cancel", 0);

	setAlarmAckKeySelection(false);
}

void AckSettingsWindow::resized()
{
	auto area = getLocalBounds();
	auto customArea = area.removeFromTop(40).removeFromLeft(area.getWidth() / 1.2);
	selectAlarmAckKeyButton.setBounds(customArea);
	showAckButtonCheckbox.setBounds(customArea.translated(0, 40));
}

bool AckSettingsWindow::keyPressed(const KeyPress &key, Component *originatingComponent)
{
	if (keySelectionMode)
	{
		alarmAckKey = key.getKeyCode();
		setAlarmAckKeySelection(false);
		updateAlarmAckButtonLabel(key);
		return true;
	}
	return false;
}

int AckSettingsWindow::alarmAckKeyCode() const
{
	return alarmAckKey;
}

bool AckSettingsWindow::shouldShowAckButton() const
{
	return showAckButtonCheckbox.getToggleState();
}

void AckSettingsWindow::updateAlarmAckButtonLabel(const KeyPress &key)
{
	selectAlarmAckKeyButton.setButtonText("Alarm acknowledge key: " + key.getTextDescription());
}

void AckSettingsWindow::setAlarmAckKeySelection(bool isEnabled)
{
	keySelectionMode = isEnabled;
	selectAlarmAckKeyButton.setEnabled(!isEnabled);
	if (isEnabled)
	{
		selectAlarmAckKeyButton.setButtonText("Alarm acknowledge key: press one key...");

		setWantsKeyboardFocus(true);
		addKeyListener(this);
		grabKeyboardFocus();
	}
	else
	{
		setWantsKeyboardFocus(false);
		removeKeyListener(this);
		if (hasKeyboardFocus(false))
			giveAwayKeyboardFocus();
	}
}
