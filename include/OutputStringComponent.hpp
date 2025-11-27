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

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputStringComponent : public isobus::OutputString
  , public Component
  , private juce::Timer
{
public:
	OutputStringComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputString sourceObject);

	void paint(Graphics &g) override;

	static Justification convert_justification(HorizontalJustification horizontalJustification, VerticalJustification verticalJustification);

	void visibilityChanged() override;

	void timerCallback() override;

private:
	/**
   * @brief isFlashing
   * @return true if the OutputString font attribute has any of the flashing mode set, false otherwise
   */
	bool isFlashing() const;
	bool visible = false;
	/**
   * @brief show - a boolean idicating that the string needs to be drawn based on the flashing state
   */
	bool show = true;
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputStringComponent)
};

#endif // OUTPUT_STRING_COMPONENT_HPP
