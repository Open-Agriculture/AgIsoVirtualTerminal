//================================================================================================
/// @file DiagnosticsComponent.hpp
///
/// @brief Defines a GUI component to display active diagnostic trouble codes (DM1)
/// reported by control functions on the bus.
/// @author Sujan Dumaru
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "isobus/isobus/can_message.hpp"

#include "JuceHeader.h"

#include <map>
#include <string>
#include <vector>

/// @brief Defines a GUI component that lists active DTCs (DM1) grouped per source control function
class DiagnosticsComponent : public Component
{
public:
	DiagnosticsComponent();
	~DiagnosticsComponent() override;

	void paint(Graphics &g) override;

	static constexpr int WIDTH = 260;

private:
	/// @brief One decoded DTC from a DM1 message
	struct ActiveDTC
	{
		std::uint32_t suspectParameterNumber;
		std::uint8_t failureModeIdentifier;
		std::uint8_t occurrenceCount;
	};

	/// @brief The diagnostic state most recently reported by one source control function
	struct SourceEntry
	{
		std::uint8_t address;
		std::string manufacturerName;
		std::uint8_t lampStatus;
		std::uint8_t lampFlash;
		std::vector<ActiveDTC> activeDTCs;
	};

	/// @brief Decodes a received DM1 message and updates the DTC list of its source control function
	static void process_dm1_message(const isobus::CANMessage &message, void *parent);

	static constexpr int LINE_HEIGHT = 14;

	std::map<std::uint64_t, SourceEntry> sources;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiagnosticsComponent)
};
