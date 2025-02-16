#pragma once

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class WorkingSetLoadingIndicatorComponent : public Component
{
public:
	WorkingSetLoadingIndicatorComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, int keyHeight, int keyWidth);

	void paint(Graphics &g) override;

private:
	int height = 0, width = 0;
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkingSetLoadingIndicatorComponent)
};
