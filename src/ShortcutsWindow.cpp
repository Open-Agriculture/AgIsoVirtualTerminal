/*******************************************************************************
** @file       ShortcutsWindow.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ShortcutsWindow.hpp"

ShortcutsWindow::ShortcutsWindow(int currentKeyCode, Component *associatedComponent) :
  juce::AlertWindow("Keyboard shortcuts", "", MessageBoxIconType::NoIcon, associatedComponent),
  selectedKeyCode_(currentKeyCode)
{
	juce::KeyPress key(currentKeyCode);
	addCustomComponent(&selectKeyButton);
	selectKeyButton.onClick = [this]() { setKeySelectionMode(true); };
	updateAlarmAckButtonLabel(key);

	addButton("OK", 5, KeyPress(KeyPress::returnKey, 0, 0));
	addButton("Cancel", 0);

	setKeySelectionMode(false);
}

void ShortcutsWindow::resized()
{
	auto area = getLocalBounds();
	auto customArea = area.removeFromTop(40).removeFromLeft(area.getWidth() / 1.2);
	selectKeyButton.setBounds(customArea);
}

bool ShortcutsWindow::keyPressed(const KeyPress &key, Component *originatingComponent)
{
	if (keySelectionMode)
	{
		selectedKeyCode_ = key.getKeyCode();
		setKeySelectionMode(false);
		updateAlarmAckButtonLabel(key);
		return true;
	}
	return false;
}

int ShortcutsWindow::selectedKeyCode() const
{
	return selectedKeyCode_;
}

void ShortcutsWindow::updateAlarmAckButtonLabel(const KeyPress &key)
{
	selectKeyButton.setButtonText("Alarm acknowledge key: " + key.getTextDescription());
}

void ShortcutsWindow::setKeySelectionMode(bool isEnabled)
{
	keySelectionMode = isEnabled;
	selectKeyButton.setEnabled(!isEnabled);
	if (isEnabled)
	{
		selectKeyButton.setButtonText("Alarm acknowledge key: press one key...");

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
