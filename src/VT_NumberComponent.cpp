#include "VT_NumberComponent.hpp"

void VT_NumberComponent::paint(Graphics &g)
{
	g.setColour(juce::Colours::black);
	g.fillAll();

	g.setColour(juce::Colours::white);
	auto font = g.getCurrentFont();
	font.setHeight(getHeight() / 2);
	g.setFont(font);
	g.setColour(juce::Colours::white);

	g.drawFittedText(juce::String::formatted("%d", vtNumber), getLocalBounds(), Justification::centred, false);
}

void VT_NumberComponent::setVtNumber(std::uint8_t newVtNumber)
{
	vtNumber = newVtNumber;
}
