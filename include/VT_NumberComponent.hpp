//================================================================================================
/// @file VT_NumberComponent.hpp
///
/// @brief A component that shows the VT number when a client sends Identify VT
/// @author Miklos Marton
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================

#ifndef VT_NUMBER_COMPONENT_HPP
#define VT_NUMBER_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class VT_NumberComponent : public Component
{
public:
	VT_NumberComponent() = default;

	void paint(Graphics &g) override;

	void setVtNumber(std::uint8_t newVtNumber);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::uint8_t vtNumber;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VT_NumberComponent)
};

#endif // VT_NUMBER_COMPONENT_HPP
