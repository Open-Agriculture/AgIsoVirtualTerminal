//================================================================================================
/// @file ContainerComponent.hpp
///
/// @brief Defines a GUI component to draw a container.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef CONTAINER_COMPONENT_HPP
#define CONTAINER_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class ContainerComponent : public isobus::Container
  , public Component
{
public:
	ContainerComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::Container sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContainerComponent)
};

#endif // CONTAINER_COMPONENT_HPP
