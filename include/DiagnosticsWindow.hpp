//================================================================================================
/// @file DiagnosticsWindow.hpp
///
/// @brief Defines a separate window that shows active diagnostic trouble codes (DM1)
/// reported by control functions on the bus.
/// @author Sujan Dumaru
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "DiagnosticsComponent.hpp"

class ServerMainComponent;

/// @brief A window hosting the diagnostics component, opened from the menu bar
class DiagnosticsWindow : public DocumentWindow
{
public:
	explicit DiagnosticsWindow(ServerMainComponent &parentComponent);

	void closeButtonPressed() override;

	ServerMainComponent &parentServer;

private:
	DiagnosticsComponent content;
	Viewport viewport;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiagnosticsWindow)
};
