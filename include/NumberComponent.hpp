//================================================================================================
/// @file NumberComponent.hpp
///
/// @brief Common functions for drawing numbers
/// @author Miklos Marton
///
/// @copyright 2024 Adrian Del Grosso
//================================================================================================
#ifndef NUMBER_COMPONENT_HPP
#define NUMBER_COMPONENT_HPP

#include "TextDrawingComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class NumberComponent : public TextDrawingComponent
{
public:
	NumberComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);

	/// @brief Returns the decimal a pool's float scale was written as
	/// @param[in] scale The object's scale
	/// @returns The shortest decimal that reads back as the same float
	static double get_decimal_scale(float scale);

protected:
	void paintNumber(Graphics &g, bool enabled = true);

private:
	/// @brief Rounds or truncates a value to a number of decimals
	/// @param[in] value The scaled value
	/// @param[in] decimals The number of decimals shown
	/// @param[in] truncate True to truncate, false to round
	/// @returns The value as it is shown
	static double round_to_decimals(double value, std::uint8_t decimals, bool truncate);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberComponent)
};

#endif // NUMBER_COMPONENT_HPP
