//================================================================================================
/// @file DiagnosticsComponent.cpp
///
/// @brief Implements a GUI component to display active diagnostic trouble codes (DM1)
/// reported by control functions on the bus.
/// @author Sujan Dumaru
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#include "DiagnosticsComponent.hpp"

#include "ManufacturerMap.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"

/// @brief Returns a short label for the lamps a DM1 message reports as on
/// @param[in] lampStatus The first byte of the DM1 payload (lamp status)
/// @param[in] lampFlash The second byte of the DM1 payload (lamp flash)
static String lamp_summary(std::uint8_t lampStatus, std::uint8_t lampFlash)
{
	String retVal;

	if ((0xFF == lampStatus) && (0xFF == lampFlash))
	{
		// ISO 11783 mode sends 0xFF for both lamp bytes, meaning "not available"
		retVal = " [LAMPS N/A]";
	}
	else
	{
		if (0x01 == ((lampStatus >> 6) & 0x03))
		{
			retVal += " [MIL]";
		}
		if (0x01 == ((lampStatus >> 4) & 0x03))
		{
			retVal += " [STOP]";
		}
		if (0x01 == ((lampStatus >> 2) & 0x03))
		{
			retVal += " [WARN]";
		}
		if (0x01 == (lampStatus & 0x03))
		{
			retVal += " [PROTECT]";
		}
	}
	return retVal;
}

DiagnosticsComponent::DiagnosticsComponent()
{
	setSize(WIDTH, LINE_HEIGHT);
	isobus::CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::DiagnosticMessage1), process_dm1_message, this);
	startTimer(1000);
}

DiagnosticsComponent::~DiagnosticsComponent()
{
	stopTimer();
	isobus::CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::DiagnosticMessage1), process_dm1_message, this);
}

void DiagnosticsComponent::paint(Graphics &g)
{
	g.fillAll(Colours::black);
	g.setFont(16.0f);

	if (sources.empty())
	{
		g.setColour(Colours::grey);
		g.drawFittedText("No active DTCs", 4, 0, getWidth() - 4, LINE_HEIGHT, Justification::centredLeft, 1);
		return;
	}

	int line = 0;

	for (const auto &source : sources)
	{
		const auto &entry = source.second;

		g.setColour(Colours::white);
		g.drawFittedText(String("#" + std::to_string(entry.address) + " " + entry.manufacturerName) + lamp_summary(entry.lampStatus, entry.lampFlash), 4, line * LINE_HEIGHT, getWidth() - 4, LINE_HEIGHT, Justification::centredLeft, 1);
		line++;

		g.setColour(Colours::yellow);

		for (const auto &dtc : entry.activeDTCs)
		{
			g.drawFittedText("SPN " + std::to_string(dtc.suspectParameterNumber) + "  FMI " + std::to_string(dtc.failureModeIdentifier) + "  Count " + std::to_string(dtc.occurrenceCount), 16, line * LINE_HEIGHT, getWidth() - 16, LINE_HEIGHT, Justification::centredLeft, 1);
			line++;
		}
	}
}

void DiagnosticsComponent::update_content_height()
{
	int numberOfLines = sources.empty() ? 1 : 0;

	for (const auto &source : sources)
	{
		numberOfLines += 1 + static_cast<int>(source.second.activeDTCs.size());
	}

	int newHeight = numberOfLines * LINE_HEIGHT;
	auto *parentViewport = findParentComponentOfClass<Viewport>();

	if (nullptr != parentViewport)
	{
		newHeight = jmax(newHeight, parentViewport->getMaximumVisibleHeight());
	}
	setSize(getWidth(), newHeight);
	repaint();
}

void DiagnosticsComponent::process_dm1_message(const isobus::CANMessage &message, void *parent)
{
	auto *component = static_cast<DiagnosticsComponent *>(parent);
	auto source = message.get_source_control_function();
	const auto &data = message.get_data();

	if ((nullptr == component) ||
	    (nullptr == source) ||
	    (isobus::ControlFunction::Type::Internal == source->get_type()) ||
	    (data.size() < 8))
	{
		// The VT's own DM1 broadcasts are not shown; the panel is for the other devices on the bus
		return;
	}

	SourceEntry entry;
	entry.address = source->get_address();
	entry.lampStatus = data.at(0);
	entry.lampFlash = data.at(1);
	entry.lastSeenTimestamp = Time::getMillisecondCounter();

	if (manufacturerMap.find(source->get_NAME().get_manufacturer_code()) != manufacturerMap.end())
	{
		entry.manufacturerName = manufacturerMap.at(source->get_NAME().get_manufacturer_code());
	}

	for (std::size_t offset = 2; (offset + 4) <= data.size(); offset += 4)
	{
		ActiveDTC dtc;
		dtc.suspectParameterNumber = static_cast<std::uint32_t>(data.at(offset)) |
		  (static_cast<std::uint32_t>(data.at(offset + 1)) << 8) |
		  (static_cast<std::uint32_t>((data.at(offset + 2) & 0xE0) >> 5) << 16);
		dtc.failureModeIdentifier = data.at(offset + 2) & 0x1F;
		dtc.occurrenceCount = data.at(offset + 3) & 0x7F;

		if (((0 == dtc.suspectParameterNumber) && (0 == dtc.failureModeIdentifier)) ||
		    ((0x7FFFF == dtc.suspectParameterNumber) && (0x1F == dtc.failureModeIdentifier)))
		{
			// A DM1 with no active DTCs carries an all-zero DTC field; 0xFF padding decodes as all-ones
			continue;
		}
		entry.activeDTCs.push_back(dtc);
	}

	const auto mmLock = MessageManagerLock();

	if (entry.activeDTCs.empty())
	{
		component->sources.erase(source->get_NAME().get_full_name());
	}
	else
	{
		component->sources[source->get_NAME().get_full_name()] = std::move(entry);
	}
	component->update_content_height();
}

void DiagnosticsComponent::timerCallback()
{
	const auto currentTimestamp = Time::getMillisecondCounter();
	bool removedSource = false;

	for (auto source = sources.begin(); source != sources.end();)
	{
		if ((currentTimestamp - source->second.lastSeenTimestamp) >= STALE_TIMEOUT_MS)
		{
			source = sources.erase(source);
			removedSource = true;
		}
		else
		{
			++source;
		}
	}

	if (removedSource)
	{
		update_content_height();
	}
}
