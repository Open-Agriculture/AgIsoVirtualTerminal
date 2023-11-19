//================================================================================================
/// @file AlarmMaskComponent.hpp
///
/// @brief Defines a GUI component to draw a data mask.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ALARM_MASK_COMPONENT_HPP
#define ALARM_MASK_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class AlarmMaskComponent : public isobus::AlarmMask
  , public Component
{
public:
	AlarmMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::AlarmMask sourceObject);

	void on_content_changed(bool initial = false);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlarmMaskComponent)
};

#endif // ALARM_MASK_COMPONENT_HPP
