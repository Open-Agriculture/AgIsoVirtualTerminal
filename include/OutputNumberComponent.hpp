//================================================================================================
/// @file OutputNumberComponent.hpp
///
/// @brief Defines a GUI component to draw an output number.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_NUMBER_COMPONENT_HPP
#define OUTPUT_NUMBER_COMPONENT_HPP

#include "NumberComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputNumberComponent : public isobus::OutputNumber
  , public NumberComponent
{
public:
	OutputNumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputNumber sourceObject);

	void paint(Graphics &g) override;

private:
	virtual const isobus::VTObject *vtObject() const override
	{
		return static_cast<const isobus::VTObject *>(this);
	};
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputNumberComponent)
};

#endif // OUTPUT_NUMBER_COMPONENT_HPP
