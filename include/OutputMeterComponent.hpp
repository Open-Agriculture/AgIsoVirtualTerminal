//================================================================================================
/// @file OutputMeterComponent.hpp
///
/// @brief Defines a GUI component to draw an output meter.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_METER_COMPONENT_HPP
#define OUTPUT_METER_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputMeterComponent : public isobus::OutputMeter
  , public Component
{
public:
	OutputMeterComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputMeter sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputMeterComponent)
};

#endif // OUTPUT_METER_COMPONENT_HPP
