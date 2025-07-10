//================================================================================================
/// @file WorkingSetLoadingIndicatorComponent.hpp
///
/// @brief Defines a component to show object pools currently being transferred
/// @author Miklos Marton
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================
#include "WorkingSetLoadingIndicatorComponent.hpp"
#include "ManufacturerMap.hpp"

WorkingSetLoadingIndicatorComponent::WorkingSetLoadingIndicatorComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, int keyHeight, int keyWidth) :
  parentWorkingSet(workingSet),
  width(keyWidth),
  height(keyHeight)
{
	setSize(width, height);
	setOpaque(false);
	if (manufacturerMap.find(parentWorkingSet->get_control_function()->get_NAME().get_manufacturer_code()) != manufacturerMap.end())
	{
		m_manufacturerName = manufacturerMap.at(parentWorkingSet->get_control_function()->get_NAME().get_manufacturer_code());
	}
}

void WorkingSetLoadingIndicatorComponent::paint(Graphics &g)
{
	g.setColour(Colours::black);
	g.fillAll();
	g.setColour(Colours::white);
	auto font = g.getCurrentFont();

	juce::AttributedString attributedText;
	attributedText.setWordWrap(juce::AttributedString::WordWrap::byWord);
	attributedText.setJustification(juce::Justification::top | juce::Justification::horizontallyCentred | juce::Justification::horizontallyJustified);
	attributedText.append("#" + std::to_string(parentWorkingSet->get_control_function()->get_address()) + " " + m_manufacturerName, g.getCurrentFont(), juce::Colours::white);

	juce::TextLayout layout;
	layout.createLayout(attributedText, width);
	layout.draw(g, juce::Rectangle<float>(0, 0, width, height * 0.75));

	// draw "progress bar"
	g.setColour(Colours::white);
	g.fillRect(2, height * 0.75, width - 4, height / 4.0);
	g.setColour(Colour::fromRGB(57, 255, 20));
	g.fillRect(4, height * 0.75 + 2, (width - 8) * parentWorkingSet->iop_load_percentage() / 100.0, height / 4.0 - 4);

	font.setBold(true);
	g.setFont(font);
	g.setColour(Colours::black);
	std::ostringstream valueText;
	valueText << std::fixed << std::setprecision(0) << parentWorkingSet->iop_load_percentage() << " %";
	g.drawText(valueText.str(), 0, height * 0.75 + 2, width, height / 4.0 - 4, Justification::centred, false);
}
