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
  , private Timer
{
public:
	DiagnosticsComponent();
	~DiagnosticsComponent() override;

	void paint(Graphics &g) override;
	void update_content_height();

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
		std::uint32_t lastSeenTimestamp;
		std::vector<ActiveDTC> activeDTCs;
	};

	/// @brief Decodes a received DM1 message and updates the DTC list of its source control function
	static void process_dm1_message(const isobus::CANMessage &message, void *parent);

	/// @brief Stores the latest DM1 state of one source, or drops it when no DTC is active. Runs on the message thread.
	/// @param[in] sourceNAME The full NAME of the source control function
	/// @param[in] entry The decoded DM1 state
	void update_source(std::uint64_t sourceNAME, SourceEntry entry);
	static TextLayout source_summary_layout(const SourceEntry &entry, int width);
	void timerCallback() override;

	static constexpr int LINE_HEIGHT = 20;
	static constexpr float FONT_HEIGHT = 17.0f;
	static constexpr int SPN_COLUMN_X = 16;
	static constexpr int FMI_COLUMN_X = 120;
	static constexpr int COUNT_COLUMN_X = 190;
	static constexpr std::uint32_t STALE_TIMEOUT_MS = 7000;

	std::map<std::uint64_t, SourceEntry> sources;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiagnosticsComponent)
};
