/*******************************************************************************
** @file       ShortcutsWindow.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ShortcutsWindow.hpp"

ShortcutsWindow::ShortcutsWindow(int alarmAckKeyCode, Component *associatedComponent) :
  juce::AlertWindow("Keyboard shortcuts", "", MessageBoxIconType::NoIcon, associatedComponent),
  alarmAckKey(alarmAckKeyCode)
{
	juce::KeyPress alarmAckKey(alarmAckKeyCode);
	addCustomComponent(&selectAlarmAckKeyButton);
	selectAlarmAckKeyButton.onClick = [this]() { setAlarmAckKeySelection(true); };
	updateAlarmAckButtonLabel(alarmAckKey);

	addButton("OK", 5, KeyPress(KeyPress::returnKey, 0, 0));
	addButton("Cancel", 0);

	setAlarmAckKeySelection(false);
}

void ShortcutsWindow::resized()
{
	auto area = getLocalBounds();
	auto customArea = area.removeFromTop(40).removeFromLeft(area.getWidth() / 1.2);
	selectAlarmAckKeyButton.setBounds(customArea);
}

bool ShortcutsWindow::keyPressed(const KeyPress &key, Component *originatingComponent)
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

int ShortcutsWindow::alarmAckKeyCode() const
{
	return alarmAckKey;
}

void ShortcutsWindow::updateAlarmAckButtonLabel(const KeyPress &key)
{
	selectAlarmAckKeyButton.setButtonText("Alarm acknowledge key: " + key.getTextDescription());
}

void ShortcutsWindow::setAlarmAckKeySelection(bool isEnabled)
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
