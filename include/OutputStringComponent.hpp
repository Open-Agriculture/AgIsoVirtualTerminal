//================================================================================================
/// @file OutputStringComponent.hpp
///
/// @brief Defines a GUI component to draw an output string.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_STRING_COMPONENT_HPP
#define OUTPUT_STRING_COMPONENT_HPP

#include "StringDrawingComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputStringComponent : public isobus::OutputString
  , public StringDrawingComponent
{
public:
	OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject);

	void paint(Graphics &g) override;

private:
	virtual const isobus::VTObject *vtObject() const override
	{
		return static_cast<const isobus::VTObject *>(this);
	};
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStringComponent)
};

#endif // OUTPUT_STRING_COMPONENT_HPP
