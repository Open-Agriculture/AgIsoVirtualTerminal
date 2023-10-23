//================================================================================================
/// @file InputBooleanComponent.hpp
///
/// @brief Defines a GUI component to draw an input boolean.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef INPUT_BOOLEAN_COMPONENT_HPP
#define INPUT_BOOLEAN_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class InputBooleanComponent : public isobus::InputBoolean
  , public Component
{
public:
	InputBooleanComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputBoolean sourceObject);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
};

#endif // INPUT_BOOLEAN_COMPONENT_HPP
