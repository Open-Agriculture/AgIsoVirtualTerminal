/*******************************************************************************
** @file       ConfigureHardwareWindow.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/

#include "ConfigureHardwareWindow.hpp"

ConfigureHardwareWindow::ConfigureHardwareWindow(ServerMainComponent &parentComponent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  DocumentWindow("Configure Hardware", juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), DocumentWindow::closeButton),
  parentServer(parentComponent),
  content(*this, canDrivers)
{
	setOpaque(true);
	setSize(400, 280);
	content.setSize(400, 280);
	setContentNonOwned(&content, false);
}

void ConfigureHardwareWindow::closeButtonPressed()
{
	setVisible(false);
}
