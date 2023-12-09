//================================================================================================
/// @file KeyComponent.hpp
///
/// @brief Defines a GUI component to draw a soft key.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef KEY_COMPONENT_HPP
#define KEY_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class KeyComponent : public isobus::Key
  , public Component
{
public:
	KeyComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Key sourceObject, int keyWidth, int keyHeight);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyComponent)
};

#endif // KEY_COMPONENT_HPP
