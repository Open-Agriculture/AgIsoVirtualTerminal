//================================================================================================
/// @file DiagnosticsWindow.cpp
///
/// @brief Implements a separate window that shows active diagnostic trouble codes (DM1)
/// reported by control functions on the bus.
/// @author Sujan Dumaru
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#include "DiagnosticsWindow.hpp"

DiagnosticsWindow::DiagnosticsWindow(ServerMainComponent &parentComponent) :
  DocumentWindow("Diagnostics", juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), DocumentWindow::closeButton),
  parentServer(parentComponent)
{
	setOpaque(true);
	setResizable(true, true);
	setSize(DiagnosticsComponent::WIDTH + 40, 400);
	content.setSize(DiagnosticsComponent::WIDTH, 400);
	viewport.setViewedComponent(&content, false);
	setContentNonOwned(&viewport, false);
}

void DiagnosticsWindow::closeButtonPressed()
{
	setVisible(false);
}
