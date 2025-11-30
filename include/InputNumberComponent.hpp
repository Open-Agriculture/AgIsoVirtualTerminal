//================================================================================================
/// @file InputNumberComponent.hpp
///
/// @brief Defines a GUI component to draw an input number.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef INPUT_NUMBER_COMPONENT_HPP
#define INPUT_NUMBER_COMPONENT_HPP

#include "NumberComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class InputNumberComponent : public isobus::InputNumber
  , public NumberComponent
{
public:
	InputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::InputNumber sourceObject);

	void paint(Graphics &g) override;

private:
	virtual const isobus::VTObject *vtObject() const override
	{
		return static_cast<const isobus::VTObject *>(this);
	};
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputNumberComponent)
};

#endif // INPUT_NUMBER_COMPONENT_HPP
