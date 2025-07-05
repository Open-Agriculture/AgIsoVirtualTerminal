//================================================================================================
/// @file WorkingSetLoadingIndicatorComponent.hpp
///
/// @brief Defines a component to show object pools currently being transferred
/// @author Miklos Marton
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================
#include "WorkingSetLoadingIndicatorComponent.hpp"

WorkingSetLoadingIndicatorComponent::WorkingSetLoadingIndicatorComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, int keyHeight, int keyWidth) :
  parentWorkingSet(workingSet),
  width(keyWidth),
  height(keyHeight)
{
	setSize(width, height);
	setOpaque(false);
}

void WorkingSetLoadingIndicatorComponent::paint(Graphics &g)
{
	g.setColour(Colours::black);
	g.fillAll();
	g.setColour(Colours::white);
	auto font = g.getCurrentFont();

	std::ostringstream addrText;
	addrText << "#" << (int)parentWorkingSet->get_control_function()->get_address();
	g.drawText(addrText.str(), 0, 0, width, height * 0.2, Justification::centred, false);

	font.setBold(true);
	g.setFont(font);
	std::ostringstream valueText;
	valueText << std::fixed << std::setprecision(0) << parentWorkingSet->iop_load_percentage() << " %";
	g.drawText(valueText.str(), 0, height * 0.25, width, height * 0.2, Justification::centred, false);

	// draw "progress bar"
	g.setColour(Colours::white);
	g.fillRect(2, height / 2.0 + 2, width - 4, height / 2.0 - 4);
	g.setColour(Colour::fromRGB(57, 255, 20));
	g.fillRect(4, height / 2.0 + 4, (width - 8) * parentWorkingSet->iop_load_percentage() / 100.0, height / 2.0 - 8);
}
